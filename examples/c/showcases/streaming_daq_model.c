/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */


/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "streaming_daq_model.h"

#include <math.h>
#include <stddef.h>

#include "_alloc.h"
#include "_compat.h"
#include "_time_utils.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

static const float TAU = 6.28318530718f;
static const uint64_t NS_PER_SECOND = 1000000000ULL;
static const uint32_t UNIT_COUNT = 28u;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

/**
 * Return a deterministic integer hash.
 *
 * @param value input value
 * @return mixed value
 */
static uint32_t _hash_u32(uint32_t value)
{
    value ^= value >> 16u;
    value *= 0x7feb352du;
    value ^= value >> 15u;
    value *= 0x846ca68bu;
    value ^= value >> 16u;
    return value;
}


/**
 * Return deterministic signed noise for one channel sample.
 *
 * @param model source model
 * @param channel channel index
 * @param sample logical sample index
 * @return noise in [-1, +1]
 */
static float _sample_noise(const DaqModel* model, uint32_t channel, uint64_t sample)
{
    uint32_t folded = (uint32_t)sample ^ (uint32_t)(sample >> 32u);
    uint32_t hash = _hash_u32(folded ^ (channel * 0x9e3779b9u) ^ model->config.seed);
    return 2.0f * ((float)(hash & 0xffffu) / 65535.0f) - 1.0f;
}


/**
 * Return one deterministic event sample in a repeated timing cell.
 *
 * @param model source model
 * @param stream deterministic stream index
 * @param cell repeated timing-cell index
 * @param period timing-cell length in samples
 * @param jitter maximum signed timing jitter
 * @return logical event sample
 */
static int64_t _event_sample(
    const DaqModel* model, uint32_t stream, int64_t cell, uint32_t period, uint32_t jitter)
{
    uint32_t folded = (uint32_t)cell ^ (uint32_t)((uint64_t)cell >> 32u);
    uint32_t hash = _hash_u32(folded ^ (stream * 0x9e3779b9u) ^ model->config.seed ^ 0x4f1bbcdcu);
    int32_t offset = jitter > 0u ? (int32_t)(hash % (2u * jitter + 1u)) - (int32_t)jitter : 0;
    return cell * (int64_t)period + period / 2u + offset;
}


/**
 * Return a compact extracellular action-potential waveform.
 *
 * @param delta samples relative to the unit firing time
 * @return normalized biphasic waveform
 */
static float _spike_waveform(int64_t delta)
{
    if (delta < -3 || delta > 19)
        return 0.0f;
    const float x = (float)delta;
    const float pre = 0.18f * expf(-0.5f * ((x + 1.2f) / 1.2f) * ((x + 1.2f) / 1.2f));
    const float trough = -1.15f * expf(-0.5f * ((x - 3.0f) / 2.2f) * ((x - 3.0f) / 2.2f));
    const float rebound = 0.42f * expf(-0.5f * ((x - 9.0f) / 4.2f) * ((x - 9.0f) / 4.2f));
    return pre + trough + rebound;
}


/**
 * Return one unit waveform value at a logical sample.
 *
 * @param model source model
 * @param unit synthetic unit index
 * @param sample logical sample
 * @param rate_scale firing-rate multiplier
 * @return waveform value, or zero away from a spike
 */
static float _unit_value(const DaqModel* model, uint32_t unit, uint64_t sample, float rate_scale)
{
    const float base_rate_hz = 5.0f + 1.7f * (float)(unit % 9u);
    uint32_t period = (uint32_t)lroundf(
        (float)model->config.sample_rate_hz / fmaxf(0.5f, base_rate_hz * rate_scale));
    if (period < 40u)
        period = 40u;
    const uint32_t jitter = period / 4u;
    int64_t cell = (int64_t)(sample / period);
    for (int32_t dc = -1; dc <= 1; dc++)
    {
        int64_t candidate_cell = cell + dc;
        if (candidate_cell < 0)
            continue;
        int64_t firing = _event_sample(model, unit, candidate_cell, period, jitter);
        float waveform = _spike_waveform((int64_t)sample - firing);
        if (waveform != 0.0f)
            return waveform;
    }
    return 0.0f;
}


/**
 * Return the event marker at one logical sample.
 *
 * @param model source model
 * @param sample logical sample
 * @return event kind
 */
static DaqEventKind _event_kind(const DaqModel* model, uint64_t sample)
{
    const uint32_t period = model->config.sample_rate_hz / 4u;
    if (period == 0u)
        return DAQ_EVENT_NONE;
    int64_t cell = (int64_t)(sample / period);
    for (int32_t dc = -1; dc <= 1; dc++)
    {
        int64_t candidate_cell = cell + dc;
        if (candidate_cell < 0)
            continue;
        int64_t event = _event_sample(model, 97u, candidate_cell, period, period / 7u);
        if ((int64_t)sample == event)
        {
            switch ((uint32_t)candidate_cell % 6u)
            {
            case 1u:
            case 4u:
                return DAQ_EVENT_REWARD;
            case 3u:
                return DAQ_EVENT_SYNC;
            default:
                return DAQ_EVENT_STIMULUS;
            }
        }
    }
    return DAQ_EVENT_NONE;
}


/**
 * Add one spatially tapered spike to nearby channels.
 *
 * @param model source model
 * @param unit synthetic unit index
 * @param waveform temporal waveform value
 * @param amplitude overall spike amplitude
 * @param values one interleaved sample row
 */
static void _add_unit_spike(
    const DaqModel* model, uint32_t unit, float waveform, float amplitude, float* values)
{
    if (waveform == 0.0f)
        return;
    uint32_t center = unit % model->config.channel_count;
    if (model->config.channel_count > 6u)
    {
        center = 3u + _hash_u32(model->config.seed ^ (unit * 0x45d9f3bu)) %
                          (model->config.channel_count - 6u);
    }
    const int32_t radius = 4 + (int32_t)(unit % 3u);
    for (int32_t offset = -radius; offset <= radius; offset++)
    {
        int32_t channel = (int32_t)center + offset;
        if (channel < 0 || channel >= (int32_t)model->config.channel_count)
            continue;
        const float distance = (float)offset / (0.46f * (float)radius);
        const float spatial = expf(-0.5f * distance * distance);
        const float polarity = (unit % 5u == 0u && offset > 1) ? -0.72f : 1.0f;
        values[channel] += amplitude * polarity * spatial * waveform;
    }
}


/**
 * Add deterministic population activity around hardware events.
 *
 * @param model source model
 * @param sample logical sample
 * @param amplitude population-event amplitude
 * @param values one interleaved sample row
 */
static void
_add_population_event(const DaqModel* model, uint64_t sample, float amplitude, float* values)
{
    const uint32_t period = model->config.sample_rate_hz / 4u;
    if (period == 0u || amplitude == 0.0f)
        return;
    int64_t cell = (int64_t)(sample / period);
    for (int32_t dc = -1; dc <= 1; dc++)
    {
        int64_t candidate_cell = cell + dc;
        if (candidate_cell < 0)
            continue;
        int64_t event = _event_sample(model, 97u, candidate_cell, period, period / 7u);
        float waveform = _spike_waveform((int64_t)sample - event - 4);
        if (waveform == 0.0f)
            continue;
        for (uint32_t group = 0; group < 3u; group++)
        {
            uint32_t pseudo_unit = (uint32_t)(candidate_cell * 5 + group * 9) % UNIT_COUNT;
            _add_unit_spike(model, pseudo_unit, waveform, amplitude, values);
        }
    }
}


/**
 * Fill one interleaved acquisition block.
 *
 * @param model source model
 * @param first_sample first logical sample
 * @param sample_count block sample count
 * @param out_values output interleaved values
 */
static void _generate_values(
    const DaqModel* model, uint64_t first_sample, uint32_t sample_count, float* out_values)
{
    const float noise_scale =
        0.125f *
        (float)atomic_load_explicit(&model->producer_noise_permille, memory_order_relaxed) /
        1000.0f;
    const float rate_scale =
        (float)atomic_load_explicit(&model->producer_spike_rate_permille, memory_order_relaxed) /
        1000.0f;
    const float spike_amplitude =
        0.88f *
        (float)atomic_load_explicit(
            &model->producer_spike_amplitude_permille, memory_order_relaxed) /
        1000.0f;
    const float synchrony =
        0.58f *
        (float)atomic_load_explicit(&model->producer_synchrony_permille, memory_order_relaxed) /
        1000.0f;
    for (uint32_t i = 0; i < sample_count; i++)
    {
        uint64_t sample = first_sample + i;
        float* values = &out_values[(uint64_t)i * model->config.channel_count];
        const float t = (float)((double)sample / (double)model->config.sample_rate_hz);
        for (uint32_t channel = 0; channel < model->config.channel_count; channel++)
        {
            const uint32_t shank = channel / 32u;
            const float shared = 0.48f * _sample_noise(model, 311u + shank, sample) +
                                 0.22f * _sample_noise(model, 401u + channel / 4u, sample);
            const float local = _sample_noise(model, channel, sample);
            const float lfp = 0.34f * sinf(TAU * (5.2f + 0.35f * shank) * t + 0.04f * channel) +
                              0.16f * sinf(TAU * 17.0f * t + 0.11f * channel);
            values[channel] = noise_scale * (0.58f * local + shared + lfp);
        }

        for (uint32_t unit = 0; unit < UNIT_COUNT; unit++)
        {
            float waveform = _unit_value(model, unit, sample, rate_scale);
            _add_unit_spike(model, unit, waveform, spike_amplitude, values);
        }
        _add_population_event(model, sample, synchrony, values);
    }
}


/**
 * Write invalid samples into the display ring to represent a producer gap.
 *
 * @param model destination model
 * @param sample_count gap sample count
 */
static void _append_gap(DaqModel* model, uint64_t sample_count)
{
    if (sample_count >= model->config.display_sample_count)
    {
        for (uint32_t i = 0; i < model->config.display_sample_count; i++)
        {
            model->display_valid[i] = false;
            model->display_events[i] = DAQ_EVENT_NONE;
        }
        uint64_t wraps =
            ((uint64_t)model->write_index + sample_count) / model->config.display_sample_count;
        model->wrap_count += wraps;
        model->write_index =
            (model->write_index + sample_count % model->config.display_sample_count) %
            model->config.display_sample_count;
        return;
    }

    for (uint64_t i = 0; i < sample_count; i++)
    {
        model->display_valid[model->write_index] = false;
        model->display_events[model->write_index] = DAQ_EVENT_NONE;
        model->write_index++;
        if (model->write_index == model->config.display_sample_count)
        {
            model->write_index = 0;
            model->wrap_count++;
        }
    }
}


/**
 * Append one acquisition block to the display ring.
 *
 * @param model destination model
 * @param block source block
 * @return logical sample count advanced, including detected gaps
 */
static uint64_t _append_block(DaqModel* model, const DaqBlock* block)
{
    uint64_t advanced = 0;
    if (block->first_sample > model->next_expected_sample)
    {
        uint64_t gap = block->first_sample - model->next_expected_sample;
        _append_gap(model, gap);
        advanced += gap;
    }
    else if (block->first_sample < model->next_expected_sample)
    {
        return 0;
    }

    for (uint32_t i = 0; i < block->sample_count; i++)
    {
        uint64_t dst = (uint64_t)model->write_index * model->config.channel_count;
        uint64_t src = (uint64_t)i * model->config.channel_count;
        uint64_t byte_size = (uint64_t)model->config.channel_count * sizeof(float);
        dvz_memcpy(&model->display_values[dst], byte_size, &block->values[src], byte_size);
        model->display_valid[model->write_index] = true;
        model->display_events[model->write_index] =
            (uint8_t)_event_kind(model, block->first_sample + i);
        model->write_index++;
        if (model->write_index == model->config.display_sample_count)
        {
            model->write_index = 0;
            model->wrap_count++;
        }
    }
    advanced += block->sample_count;
    model->next_expected_sample = block->first_sample + block->sample_count;
    return advanced;
}


/**
 * Resolve physical dirty spans from a starting ring index and advance count.
 *
 * @param model source model
 * @param first physical starting index
 * @param advanced logical advance count
 * @param out_dirty output ranges
 */
static void
_dirty_ranges(const DaqModel* model, uint32_t first, uint64_t advanced, DaqDirtyRanges* out_dirty)
{
    dvz_memset(out_dirty, sizeof(*out_dirty), 0, sizeof(*out_dirty));
    out_dirty->advanced_sample_count = advanced > UINT32_MAX ? UINT32_MAX : (uint32_t)advanced;
    if (advanced == 0)
        return;
    if (advanced >= model->config.display_sample_count)
    {
        out_dirty->full = true;
        out_dirty->span_count = 1;
        out_dirty->spans[0] = (DaqDirtySpan){0, model->config.display_sample_count};
        return;
    }

    uint32_t count = (uint32_t)advanced;
    uint32_t first_count = model->config.display_sample_count - first;
    if (count <= first_count)
    {
        out_dirty->span_count = 1;
        out_dirty->spans[0] = (DaqDirtySpan){first, count};
    }
    else
    {
        out_dirty->span_count = 2;
        out_dirty->spans[0] = (DaqDirtySpan){first, first_count};
        out_dirty->spans[1] = (DaqDirtySpan){0, count - first_count};
    }
}


/**
 * Return whether one deterministic acquisition block should be dropped.
 *
 * @param model source model
 * @param first_sample block sample index
 * @return whether to drop the block
 */
static bool _drop_block(const DaqModel* model, uint64_t first_sample)
{
    uint32_t dropout =
        atomic_load_explicit(&model->producer_dropout_permille, memory_order_relaxed);
    if (dropout == 0)
        return false;
    uint32_t folded = (uint32_t)first_sample ^ (uint32_t)(first_sample >> 32u);
    return _hash_u32(folded ^ model->config.seed ^ 0xa511e9b3u) % 1000u < dropout;
}


/**
 * Run the wall-clock acquisition producer.
 *
 * @param user_data producer model
 * @return always NULL
 */
static void* _producer_main(void* user_data)
{
    DaqModel* model = (DaqModel*)user_data;
    uint64_t next_ns = atomic_load_explicit(&model->producer_clock_ns, memory_order_acquire);
    while (atomic_load_explicit(&model->producer_running, memory_order_acquire))
    {
        if (atomic_load_explicit(&model->producer_paused, memory_order_relaxed))
        {
            next_ns = dvz_time_monotonic_ns();
            dvz_sleep_us(1000);
            continue;
        }

        uint32_t block_size =
            atomic_load_explicit(&model->producer_block_size, memory_order_relaxed);
        uint64_t block_ns = (uint64_t)block_size * NS_PER_SECOND / model->config.sample_rate_hz;
        uint64_t now = dvz_time_monotonic_ns();
        if (now < next_ns + block_ns)
        {
            uint64_t wait_us = (next_ns + block_ns - now) / 1000u;
            dvz_sleep_us((int)(wait_us > 1000u ? 1000u : wait_us));
            continue;
        }

        uint32_t produced_this_turn = 0;
        while (now >= next_ns + block_ns && produced_this_turn < 8u &&
               !atomic_load_explicit(&model->producer_paused, memory_order_relaxed))
        {
            uint64_t first_sample = model->producer_next_sample;
            bool drop = _drop_block(model, first_sample);
            unsigned head = atomic_load_explicit(&model->queue_head, memory_order_relaxed);
            unsigned tail = atomic_load_explicit(&model->queue_tail, memory_order_acquire);
            bool full = head - tail >= DAQ_BLOCK_QUEUE_CAPACITY;
            if (drop || full)
            {
                atomic_fetch_add_explicit(
                    &model->dropped_sample_count, block_size, memory_order_relaxed);
                if (full)
                {
                    atomic_fetch_add_explicit(
                        &model->overrun_block_count, 1u, memory_order_relaxed);
                }
            }
            else
            {
                DaqBlock* block = &model->queue[head % DAQ_BLOCK_QUEUE_CAPACITY];
                block->first_sample = first_sample;
                block->sample_count = block_size;
                _generate_values(model, first_sample, block_size, block->values);
                atomic_store_explicit(&model->queue_head, head + 1u, memory_order_release);
            }
            model->producer_next_sample += block_size;
            atomic_fetch_add_explicit(
                &model->generated_sample_count, block_size, memory_order_relaxed);
            next_ns += block_ns;
            atomic_store_explicit(
                &model->producer_clock_sample, model->producer_next_sample, memory_order_relaxed);
            atomic_store_explicit(&model->producer_clock_ns, next_ns, memory_order_release);
            produced_this_turn++;
            block_size = atomic_load_explicit(&model->producer_block_size, memory_order_relaxed);
            block_ns = (uint64_t)block_size * NS_PER_SECOND / model->config.sample_rate_hz;
            now = dvz_time_monotonic_ns();
        }
    }
    return NULL;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the default synthetic DAQ configuration.
 *
 * @return default configuration
 */
DaqConfig daq_config_default(void)
{
    return (DaqConfig){
        .channel_count = 128u,
        .analog_channel_count = 128u,
        .sample_rate_hz = 10000u,
        .display_sample_count = 10000u,
        .block_size = 64u,
        .seed = 20260716u,
    };
}


/**
 * Initialize a deterministic DAQ model and its fixed-size display ring.
 *
 * @param model model to initialize
 * @param config model configuration
 * @return whether initialization succeeded
 */
bool daq_model_init(DaqModel* model, const DaqConfig* config)
{
    if (model == NULL || config == NULL || config->channel_count == 0 ||
        config->channel_count > DAQ_MAX_CHANNELS || config->analog_channel_count == 0 ||
        config->analog_channel_count > config->channel_count || config->sample_rate_hz == 0 ||
        config->display_sample_count < 2u || config->block_size == 0 ||
        config->block_size > DAQ_MAX_BLOCK_SAMPLES)
    {
        return false;
    }

    dvz_memset(model, sizeof(*model), 0, sizeof(*model));
    model->config = *config;
    uint64_t value_count = (uint64_t)config->channel_count * config->display_sample_count;
    if (value_count > SIZE_MAX / sizeof(float))
        return false;
    model->display_values = (float*)dvz_calloc(value_count, sizeof(float));
    model->display_valid = (bool*)dvz_calloc(config->display_sample_count, sizeof(bool));
    model->display_events = (uint8_t*)dvz_calloc(config->display_sample_count, sizeof(uint8_t));
    if (model->display_values == NULL || model->display_valid == NULL ||
        model->display_events == NULL)
    {
        daq_model_destroy(model);
        return false;
    }

    const uint32_t digital_count = config->channel_count - config->analog_channel_count;
    static const char* const digital_names[] = {
        "trial_start", "stimulus",    "photodiode", "response",
        "reward",      "camera_sync", "frame_sync", "clock_100hz",
    };
    for (uint32_t channel = 0; channel < config->channel_count; channel++)
    {
        DaqChannel* meta = &model->channels[channel];
        if (channel < digital_count)
        {
            meta->kind = DAQ_CHANNEL_DIGITAL;
            if (channel < sizeof(digital_names) / sizeof(digital_names[0]))
                dvz_snprintf(meta->name, sizeof(meta->name), "%s", digital_names[channel]);
            else
                dvz_snprintf(meta->name, sizeof(meta->name), "ttl_%03u", channel);
        }
        else
        {
            uint32_t analog = channel - digital_count;
            meta->kind = DAQ_CHANNEL_ANALOG;
            dvz_snprintf(meta->name, sizeof(meta->name), "AP%03u", analog);
        }
    }

    atomic_init(&model->queue_head, 0u);
    atomic_init(&model->queue_tail, 0u);
    atomic_init(&model->producer_running, false);
    atomic_init(&model->producer_paused, false);
    atomic_init(&model->producer_block_size, config->block_size);
    atomic_init(&model->producer_dropout_permille, 0u);
    atomic_init(&model->producer_noise_permille, 1000u);
    atomic_init(&model->producer_spike_rate_permille, 1000u);
    atomic_init(&model->producer_spike_amplitude_permille, 1000u);
    atomic_init(&model->producer_synchrony_permille, 1000u);
    atomic_init(&model->producer_clock_sample, 0u);
    atomic_init(&model->producer_clock_ns, 0u);
    atomic_init(&model->generated_sample_count, 0u);
    atomic_init(&model->dropped_sample_count, 0u);
    atomic_init(&model->overrun_block_count, 0u);
    return true;
}


/**
 * Release model-owned storage after stopping the producer.
 *
 * @param model model to destroy
 */
void daq_model_destroy(DaqModel* model)
{
    if (model == NULL)
        return;
    daq_model_stop(model);
    dvz_free(model->display_events);
    dvz_free(model->display_valid);
    dvz_free(model->display_values);
    model->display_events = NULL;
    model->display_valid = NULL;
    model->display_values = NULL;
}


/**
 * Fill the display ring with deterministic history before presentation.
 *
 * @param model initialized model
 * @return whether the complete history was generated
 */
bool daq_model_prefill(DaqModel* model)
{
    if (model == NULL || model->display_values == NULL)
        return false;
    uint64_t remaining = (uint64_t)model->config.display_sample_count +
                         3u * (uint64_t)model->config.display_sample_count / 4u;
    while (remaining > 0)
    {
        uint32_t count =
            remaining > DAQ_MAX_BLOCK_SAMPLES ? DAQ_MAX_BLOCK_SAMPLES : (uint32_t)remaining;
        DaqBlock block = {.first_sample = model->next_expected_sample, .sample_count = count};
        _generate_values(model, block.first_sample, count, block.values);
        if (_append_block(model, &block) != count)
            return false;
        remaining -= count;
    }
    model->wrap_count = 0;
    model->producer_next_sample = model->next_expected_sample;
    atomic_store_explicit(
        &model->producer_clock_sample, model->producer_next_sample, memory_order_relaxed);
    atomic_store_explicit(
        &model->generated_sample_count, model->next_expected_sample, memory_order_relaxed);
    return true;
}


/**
 * Start the wall-clock-paced acquisition producer.
 *
 * @param model initialized model
 * @return whether the producer started
 */
bool daq_model_start(DaqModel* model)
{
    if (model == NULL || model->producer_started)
        return model != NULL && model->producer_started;
    atomic_store_explicit(
        &model->producer_clock_sample, model->producer_next_sample, memory_order_relaxed);
    atomic_store_explicit(
        &model->producer_clock_ns, dvz_time_monotonic_ns(), memory_order_release);
    atomic_store_explicit(&model->producer_running, true, memory_order_release);
    if (pthread_create(&model->producer_thread, NULL, _producer_main, model) != 0)
    {
        atomic_store_explicit(&model->producer_running, false, memory_order_release);
        return false;
    }
    model->producer_started = true;
    return true;
}


/**
 * Stop and join the acquisition producer when it is active.
 *
 * @param model model owning the producer
 */
void daq_model_stop(DaqModel* model)
{
    if (model == NULL || !model->producer_started)
        return;
    atomic_store_explicit(&model->producer_running, false, memory_order_release);
    pthread_join(model->producer_thread, NULL);
    model->producer_started = false;
}


/**
 * Pause or resume wall-clock acquisition without accumulating catch-up work.
 *
 * @param model model owning the producer
 * @param paused whether acquisition should pause
 */
void daq_model_set_paused(DaqModel* model, bool paused)
{
    if (model == NULL)
        return;
    if (!paused)
    {
        atomic_store_explicit(
            &model->producer_clock_ns, dvz_time_monotonic_ns(), memory_order_release);
    }
    atomic_store_explicit(&model->producer_paused, paused, memory_order_relaxed);
}


/**
 * Set the producer block size for later acquisition blocks.
 *
 * @param model model owning the producer
 * @param block_size requested sample count per block
 */
void daq_model_set_block_size(DaqModel* model, uint32_t block_size)
{
    if (model == NULL)
        return;
    if (block_size < 1u)
        block_size = 1u;
    if (block_size > DAQ_MAX_BLOCK_SAMPLES)
        block_size = DAQ_MAX_BLOCK_SAMPLES;
    atomic_store_explicit(&model->producer_block_size, block_size, memory_order_relaxed);
}


/**
 * Set deterministic producer dropout frequency.
 *
 * @param model model owning the producer
 * @param dropout_permille requested dropped blocks per thousand
 */
void daq_model_set_dropout(DaqModel* model, uint32_t dropout_permille)
{
    if (model == NULL)
        return;
    if (dropout_permille > 250u)
        dropout_permille = 250u;
    atomic_store_explicit(
        &model->producer_dropout_permille, dropout_permille, memory_order_relaxed);
}


/**
 * Set synthetic noise amplitude.
 *
 * @param model model owning the producer
 * @param noise_permille noise amplitude in thousandths of the default scale
 */
void daq_model_set_noise(DaqModel* model, uint32_t noise_permille)
{
    if (model == NULL)
        return;
    if (noise_permille > 3000u)
        noise_permille = 3000u;
    atomic_store_explicit(&model->producer_noise_permille, noise_permille, memory_order_relaxed);
}


/**
 * Set the synthetic unit firing-rate multiplier.
 *
 * @param model model owning the producer
 * @param rate_permille rate multiplier in thousandths
 */
void daq_model_set_spike_rate(DaqModel* model, uint32_t rate_permille)
{
    if (model == NULL)
        return;
    if (rate_permille < 100u)
        rate_permille = 100u;
    if (rate_permille > 3000u)
        rate_permille = 3000u;
    atomic_store_explicit(
        &model->producer_spike_rate_permille, rate_permille, memory_order_relaxed);
}


/**
 * Set the synthetic extracellular spike amplitude.
 *
 * @param model model owning the producer
 * @param amplitude_permille amplitude multiplier in thousandths
 */
void daq_model_set_spike_amplitude(DaqModel* model, uint32_t amplitude_permille)
{
    if (model == NULL)
        return;
    if (amplitude_permille > 3000u)
        amplitude_permille = 3000u;
    atomic_store_explicit(
        &model->producer_spike_amplitude_permille, amplitude_permille, memory_order_relaxed);
}


/**
 * Set the amplitude of population-synchronous activity.
 *
 * @param model model owning the producer
 * @param synchrony_permille synchrony multiplier in thousandths
 */
void daq_model_set_synchrony(DaqModel* model, uint32_t synchrony_permille)
{
    if (model == NULL)
        return;
    if (synchrony_permille > 3000u)
        synchrony_permille = 3000u;
    atomic_store_explicit(
        &model->producer_synchrony_permille, synchrony_permille, memory_order_relaxed);
}


/**
 * Drain bounded producer blocks into the display ring.
 *
 * @param model model receiving queued blocks
 * @param max_blocks maximum block count to consume, or zero for all available blocks
 * @param out_dirty physical display ranges changed by the drain
 * @return consumed block count
 */
uint32_t daq_model_drain(DaqModel* model, uint32_t max_blocks, DaqDirtyRanges* out_dirty)
{
    if (model == NULL || out_dirty == NULL)
        return 0;
    uint32_t first = model->write_index;
    uint64_t advanced = 0;
    uint32_t consumed = 0;
    unsigned tail = atomic_load_explicit(&model->queue_tail, memory_order_relaxed);
    unsigned head = atomic_load_explicit(&model->queue_head, memory_order_acquire);
    while (tail != head && (max_blocks == 0 || consumed < max_blocks))
    {
        const DaqBlock* block = &model->queue[tail % DAQ_BLOCK_QUEUE_CAPACITY];
        advanced += _append_block(model, block);
        tail++;
        consumed++;
    }
    atomic_store_explicit(&model->queue_tail, tail, memory_order_release);
    _dirty_ranges(model, first, advanced, out_dirty);
    return consumed;
}


/**
 * Advance the deterministic non-threaded source by an exact number of samples.
 *
 * @param model model to advance
 * @param sample_count logical sample count to generate and consume
 * @param out_dirty physical display ranges changed by the advance
 * @return whether the requested samples were generated
 */
bool daq_model_advance(DaqModel* model, uint32_t sample_count, DaqDirtyRanges* out_dirty)
{
    if (model == NULL || out_dirty == NULL)
        return false;
    uint32_t first = model->write_index;
    uint64_t advanced = 0;
    uint32_t remaining = sample_count;
    while (remaining > 0)
    {
        uint32_t count = remaining > DAQ_MAX_BLOCK_SAMPLES ? DAQ_MAX_BLOCK_SAMPLES : remaining;
        DaqBlock block = {.first_sample = model->next_expected_sample, .sample_count = count};
        _generate_values(model, block.first_sample, count, block.values);
        advanced += _append_block(model, &block);
        atomic_fetch_add_explicit(&model->generated_sample_count, count, memory_order_relaxed);
        model->producer_next_sample = model->next_expected_sample;
        remaining -= count;
    }
    _dirty_ranges(model, first, advanced, out_dirty);
    return advanced == sample_count;
}


/**
 * Reset the display ring and deterministic acquisition position.
 *
 * @param model model to reset
 * @return whether a fresh history was generated
 */
bool daq_model_reset(DaqModel* model)
{
    if (model == NULL || model->display_values == NULL)
        return false;
    bool was_started = model->producer_started;
    bool was_paused = atomic_load_explicit(&model->producer_paused, memory_order_relaxed);
    daq_model_stop(model);

    uint64_t value_count =
        (uint64_t)model->config.channel_count * model->config.display_sample_count;
    dvz_memset(model->display_values, value_count * sizeof(float), 0, value_count * sizeof(float));
    dvz_memset(
        model->display_valid, model->config.display_sample_count * sizeof(bool), 0,
        model->config.display_sample_count * sizeof(bool));
    dvz_memset(
        model->display_events, model->config.display_sample_count * sizeof(uint8_t), 0,
        model->config.display_sample_count * sizeof(uint8_t));
    model->write_index = 0;
    model->next_expected_sample = 0;
    model->wrap_count = 0;
    model->producer_next_sample = 0;
    atomic_store_explicit(&model->producer_clock_sample, 0u, memory_order_relaxed);
    atomic_store_explicit(&model->producer_clock_ns, 0u, memory_order_relaxed);
    atomic_store_explicit(&model->queue_head, 0u, memory_order_relaxed);
    atomic_store_explicit(&model->queue_tail, 0u, memory_order_relaxed);
    atomic_store_explicit(&model->generated_sample_count, 0u, memory_order_relaxed);
    atomic_store_explicit(&model->dropped_sample_count, 0u, memory_order_relaxed);
    atomic_store_explicit(&model->overrun_block_count, 0u, memory_order_relaxed);
    if (!daq_model_prefill(model))
        return false;
    atomic_store_explicit(&model->producer_paused, was_paused, memory_order_relaxed);
    return !was_started || daq_model_start(model);
}


/**
 * Copy current producer and display statistics.
 *
 * @param model source model
 * @param out output statistics
 */
void daq_model_stats(const DaqModel* model, DaqStats* out)
{
    if (model == NULL || out == NULL)
        return;
    unsigned head = atomic_load_explicit(&model->queue_head, memory_order_acquire);
    unsigned tail = atomic_load_explicit(&model->queue_tail, memory_order_acquire);
    *out = (DaqStats){
        .generated_sample_count =
            atomic_load_explicit(&model->generated_sample_count, memory_order_relaxed),
        .consumed_sample_count = model->next_expected_sample,
        .dropped_sample_count =
            atomic_load_explicit(&model->dropped_sample_count, memory_order_relaxed),
        .overrun_block_count =
            atomic_load_explicit(&model->overrun_block_count, memory_order_relaxed),
        .wrap_count = model->wrap_count,
        .queue_depth = head - tail,
    };
}


/**
 * Return the wall-clock-interpolated logical hardware cursor sample.
 *
 * @param model source model
 * @return fractional logical sample position
 */
double daq_model_cursor_sample(const DaqModel* model)
{
    if (model == NULL)
        return 0.0;

    if (!atomic_load_explicit(&model->producer_running, memory_order_acquire) ||
        atomic_load_explicit(&model->producer_paused, memory_order_relaxed))
    {
        return (double)atomic_load_explicit(&model->producer_clock_sample, memory_order_relaxed);
    }

    const uint64_t anchor_ns =
        atomic_load_explicit(&model->producer_clock_ns, memory_order_acquire);
    const uint64_t sample =
        atomic_load_explicit(&model->producer_clock_sample, memory_order_relaxed);
    const uint64_t now_ns = dvz_time_monotonic_ns();
    if (anchor_ns == 0u || now_ns <= anchor_ns)
        return (double)sample;
    return (double)sample + (double)(now_ns - anchor_ns) * (double)model->config.sample_rate_hz /
                                (double)NS_PER_SECOND;
}

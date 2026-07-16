/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

#pragma once



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DAQ_MAX_CHANNELS         128u
#define DAQ_MAX_BLOCK_SAMPLES    64u
#define DAQ_BLOCK_QUEUE_CAPACITY 32u
#define DAQ_CHANNEL_NAME_SIZE    24u



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum DaqChannelKind
{
    DAQ_CHANNEL_DIGITAL = 0,
    DAQ_CHANNEL_ANALOG = 1,
} DaqChannelKind;


typedef enum DaqEventKind
{
    DAQ_EVENT_NONE = 0,
    DAQ_EVENT_STIMULUS = 1,
    DAQ_EVENT_REWARD = 2,
    DAQ_EVENT_SYNC = 3,
} DaqEventKind;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct DaqConfig
{
    uint32_t channel_count;
    uint32_t analog_channel_count;
    uint32_t sample_rate_hz;
    uint32_t display_sample_count;
    uint32_t block_size;
    uint32_t seed;
} DaqConfig;


typedef struct DaqChannel
{
    DaqChannelKind kind;
    char name[DAQ_CHANNEL_NAME_SIZE];
} DaqChannel;


typedef struct DaqBlock
{
    uint64_t first_sample;
    uint32_t sample_count;
    float values[DAQ_MAX_BLOCK_SAMPLES * DAQ_MAX_CHANNELS];
} DaqBlock;


typedef struct DaqDirtySpan
{
    uint32_t first_sample;
    uint32_t sample_count;
} DaqDirtySpan;


typedef struct DaqDirtyRanges
{
    bool full;
    uint32_t span_count;
    DaqDirtySpan spans[2];
    uint32_t advanced_sample_count;
} DaqDirtyRanges;


typedef struct DaqStats
{
    uint64_t generated_sample_count;
    uint64_t consumed_sample_count;
    uint64_t dropped_sample_count;
    uint64_t overrun_block_count;
    uint64_t wrap_count;
    uint32_t queue_depth;
} DaqStats;


typedef struct DaqModel
{
    DaqConfig config;
    DaqChannel channels[DAQ_MAX_CHANNELS];

    float* display_values;
    bool* display_valid;
    uint8_t* display_events;
    uint32_t write_index;
    uint64_t next_expected_sample;
    uint64_t wrap_count;

    DaqBlock queue[DAQ_BLOCK_QUEUE_CAPACITY];
    atomic_uint queue_head;
    atomic_uint queue_tail;
    atomic_bool producer_running;
    atomic_bool producer_paused;
    atomic_uint producer_block_size;
    atomic_uint producer_dropout_permille;
    atomic_uint producer_noise_permille;
    atomic_uint producer_spike_rate_permille;
    atomic_uint producer_spike_amplitude_permille;
    atomic_uint producer_synchrony_permille;
    atomic_ullong producer_clock_sample;
    atomic_ullong producer_clock_ns;
    atomic_ullong generated_sample_count;
    atomic_ullong dropped_sample_count;
    atomic_ullong overrun_block_count;

    pthread_t producer_thread;
    bool producer_started;
    uint64_t producer_next_sample;
} DaqModel;



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Return the default synthetic DAQ configuration.
 *
 * @return default configuration
 */
DaqConfig daq_config_default(void);


/**
 * Initialize a deterministic DAQ model and its fixed-size display ring.
 *
 * @param model model to initialize
 * @param config model configuration
 * @return whether initialization succeeded
 */
bool daq_model_init(DaqModel* model, const DaqConfig* config);


/**
 * Release model-owned storage after stopping the producer.
 *
 * @param model model to destroy
 */
void daq_model_destroy(DaqModel* model);


/**
 * Fill the display ring with deterministic history before presentation.
 *
 * @param model initialized model
 * @return whether the complete history was generated
 */
bool daq_model_prefill(DaqModel* model);


/**
 * Start the wall-clock-paced acquisition producer.
 *
 * @param model initialized model
 * @return whether the producer started
 */
bool daq_model_start(DaqModel* model);


/**
 * Stop and join the acquisition producer when it is active.
 *
 * @param model model owning the producer
 */
void daq_model_stop(DaqModel* model);


/**
 * Pause or resume wall-clock acquisition without accumulating catch-up work.
 *
 * @param model model owning the producer
 * @param paused whether acquisition should pause
 */
void daq_model_set_paused(DaqModel* model, bool paused);


/**
 * Set the producer block size for later acquisition blocks.
 *
 * @param model model owning the producer
 * @param block_size requested sample count per block
 */
void daq_model_set_block_size(DaqModel* model, uint32_t block_size);


/**
 * Set deterministic producer dropout frequency.
 *
 * @param model model owning the producer
 * @param dropout_permille requested dropped blocks per thousand
 */
void daq_model_set_dropout(DaqModel* model, uint32_t dropout_permille);


/**
 * Set synthetic noise amplitude.
 *
 * @param model model owning the producer
 * @param noise_permille noise amplitude in thousandths of the default scale
 */
void daq_model_set_noise(DaqModel* model, uint32_t noise_permille);


/**
 * Set the synthetic unit firing-rate multiplier.
 *
 * @param model model owning the producer
 * @param rate_permille rate multiplier in thousandths
 */
void daq_model_set_spike_rate(DaqModel* model, uint32_t rate_permille);


/**
 * Set the synthetic extracellular spike amplitude.
 *
 * @param model model owning the producer
 * @param amplitude_permille amplitude multiplier in thousandths
 */
void daq_model_set_spike_amplitude(DaqModel* model, uint32_t amplitude_permille);


/**
 * Set the amplitude of population-synchronous activity.
 *
 * @param model model owning the producer
 * @param synchrony_permille synchrony multiplier in thousandths
 */
void daq_model_set_synchrony(DaqModel* model, uint32_t synchrony_permille);


/**
 * Drain bounded producer blocks into the display ring.
 *
 * @param model model receiving queued blocks
 * @param max_blocks maximum block count to consume, or zero for all available blocks
 * @param out_dirty physical display ranges changed by the drain
 * @return consumed block count
 */
uint32_t daq_model_drain(DaqModel* model, uint32_t max_blocks, DaqDirtyRanges* out_dirty);


/**
 * Advance the deterministic non-threaded source by an exact number of samples.
 *
 * @param model model to advance
 * @param sample_count logical sample count to generate and consume
 * @param out_dirty physical display ranges changed by the advance
 * @return whether the requested samples were generated
 */
bool daq_model_advance(DaqModel* model, uint32_t sample_count, DaqDirtyRanges* out_dirty);


/**
 * Reset the display ring and deterministic acquisition position.
 *
 * @param model model to reset
 * @return whether a fresh history was generated
 */
bool daq_model_reset(DaqModel* model);


/**
 * Copy current producer and display statistics.
 *
 * @param model source model
 * @param out output statistics
 */
void daq_model_stats(const DaqModel* model, DaqStats* out);


/**
 * Return the wall-clock-interpolated logical hardware cursor sample.
 *
 * @param model source model
 * @return fractional logical sample position
 */
double daq_model_cursor_sample(const DaqModel* model);

/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Stream tests                                                                                 */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "test_stream.h"

#include <string.h>

#include "_alloc.h"
#include "_assertions.h"
#include "_log.h"
#include "datoviz/stream.h"
#include "datoviz/video.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct StreamMockSinkState
{
    int create_count;
    int start_count;
    int submit_count;
    int update_count;
    int stop_count;
    int destroy_count;
    int create_rc;
    int start_rc;
    int restart_rc;
    int fail_start_on_count;
    int submit_rc;
    int stop_rc;
} StreamMockSinkState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _stream_mock_probe(const void* config)
{
    return config != NULL;
}



static bool _stream_mock_probe_unavailable(const void* config)
{
    (void)config;
    return false;
}



static int _stream_mock_create(DvzStreamSink* sink, const void* config)
{
    ANN(sink);
    ANN(config);
    StreamMockSinkState* state = NULL;
    dvz_memcpy(&state, sizeof(state), &config, sizeof(config));
    ANN(state);
    sink->backend_data = state;
    state->create_count++;
    return state->create_rc;
}



static int _stream_mock_start(DvzStreamSink* sink, const DvzStreamFrame* frame)
{
    ANN(sink);
    ANN(frame);
    StreamMockSinkState* state = (StreamMockSinkState*)sink->backend_data;
    ANN(state);
    state->start_count++;
    if (state->fail_start_on_count > 0 && state->start_count == state->fail_start_on_count)
    {
        return state->restart_rc != 0 ? state->restart_rc : -1;
    }
    return state->start_rc;
}



static int _stream_mock_submit(DvzStreamSink* sink, uint64_t wait_value)
{
    ANN(sink);
    (void)wait_value;
    StreamMockSinkState* state = (StreamMockSinkState*)sink->backend_data;
    ANN(state);
    state->submit_count++;
    return state->submit_rc;
}



static int _stream_mock_update(DvzStreamSink* sink, const DvzStreamFrame* frame)
{
    ANN(sink);
    ANN(frame);
    StreamMockSinkState* state = (StreamMockSinkState*)sink->backend_data;
    ANN(state);
    state->update_count++;
    return 0;
}



static int _stream_mock_stop(DvzStreamSink* sink)
{
    ANN(sink);
    StreamMockSinkState* state = (StreamMockSinkState*)sink->backend_data;
    ANN(state);
    state->stop_count++;
    return state->stop_rc;
}



static void _stream_mock_destroy(DvzStreamSink* sink)
{
    ANN(sink);
    StreamMockSinkState* state = (StreamMockSinkState*)sink->backend_data;
    if (state != NULL)
    {
        state->destroy_count++;
    }
    sink->backend_data = NULL;
}



/**
 * Return whether captured logs contain a message fragment.
 *
 * @param suite test suite that owns the captured logs
 * @param needle message fragment to search for
 * @return true when at least one captured log contains the fragment
 */
static bool _stream_log_contains(TstContext* suite, const char* needle)
{
    ANN(suite);
    ANN(needle);
    uint32_t count = tst_log_capture_count(suite);
    for (uint32_t i = 0; i < count; ++i)
    {
        const TstLogRecord* rec = tst_log_capture_get(suite, i);
        if (rec != NULL && strstr(rec->message, needle) != NULL)
        {
            return true;
        }
    }
    return false;
}



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_stream_attach_video(TstContext* suite, const TstCase* item)
{
    (void)item;
    ANN(suite);

    DvzStreamSinkRegistry* registry = dvz_stream_sink_registry_create();
    ANN(registry);

    DvzStreamConfig cfg = dvz_stream_config();
    dvz_stream_sink_registry_register(registry, dvz_stream_sink_video());
    DvzStream* stream = dvz_stream_create(NULL, registry, &cfg);
    AT(stream != NULL);

    DvzVideoSinkConfig vc = dvz_video_sink_config();
    AT(dvz_stream_attach_sink(stream, dvz_stream_sink_video(), &vc) == 0);

    dvz_stream_destroy(stream);
    dvz_stream_sink_registry_destroy(registry);
    return 0;
}



int test_stream_start_rollback_on_sink_failure(TstContext* suite, const TstCase* item)
{
    (void)item;
    ANN(suite);

    DvzStreamSinkRegistry* registry = dvz_stream_sink_registry_create();
    ANN(registry);

    DvzStreamConfig cfg = dvz_stream_config();
    DvzStream* stream = dvz_stream_create(NULL, registry, &cfg);
    AT(stream != NULL);

    StreamMockSinkState ok_state = {0};
    StreamMockSinkState fail_state = {0};
    fail_state.start_rc = -7;

    DvzStreamSinkBackend sink_ok = {
        .name = "mock_ok",
        .probe = _stream_mock_probe,
        .create = _stream_mock_create,
        .start = _stream_mock_start,
        .submit = _stream_mock_submit,
        .stop = _stream_mock_stop,
        .update = _stream_mock_update,
        .destroy = _stream_mock_destroy,
    };
    DvzStreamSinkBackend sink_fail = {
        .name = "mock_fail",
        .probe = _stream_mock_probe,
        .create = _stream_mock_create,
        .start = _stream_mock_start,
        .submit = _stream_mock_submit,
        .stop = _stream_mock_stop,
        .update = _stream_mock_update,
        .destroy = _stream_mock_destroy,
    };

    AT(dvz_stream_attach_sink(stream, &sink_ok, &ok_state) == 0);
    AT(dvz_stream_attach_sink(stream, &sink_fail, &fail_state) == 0);

    tst_log_capture_begin(suite);

    DvzStreamFrame frame = {0};
    tst_expect_error_begin(suite);
    AT(dvz_stream_start(stream, &frame) != 0);
    AT(tst_expect_error_end(suite) == 0);
    AT(tst_log_capture_count(suite) > 0);
    AT(_stream_log_contains(suite, "failed to start"));

    AT(ok_state.create_count == 1);
    AT(ok_state.start_count == 1);
    AT(ok_state.stop_count == 1);
    AT(fail_state.create_count == 1);
    AT(fail_state.start_count == 1);
    AT(fail_state.stop_count == 0);

    tst_expect_error_begin(suite);
    AT(dvz_stream_submit(stream, 1) != 0);
    AT(tst_expect_error_end(suite) == 0);
    AT(_stream_log_contains(suite, "not started"));

    dvz_stream_destroy(stream);

    AT(ok_state.stop_count == 1);
    AT(ok_state.destroy_count == 1);
    AT(fail_state.destroy_count == 1);

    tst_log_capture_end(suite);
    dvz_stream_sink_registry_destroy(registry);
    return 0;
}



int test_stream_attach_rolls_back_failed_create(TstContext* suite, const TstCase* item)
{
    (void)item;
    ANN(suite);

    DvzStreamSinkRegistry* registry = dvz_stream_sink_registry_create();
    ANN(registry);

    DvzStreamConfig cfg = dvz_stream_config();
    DvzStream* stream = dvz_stream_create(NULL, registry, &cfg);
    ANN(stream);

    StreamMockSinkState state = {.create_rc = -7};
    DvzStreamSinkBackend backend = {
        .name = "mock_create_failure",
        .probe = _stream_mock_probe,
        .create = _stream_mock_create,
        .destroy = _stream_mock_destroy,
    };

    tst_expect_error_begin(suite);
    AT(dvz_stream_attach_sink(stream, &backend, &state) == -1);
    AT(tst_expect_error_end(suite) == 0);
    AT(state.create_count == 1);
    AT(state.destroy_count == 1);

    dvz_stream_destroy(stream);
    AT(state.destroy_count == 1);
    dvz_stream_sink_registry_destroy(registry);
    return 0;
}



int test_stream_submit_returns_first_error(TstContext* suite, const TstCase* item)
{
    (void)item;
    ANN(suite);

    DvzStreamSinkRegistry* registry = dvz_stream_sink_registry_create();
    ANN(registry);

    DvzStreamConfig cfg = dvz_stream_config();
    DvzStream* stream = dvz_stream_create(NULL, registry, &cfg);
    AT(stream != NULL);

    StreamMockSinkState sink0_state = {0};
    StreamMockSinkState sink1_state = {0};
    StreamMockSinkState sink2_state = {0};
    sink1_state.submit_rc = -2;
    sink2_state.submit_rc = -3;

    DvzStreamSinkBackend sink0 = {
        .name = "mock_submit_0",
        .probe = _stream_mock_probe,
        .create = _stream_mock_create,
        .start = _stream_mock_start,
        .submit = _stream_mock_submit,
        .stop = _stream_mock_stop,
        .update = _stream_mock_update,
        .destroy = _stream_mock_destroy,
    };
    DvzStreamSinkBackend sink1 = {
        .name = "mock_submit_1",
        .probe = _stream_mock_probe,
        .create = _stream_mock_create,
        .start = _stream_mock_start,
        .submit = _stream_mock_submit,
        .stop = _stream_mock_stop,
        .update = _stream_mock_update,
        .destroy = _stream_mock_destroy,
    };
    DvzStreamSinkBackend sink2 = {
        .name = "mock_submit_2",
        .probe = _stream_mock_probe,
        .create = _stream_mock_create,
        .start = _stream_mock_start,
        .submit = _stream_mock_submit,
        .stop = _stream_mock_stop,
        .update = _stream_mock_update,
        .destroy = _stream_mock_destroy,
    };

    AT(dvz_stream_attach_sink(stream, &sink0, &sink0_state) == 0);
    AT(dvz_stream_attach_sink(stream, &sink1, &sink1_state) == 0);
    AT(dvz_stream_attach_sink(stream, &sink2, &sink2_state) == 0);

    DvzStreamFrame frame = {0};
    AT(dvz_stream_start(stream, &frame) == 0);

    tst_log_capture_begin(suite);
    int rc = dvz_stream_submit(stream, 1);
    AT(rc == -2);
    AT(tst_log_capture_count(suite) == 0);
    AT(sink0_state.submit_count == 1);
    AT(sink1_state.submit_count == 1);
    AT(sink2_state.submit_count == 1);

    tst_log_capture_end(suite);
    dvz_stream_destroy(stream);
    dvz_stream_sink_registry_destroy(registry);
    return 0;
}



int test_stream_update_restart_failure_stops_stream(TstContext* suite, const TstCase* item)
{
    (void)item;
    ANN(suite);

    DvzStreamSinkRegistry* registry = dvz_stream_sink_registry_create();
    ANN(registry);

    DvzStreamConfig cfg = dvz_stream_config();
    DvzStream* stream = dvz_stream_create(NULL, registry, &cfg);
    AT(stream != NULL);

    StreamMockSinkState ok_state = {0};
    StreamMockSinkState fail_state = {
        .restart_rc = -9,
        .fail_start_on_count = 2,
    };

    DvzStreamSinkBackend sink_ok = {
        .name = "mock_restart_ok",
        .probe = _stream_mock_probe,
        .create = _stream_mock_create,
        .start = _stream_mock_start,
        .submit = _stream_mock_submit,
        .stop = _stream_mock_stop,
        .update = NULL,
        .destroy = _stream_mock_destroy,
    };
    DvzStreamSinkBackend sink_fail = {
        .name = "mock_restart_fail",
        .probe = _stream_mock_probe,
        .create = _stream_mock_create,
        .start = _stream_mock_start,
        .submit = _stream_mock_submit,
        .stop = _stream_mock_stop,
        .update = NULL,
        .destroy = _stream_mock_destroy,
    };

    AT(dvz_stream_attach_sink(stream, &sink_ok, &ok_state) == 0);
    AT(dvz_stream_attach_sink(stream, &sink_fail, &fail_state) == 0);

    DvzStreamFrame frame0 = {0};
    DvzStreamFrame frame1 = {.extent = {.width = 1, .height = 1}};
    AT(dvz_stream_start(stream, &frame0) == 0);
    AT(ok_state.start_count == 1);
    AT(fail_state.start_count == 1);

    tst_log_capture_begin(suite);
    tst_expect_error_begin(suite);
    AT(dvz_stream_update(stream, &frame1) == -1);
    AT(tst_expect_error_end(suite) == 0);
    AT(_stream_log_contains(suite, "failed to restart sink"));

    AT(ok_state.start_count == 2);
    AT(ok_state.stop_count == 2);
    AT(fail_state.start_count == 2);
    AT(fail_state.stop_count == 1);

    tst_expect_error_begin(suite);
    AT(dvz_stream_submit(stream, 7) != 0);
    AT(tst_expect_error_end(suite) == 0);
    AT(_stream_log_contains(suite, "not started"));

    AT(dvz_stream_start(stream, &frame1) == 0);
    AT(ok_state.start_count == 3);
    AT(fail_state.start_count == 3);
    AT(dvz_stream_submit(stream, 8) == 0);

    tst_log_capture_end(suite);
    dvz_stream_destroy(stream);
    dvz_stream_sink_registry_destroy(registry);
    return 0;
}



int test_stream_stop_returns_first_error(TstContext* suite, const TstCase* item)
{
    (void)item;
    ANN(suite);

    DvzStreamSinkRegistry* registry = dvz_stream_sink_registry_create();
    ANN(registry);
    DvzStream* stream = dvz_stream_create(NULL, registry, NULL);
    ANN(stream);

    StreamMockSinkState first = {.stop_rc = -7};
    StreamMockSinkState second = {.stop_rc = -9};
    DvzStreamSinkBackend backend = {
        .name = "mock_stop_error",
        .probe = _stream_mock_probe,
        .create = _stream_mock_create,
        .start = _stream_mock_start,
        .submit = _stream_mock_submit,
        .stop = _stream_mock_stop,
        .destroy = _stream_mock_destroy,
    };
    AT(dvz_stream_attach_sink(stream, &backend, &first) == 0);
    AT(dvz_stream_attach_sink(stream, &backend, &second) == 0);
    DvzStreamFrame frame = {0};
    AT(dvz_stream_start(stream, &frame) == 0);
    AT(dvz_stream_stop(stream) == -7);
    AT(first.stop_count == 1);
    AT(second.stop_count == 1);
    AT(dvz_stream_stop(stream) == 0);

    dvz_stream_destroy(stream);
    dvz_stream_sink_registry_destroy(registry);
    return 0;
}



int test_stream_attach_sink_name_prefers_requested_then_auto(TstContext* suite, const TstCase* item)
{
    (void)item;
    ANN(suite);

    DvzStreamSinkRegistry* registry = dvz_stream_sink_registry_create();
    ANN(registry);

    DvzStreamConfig cfg = dvz_stream_config();
    DvzStream* stream = dvz_stream_create(NULL, registry, &cfg);
    AT(stream != NULL);

    StreamMockSinkState primary_state = {0};
    StreamMockSinkState fallback_state = {0};

    DvzStreamSinkBackend unavailable = {
        .name = "named_unavailable",
        .probe = _stream_mock_probe_unavailable,
        .create = _stream_mock_create,
        .start = _stream_mock_start,
        .submit = _stream_mock_submit,
        .stop = _stream_mock_stop,
        .update = _stream_mock_update,
        .destroy = _stream_mock_destroy,
    };
    DvzStreamSinkBackend fallback = {
        .name = "fallback_auto",
        .probe = _stream_mock_probe,
        .create = _stream_mock_create,
        .start = _stream_mock_start,
        .submit = _stream_mock_submit,
        .stop = _stream_mock_stop,
        .update = _stream_mock_update,
        .destroy = _stream_mock_destroy,
    };

    dvz_stream_sink_registry_register(registry, &unavailable);
    dvz_stream_sink_registry_register(registry, &fallback);

    AT(dvz_stream_sink_registry_find(registry, "named_unavailable") == &unavailable);
    AT(dvz_stream_sink_registry_find(registry, "fallback_auto") == &fallback);

    AT(dvz_stream_attach_sink_name(stream, "fallback_auto", &primary_state) == 0);
    tst_log_capture_begin(suite);
    AT_EXPECTED_LOG_STRICT(
        suite, LOG_WARN, dvz_stream_attach_sink_name(stream, "named_unavailable", &fallback_state) == 0);
    AT(_stream_log_contains(suite, "falling back to auto"));
    tst_log_capture_end(suite);

    DvzStreamFrame frame = {0};
    AT(dvz_stream_start(stream, &frame) == 0);
    AT(primary_state.create_count == 1);
    AT(primary_state.start_count == 1);
    AT(fallback_state.create_count == 1);
    AT(fallback_state.start_count == 1);

    dvz_stream_destroy(stream);
    dvz_stream_sink_registry_destroy(registry);
    return 0;
}



int test_stream(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "stream";
    TST_MODULE(suite, tags);
    TST_CASE(test_stream_attach_video);
    TST_CASE(test_stream_attach_rolls_back_failed_create);
    TST_CASE(test_stream_start_rollback_on_sink_failure);
    TST_CASE(test_stream_submit_returns_first_error);
    TST_CASE(test_stream_update_restart_failure_stops_stream);
    TST_CASE(test_stream_stop_returns_first_error);
    TST_CASE(test_stream_attach_sink_name_prefers_requested_then_auto);
    return 0;
}

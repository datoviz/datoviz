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

#include "_alloc.h"
#include "_assertions.h"
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
    int stop_count;
    int destroy_count;
    int start_rc;
    int submit_rc;
} StreamMockSinkState;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static bool _stream_mock_probe(const void* config)
{
    return config != NULL;
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
    return 0;
}



static int _stream_mock_start(DvzStreamSink* sink, const DvzStreamFrame* frame)
{
    ANN(sink);
    ANN(frame);
    StreamMockSinkState* state = (StreamMockSinkState*)sink->backend_data;
    ANN(state);
    state->start_count++;
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



static int _stream_mock_stop(DvzStreamSink* sink)
{
    ANN(sink);
    StreamMockSinkState* state = (StreamMockSinkState*)sink->backend_data;
    ANN(state);
    state->stop_count++;
    return 0;
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



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_stream_attach_video(TstSuite* suite, TstItem* item)
{
    (void)item;
    ANN(suite);

    DvzStreamSinkRegistry* registry = dvz_stream_sink_registry_create();
    ANN(registry);

    DvzStreamConfig cfg = dvz_stream_default_config();
    dvz_stream_sink_registry_register(registry, dvz_stream_sink_video());
    DvzStream* stream = dvz_stream_create(NULL, registry, &cfg);
    AT(stream != NULL);

    DvzVideoSinkConfig vc = dvz_video_sink_default_config();
    AT(dvz_stream_attach_sink(stream, dvz_stream_sink_video(), &vc) == 0);

    dvz_stream_destroy(stream);
    dvz_stream_sink_registry_destroy(registry);
    return 0;
}



int test_stream_start_rollback_on_sink_failure(TstSuite* suite, TstItem* item)
{
    (void)item;
    ANN(suite);

    DvzStreamSinkRegistry* registry = dvz_stream_sink_registry_create();
    ANN(registry);

    DvzStreamConfig cfg = dvz_stream_default_config();
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
        .update = NULL,
        .destroy = _stream_mock_destroy,
    };
    DvzStreamSinkBackend sink_fail = {
        .name = "mock_fail",
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

    DvzStreamFrame frame = {0};
    AT(dvz_stream_start(stream, &frame) != 0);

    AT(ok_state.create_count == 1);
    AT(ok_state.start_count == 1);
    AT(ok_state.stop_count == 1);
    AT(fail_state.create_count == 1);
    AT(fail_state.start_count == 1);
    AT(fail_state.stop_count == 0);

    AT(dvz_stream_submit(stream, 1) != 0);

    dvz_stream_destroy(stream);

    AT(ok_state.stop_count == 1);
    AT(ok_state.destroy_count == 1);
    AT(fail_state.destroy_count == 1);

    dvz_stream_sink_registry_destroy(registry);
    return 0;
}



int test_stream_submit_returns_first_error(TstSuite* suite, TstItem* item)
{
    (void)item;
    ANN(suite);

    DvzStreamSinkRegistry* registry = dvz_stream_sink_registry_create();
    ANN(registry);

    DvzStreamConfig cfg = dvz_stream_default_config();
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
        .update = NULL,
        .destroy = _stream_mock_destroy,
    };
    DvzStreamSinkBackend sink1 = {
        .name = "mock_submit_1",
        .probe = _stream_mock_probe,
        .create = _stream_mock_create,
        .start = _stream_mock_start,
        .submit = _stream_mock_submit,
        .stop = _stream_mock_stop,
        .update = NULL,
        .destroy = _stream_mock_destroy,
    };
    DvzStreamSinkBackend sink2 = {
        .name = "mock_submit_2",
        .probe = _stream_mock_probe,
        .create = _stream_mock_create,
        .start = _stream_mock_start,
        .submit = _stream_mock_submit,
        .stop = _stream_mock_stop,
        .update = NULL,
        .destroy = _stream_mock_destroy,
    };

    AT(dvz_stream_attach_sink(stream, &sink0, &sink0_state) == 0);
    AT(dvz_stream_attach_sink(stream, &sink1, &sink1_state) == 0);
    AT(dvz_stream_attach_sink(stream, &sink2, &sink2_state) == 0);

    DvzStreamFrame frame = {0};
    AT(dvz_stream_start(stream, &frame) == 0);

    int rc = dvz_stream_submit(stream, 1);
    AT(rc == -2);
    AT(sink0_state.submit_count == 1);
    AT(sink1_state.submit_count == 1);
    AT(sink2_state.submit_count == 1);

    dvz_stream_destroy(stream);
    dvz_stream_sink_registry_destroy(registry);
    return 0;
}



int test_stream(TstSuite* suite)
{
    ANN(suite);
    const char* tags = "stream";
    TEST_SIMPLE(test_stream_attach_video);
    TEST_SIMPLE(test_stream_start_rollback_on_sink_failure);
    TEST_SIMPLE(test_stream_submit_returns_first_error);
    return 0;
}

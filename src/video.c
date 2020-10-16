/*
 * Copyright (c) 2003 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "video.h"
#include "log.h"

#define NUM_THREADS 8

#if HAS_FFMPEG

#include <libavformat/avformat.h>
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libavutil/timestamp.h>
#include <libswscale/swscale.h>

// Static util functions.

static int
write_frame(AVFormatContext* fmt_ctx, const AVRational* time_base, AVStream* st, AVPacket* pkt)
{
    av_packet_rescale_ts(pkt, *time_base, st->time_base);
    pkt->stream_index = st->index;
    return av_interleaved_write_frame(fmt_ctx, pkt);
}

static void add_stream(
    OutputStream* ost, AVFormatContext* oc, AVCodec** codec, enum AVCodecID codec_id, int width,
    int height, int fps, int bitrate)
{
    AVCodecContext* c;

    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec))
    {
        fprintf(stderr, "Could not find encoder for '%s'\n", avcodec_get_name(codec_id));
    }

    ost->st = avformat_new_stream(oc, NULL);
    if (!ost->st)
    {
        fprintf(stderr, "Could not allocate stream\n");
    }
    ost->st->id = (int)oc->nb_streams - 1;
    c = avcodec_alloc_context3(*codec);
    if (!c)
    {
        fprintf(stderr, "Could not alloc an encoding context\n");
    }
    ost->enc = c;

    assert((*codec)->type == AVMEDIA_TYPE_VIDEO);
    c->codec_id = codec_id;

    c->bit_rate = bitrate;
    /* Resolution must be a multiple of two. */
    c->width = width;
    c->height = height;
    /* timebase: This is the fundamental unit of time (in seconds) in terms
     * of which frame timestamps are represented. For fixed-fps content,
     * timebase should be 1/framerate and timestamp increments should be
     * identical to 1. */
    ost->st->time_base = (AVRational){1, fps};
    c->time_base = ost->st->time_base;

    c->gop_size = 12; /* emit one intra frame every twelve frames at most */
    c->pix_fmt = AV_PIX_FMT_YUV420P;

    // TODO: the current implementation is serial and makes poor use of multicore CPUs.
    // What should be done is to separate the send frame/write frame logic.
    c->thread_count = NUM_THREADS;
    c->thread_type = FF_THREAD_FRAME;

    if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO)
    {
        /* just for testing, we also add B-frames */
        c->max_b_frames = 2;
    }
    if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO)
    {
        /* Needed to avoid using macroblocks in which some coeffs overflow.
         * This does not happen with normal video, it just happens here as
         * the motion of the chroma plane does not match the luma plane. */
        c->mb_decision = 2;
    }

    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

static AVFrame* alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
{
    AVFrame* picture;
    int ret;

    picture = av_frame_alloc();
    if (!picture)
        return NULL;

    picture->format = pix_fmt;
    picture->width = width;
    picture->height = height;

    /* allocate the buffers for the frame data */
    ret = av_frame_get_buffer(picture, 32);
    if (ret < 0)
    {
        fprintf(stderr, "Could not allocate frame data.\n");
    }

    return picture;
}

static void
open_video(AVFormatContext* oc, AVCodec* codec, OutputStream* ost, AVDictionary* opt_arg)
{
    int ret;
    AVCodecContext* c = ost->enc;
    AVDictionary* opt = NULL;

    av_dict_copy(&opt, opt_arg, 0);

    /* open the codec */
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0)
    {
        fprintf(stderr, "Could not open video codec: %s\n", av_err2str(ret));
    }

    /* allocate and init a re-usable frame */
    ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
    printf("allocate frame %d x %d\n", c->width, c->height);
    if (!ost->frame)
    {
        fprintf(stderr, "Could not allocate video frame\n");
    }

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0)
    {
        fprintf(stderr, "Could not copy the stream parameters\n");
    }
}

static int write_video_frame(Video* video)
{
    /*
     * encode one video frame and send it to the muxer
     * return 1 when encoding is finished, 0 otherwise
     */
    OutputStream* ost = video->ost;
    AVFormatContext* oc = ost->oc;
    int ret;
    AVCodecContext* c;
    AVFrame* frame = video->frame;
    AVPacket pkt = {0};
    c = ost->enc;

    av_init_packet(&pkt);
    ret = avcodec_send_frame(c, frame);
    if (ret < 0)
    {
        fprintf(stderr, "Error sending a frame for encoding\n");
    }
    while (ret >= 0)
    {
        ret = avcodec_receive_packet(c, &pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            return (ret == AVERROR(EAGAIN)) ? 0 : 1;
        }
        else if (ret < 0)
        {
            fprintf(stderr, "Error during encoding\n");
        }
        ret = write_frame(oc, &c->time_base, ost->st, &pkt);
        if (ret < 0)
        {
            fprintf(stderr, "Error while writing video frame: %s\n", av_err2str(ret));
        }
        av_packet_unref(&pkt);
    }
    return (frame) ? 0 : 1;
}

static void close_stream(AVFormatContext* oc, OutputStream* ost)
{
    avcodec_free_context(&ost->enc);
    av_frame_free(&ost->frame);
    sws_freeContext(ost->sws_ctx);
}

// Public functions.

Video* create_video(const char* filename, int width, int height, int fps, int bitrate)
{
    OutputStream* ost = calloc(1, sizeof(OutputStream));
    AVOutputFormat* fmt = NULL;
    AVFormatContext* oc = NULL;
    AVCodec* video_codec = NULL;
    AVDictionary* opt = NULL;
    int ret = 0;

    /* allocate the output media context */
    avformat_alloc_output_context2(&oc, NULL, NULL, filename);
    if (!oc)
    {
        printf("Could not deduce output format from file extension: using MPEG.\n");
        avformat_alloc_output_context2(&oc, NULL, "mpeg", filename);
    }
    if (!oc)
        return NULL;

    fmt = oc->oformat;

    /* Add the audio and video streams using the default format codecs
     * and initialize the codecs. */
    if (fmt->video_codec != AV_CODEC_ID_NONE)
    {
        // fprintf(stdout, "fallback to default H264 codec\n");
        add_stream(ost, oc, &video_codec, AV_CODEC_ID_H264, width, height, fps, bitrate);
    }

    /* Now that all the parameters are set, we can open the audio and
     * video codecs and allocate the necessary encode buffers. */
    open_video(oc, video_codec, ost, opt);

    av_dump_format(oc, 0, filename, 1);

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0)
        {
            fprintf(stderr, "Could not open '%s': %s\n", filename, av_err2str(ret));
            return NULL;
        }
    }

    /* Write the stream header, if any. */
    ret = avformat_write_header(oc, &opt);
    if (ret < 0)
    {
        fprintf(stderr, "Error occurred when opening output file: %s\n", av_err2str(ret));
        return NULL;
    }

    Video* video = calloc(1, sizeof(Video));
    ost->oc = oc;
    video->ost = ost;
    video->fps = fps;
    video->width = width;
    video->height = height;

    return video;
}

void add_frame(Video* video, uint8_t* image)
{
    OutputStream* ost = video->ost;
    AVCodecContext* c = ost->enc;

    /* when we pass a frame to the encoder, it may keep a reference to it
     * internally; make sure we do not overwrite it here */
    if (av_frame_make_writable(ost->frame) < 0)
    {
        fprintf(stderr, "Could not write the frame\n");
    }

    if (!ost->sws_ctx)
    {
        ost->sws_ctx = sws_getContext(
            c->width, c->height, AV_PIX_FMT_RGBA, c->width, c->height, c->pix_fmt, SCALE_FLAGS,
            NULL, NULL, NULL);
        if (!ost->sws_ctx)
        {
            fprintf(stderr, "Could not initialize the conversion context\n");
        }
    }
    video->image = ost->frame->data[0];
    video->linesize = ost->frame->linesize[0];
    // RGB to YUV420P
    const uint8_t* inData[1] = {image};
    int inLinesize[1] = {4 * c->width};
    sws_scale(
        ost->sws_ctx, inData, inLinesize, 0, c->height, ost->frame->data, ost->frame->linesize);

    ost->frame->pts = ost->next_pts++;
    video->frame = ost->frame;

    write_video_frame(video);
}

void end_video(Video* video)
{
    AVFormatContext* oc = video->ost->oc;

    /* Write the trailer, if any. The trailer must be written before you
     * close the CodecContexts open when you wrote the header; otherwise
     * av_write_trailer() may try to use memory that was freed on
     * av_codec_close(). */
    av_write_trailer(oc);

    /* Close each codec. */
    close_stream(oc, video->ost);

    if (!(oc->oformat->flags & AVFMT_NOFILE))
        /* Close the output file. */
        avio_closep(&oc->pb);

    /* free the stream */
    avformat_free_context(oc);

    FREE(video);

    return;
}

// No FFMPEG support
#else

Video* create_video(const char* filename, int width, int height, int fps, int bitrate)
{
    log_error("visky was not build with ffmpeg support");
    return NULL;
}

void add_frame(Video* video, uint8_t* image) {}

void end_video(Video* video) {}

#endif

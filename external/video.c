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
#include "../include/datoviz/common.h"
#include "../include/datoviz/log.h"

#define NUM_THREADS 8
#define INBUF_SIZE  4096

#if HAS_FFMPEG

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libavutil/timestamp.h>
#include <libswscale/swscale.h>



/*************************************************************************************************/
/*  Video writer                                                                                 */
/*************************************************************************************************/

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
Video* init_video(const char* filename, int width, int height, int fps, int bitrate)
{
    ASSERT(filename != NULL);
    ASSERT(strlen(filename) > 0);

    Video* video = calloc(1, sizeof(Video));
    video->filename = filename;
    video->fps = fps;
    video->bitrate = bitrate;
    video->width = width;
    video->height = height;
    return video;
}

void create_video(Video* video)
{
    ASSERT(video != NULL);
    ASSERT(video->filename != NULL);
    ASSERT(video->fps > 0);
    ASSERT(video->bitrate > 0);
    ASSERT(video->width > 0);
    ASSERT(video->height > 0);

    OutputStream* ost = calloc(1, sizeof(OutputStream));
    AVOutputFormat* fmt = NULL;
    AVFormatContext* oc = NULL;
    AVCodec* video_codec = NULL;
    AVDictionary* opt = NULL;
    int ret = 0;

    /* allocate the output media context */
    avformat_alloc_output_context2(&oc, NULL, NULL, video->filename);
    if (!oc)
    {
        printf("Could not deduce output format from file extension: using MPEG.\n");
        avformat_alloc_output_context2(&oc, NULL, "mpeg", video->filename);
    }
    if (!oc)
        return;

    fmt = oc->oformat;

    /* Add the audio and video streams using the default format codecs
     * and initialize the codecs. */
    if (fmt->video_codec != AV_CODEC_ID_NONE)
    {
        // fprintf(stdout, "fallback to default H264 codec\n");
        add_stream(
            ost, oc, &video_codec, AV_CODEC_ID_H264, //
            video->width, video->height, video->fps, video->bitrate);
    }

    /* Now that all the parameters are set, we can open the audio and
     * video codecs and allocate the necessary encode buffers. */
    open_video(oc, video_codec, ost, opt);

    av_dump_format(oc, 0, video->filename, 1);

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&oc->pb, video->filename, AVIO_FLAG_WRITE);
        if (ret < 0)
        {
            fprintf(stderr, "Could not open '%s': %s\n", video->filename, av_err2str(ret));
            return;
        }
    }

    /* Write the stream header, if any. */
    ret = avformat_write_header(oc, &opt);
    if (ret < 0)
    {
        fprintf(stderr, "Error occurred when opening output file: %s\n", av_err2str(ret));
        return;
    }

    ost->oc = oc;
    video->ost = ost;
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


/*************************************************************************************************/
/*  Video reader                                                                                 */
/*************************************************************************************************/

static void _decode(AVCodecContext* dec_ctx, AVFrame* frame, AVPacket* pkt, VideoCallback callback)
{
    int ret;

    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0)
    {
        fprintf(stderr, "Error sending a packet for decoding\n");
        exit(1);
    }

    while (ret >= 0)
    {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0)
        {
            log_error("Error during decoding");
            exit(1);
        }

        log_debug("decoding frame #%d", dec_ctx->frame_number);
        callback(
            dec_ctx->frame_number, frame->width, frame->height, frame->linesize[0],
            frame->data[0]);
    }
}



Video* read_video(const char* filename)
{
    ASSERT(filename != NULL);
    log_debug("open video %s for reading", filename);

    Video* video = calloc(1, sizeof(Video));
    video->filename = filename;

    const AVCodec* codec;
    AVCodecParserContext* parser;
    AVCodecContext* c = NULL;
    FILE* f;
    AVFrame* frame;
    AVPacket* pkt;

    pkt = av_packet_alloc();
    if (!pkt)
        exit(1);

    /* find the video decoder */
    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec)
    {
        log_error("Codec not found");
        exit(1);
    }

    parser = av_parser_init(codec->id);
    if (!parser)
    {
        log_error("parser not found");
        exit(1);
    }

    c = avcodec_alloc_context3(codec);
    if (!c)
    {
        log_error("Could not allocate video codec context");
        exit(1);
    }

    /* For some codecs, such as msmpeg4 and mpeg4, width and height
       MUST be initialized there because this information is not
       available in the bitstream. */

    /* open it */
    if (avcodec_open2(c, codec, NULL) < 0)
    {
        log_error("Could not open codec\n");
        exit(1);
    }

    f = fopen(filename, "rb");
    if (!f)
    {
        log_error("Could not open %s\n", filename);
        exit(1);
    }

    frame = av_frame_alloc();
    if (!frame)
    {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }

    // TODO: after first frame?
    // video->width = frame->width;
    // video->height = frame->height;

    video->frame = frame;

    video->reader.c = c;
    video->reader.f = f;
    video->reader.codec = codec;
    video->reader.parser = parser;
    video->reader.pkt = pkt;

    return video;
}



void read_frames(Video* video, VideoCallback callback)
{
    // TODO: only read part of the file
    // TODO: random seek in the file (hard!)

    ASSERT(video != NULL);
    uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t* data;
    size_t data_size;
    int ret;

    AVCodecContext* c = video->reader.c;
    // FILE* f = video->reader.f;
    // const AVCodec* codec = video->reader.codec;
    AVCodecParserContext* parser = video->reader.parser;
    AVPacket* pkt = video->reader.pkt;
    AVFrame* frame = video->frame;

    /* set end of buffer to 0 (this ensures that no overreading happens for damaged MPEG
     * streams)
     */
    memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    while (!feof(video->reader.f))
    {
        /* read raw data from the input file */
        data_size = fread(inbuf, 1, INBUF_SIZE, video->reader.f);
        if (!data_size)
            break;

        /* use the parser to split the data into frames */
        data = inbuf;
        while (data_size > 0)
        {
            ret = av_parser_parse2(
                parser, c, &pkt->data, &pkt->size, data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE,
                0);
            if (ret < 0)
            {
                log_error("Error while parsing");
                exit(1);
            }
            log_debug("read %d bytes, pkt size %d", ret, pkt->size);

            data += ret;
            data_size -= (size_t)ret;

            if (pkt->size)
                _decode(c, frame, pkt, callback);
        }
    }

    /* flush the decoder */
    _decode(c, frame, NULL, callback);
}



void close_video(Video* video)
{
    ASSERT(video != NULL);
    fclose(video->reader.f);

    av_parser_close(video->reader.parser);
    avcodec_free_context(&video->reader.c);
    av_frame_free(&video->frame);
    av_packet_free(&video->reader.pkt);
}



#else

/*************************************************************************************************/
/*  FFMPEG unavailable                                                                           */
/*************************************************************************************************/

Video* init_video(const char* filename, int width, int height, int fps, int bitrate)
{
    log_error("datoviz was not compiled with ffmpeg support, unable to record a video");
    return NULL;
}

void create_video(Video* video) { return; }

void add_frame(Video* video, uint8_t* image) {}

void end_video(Video* video) {}

Video* read_video(const char* filename)
{
    log_error("datoviz was not compiled with ffmpeg support, unable to read a video file");
    return NULL;
}

void read_frames(Video* video, VideoCallback callback) {}

void close_video(Video* video) {}



#endif

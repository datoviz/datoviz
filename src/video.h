#ifndef VKY_VIDEO_HEADER
#define VKY_VIDEO_HEADER

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCALE_FLAGS SWS_BICUBIC

typedef struct AVStream AVStream;
typedef struct AVCodecContext AVCodecContext;
typedef struct AVFormatContext AVFormatContext;
typedef struct AVFrame AVFrame;
typedef struct SwsContext SwsContext;

typedef struct OutputStream OutputStream;
struct OutputStream
{
    AVStream* st;
    AVCodecContext* enc;
    AVFormatContext* oc;

    /* pts of the next frame that will be generated */
    int64_t next_pts;
    int samples_count;

    AVFrame* frame;

    struct SwsContext* sws_ctx;
};

typedef struct Video Video;
struct Video
{
    OutputStream* ost;
    int width, height, fps;
    int linesize;
    uint8_t* image;
    AVFrame* frame;
};

Video* create_video(const char* filename, int width, int height, int fps, int bitrate);

void add_frame(Video* video, uint8_t* image);

void end_video(Video* video);

#endif

#ifndef DVZ_VIDEO_HEADER
#define DVZ_VIDEO_HEADER

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCALE_FLAGS SWS_BICUBIC


/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

// Forward declarations.
typedef struct AVStream AVStream;
typedef struct AVCodecContext AVCodecContext;
typedef struct AVFormatContext AVFormatContext;
typedef struct AVFrame AVFrame;
typedef struct SwsContext SwsContext;
typedef struct AVCodec AVCodec;
typedef struct AVFormatContext AVFormatContext;
typedef struct AVCodecParserContext AVCodecParserContext;
typedef struct AVPacket AVPacket;

typedef struct OutputStream OutputStream;
typedef struct VideoReader VideoReader;
typedef struct Video Video;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

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

struct VideoReader
{
    const AVCodec* codec;
    AVCodecParserContext* parser;
    AVCodecContext* c;
    FILE* f;
    AVPacket* pkt;
};

struct Video
{
    OutputStream* ost;
    VideoReader reader;
    const char* filename;
    int width, height, fps, bitrate;
    int linesize;
    uint8_t* image;
    AVFrame* frame;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

typedef void (*VideoCallback)(int, int, int, int, uint8_t*);

Video* init_video(const char* filename, int width, int height, int fps, int bitrate);

void create_video(Video* video);

void add_frame(Video* video, uint8_t* image);

void end_video(Video* video);



Video* read_video(const char* filename);

void read_frames(Video* video, VideoCallback callback);

void close_video(Video* video);



#endif

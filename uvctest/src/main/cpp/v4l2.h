
#ifndef V4L2_H
#define V4L2_H

#include <linux/videodev2.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int width;
    int height;
    unsigned int frame_rate;
    unsigned int pixel_format;
} video_fmt_t;

typedef struct {
    void* address;
    int size;
    int offset;
    int bytesused;
} frame_buffer_t;

static inline const char* v4l2_pixel_format_to_string(unsigned int pixel_format) {
    switch (pixel_format) {
    case V4L2_PIX_FMT_MJPEG:
        return "MJPG";
    case V4L2_PIX_FMT_H264:
        return "H264";
    case V4L2_PIX_FMT_HEVC:
        return "HEVC";
    case V4L2_PIX_FMT_GREY:
        return "GREY";
    case V4L2_PIX_FMT_YUYV:
        return "YUYV";
    case V4L2_PIX_FMT_YUV420:
        return "YUV420";
    default:
        return "UNKNOWN";
    }

    return "UNKNOWN";
}

static inline unsigned int v4l2_pixel_format_from_string(const char* pixel_format) {
    if (strcmp(pixel_format, "MJPG") == 0) {
        return V4L2_PIX_FMT_MJPEG;
    } else if (strcmp(pixel_format, "H264") == 0) {
        return V4L2_PIX_FMT_H264;
    } else if (strcmp(pixel_format, "HEVC") == 0) {
       return V4L2_PIX_FMT_HEVC;
    } else if (strcmp(pixel_format, "GREY") == 0) {
        return V4L2_PIX_FMT_GREY;
    } else if (strcmp(pixel_format, "YUYV") == 0) {
        return V4L2_PIX_FMT_YUYV;
    } else if (strcmp(pixel_format, "YUV420") == 0) {
        return V4L2_PIX_FMT_YUV420;
    } else {
        return 0;
    }
}

int v4l2_open(const char *device);
void v4l2_close(int fd);
int v4l2_config_stream(int fd, video_fmt_t* fmt);
int v4l2_allocate_buffers(frame_buffer_t** buffers_out, int fd, int count);
int v4l2_free_buffers(frame_buffer_t* buffers, int fd, int count);
int v4l2_stream_on(int fd, int buffer_count);
int v4l2_stream_off(int fd);
int v4l2_buffer_dequeue(frame_buffer_t* buffers, int fd, int buffer_count);
int v4l2_buffer_enqueue(int fd, int buffer_index);

#ifdef __cplusplus
}
#endif

#endif  // V4L2_H
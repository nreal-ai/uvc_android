
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

#include "v4l2.h"


int v4l2_open(const char *device) {
    int fd = open(device, O_RDWR);
    if (fd < 0) {
        perror("v4l2_open");
        return -1;
    }
    return fd;
}

void v4l2_close(int fd) {
    if (fd >= 0) close(fd);
}

int v4l2_config_stream(int fd, video_fmt_t *fmt) {
    struct v4l2_format format = { 0 };
    struct v4l2_capability cap = { 0 };
    struct v4l2_streamparm stream = { 0 };

    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
        fprintf(stderr, "VIDIOC_QUERYCAP failed: %s", strerror(errno));
        return -1;
    }
    if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0 &&
        (cap.capabilities & V4L2_CAP_STREAMING) == 0) {
        fprintf(stderr, "Not a video capture device\n");
        return -1;
    }

    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = fmt->width;
    format.fmt.pix.height = fmt->height;
    format.fmt.pix.pixelformat = fmt->pixel_format;
    format.fmt.pix.field = V4L2_FIELD_ANY;

    if (ioctl(fd, VIDIOC_S_FMT, &format) < 0) {
        fprintf(stderr, "VIDIOC_S_FMT failed: %s", strerror(errno));
        return -1;
    }

    if (fmt->frame_rate > 0) {
        stream.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        stream.parm.capture.timeperframe.numerator = 1;
        stream.parm.capture.timeperframe.denominator = fmt->frame_rate;
        if (ioctl(fd, VIDIOC_S_PARM, &stream) < 0) {
            fprintf(stderr, "VIDIOC_S_PARM failed: %s", strerror(errno));
            return -1;
        }
        printf("Requested frame rate %d fps\n", fmt->frame_rate);

        if (ioctl(fd, VIDIOC_G_PARM, &stream) < 0) {
            fprintf(stderr, "VIDIOC_G_PARM failed: %s", strerror(errno));
            return -1;
        }
        printf("Actual frame rate: %d/%d fps\n",
               stream.parm.capture.timeperframe.denominator,
               stream.parm.capture.timeperframe.numerator);
    }

    return 0;
}

int v4l2_allocate_buffers(frame_buffer_t** buffers_out, int fd, int count) {
    struct v4l2_requestbuffers req_bufs = { 0 };
    struct v4l2_buffer buf = { 0 };

    if (buffers_out == NULL) {
        fprintf(stderr, "buffers_out is NULL");
        return -1;
    }

    *buffers_out = NULL;

    frame_buffer_t* buffers = (frame_buffer_t*)malloc(sizeof(frame_buffer_t) * count);
    if (buffers == NULL) {
        fprintf(stderr, "malloc failed: %s", strerror(errno));
        return -1;
    }

    req_bufs.count = count;
    req_bufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req_bufs.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req_bufs) < 0) {
        fprintf(stderr, "VIDIOC_REQBUFS failed: %s", strerror(errno));
        return -1;
    }

    for (int i = 0; i < count; i++) {
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
            fprintf(stderr, "VIDIOC_QUERYBUF failed: %s", strerror(errno));
            free(buffers);
            return -1;
        }

        buffers[i].address = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
        if (buffers[i].address == MAP_FAILED) {
            fprintf(stderr, "mmap failed: %s", strerror(errno));
            free(buffers);
            return -1;
        }
        buffers[i].bytesused = 0;
        buffers[i].offset = buf.m.offset;
        buffers[i].size = buf.length;
    }

    *buffers_out = buffers;

    return 0;
}


int v4l2_free_buffers(frame_buffer_t* buffers, int fd, int count) {
    struct v4l2_requestbuffers req_bufs = { 0 };
    struct v4l2_buffer buf = { 0 };

    req_bufs.count = 0;
    req_bufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req_bufs.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req_bufs) < 0) {
        fprintf(stderr, "VIDIOC_REQBUFS failed: %s", strerror(errno));
        return -1;
    }

    if (buffers == NULL || count == 0) {
        return 0;
    }

    for (int i = 0; i < count; i++) {
        if (munmap(buffers[i].address, buffers[i].size) < 0) {
            fprintf(stderr, "munmap failed: %s", strerror(errno));
        }
    }

    free(buffers);

    return 0;
}

int v4l2_stream_on(int fd, int buffer_count) {
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    struct v4l2_buffer buf = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MMAP,
    };

    for (int i = 0; i < buffer_count; i++) {
        buf.index = i;
        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
            fprintf(stderr, "VIDIOC_QBUF failed: %s", strerror(errno));
            return -1;
        }
    }

    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        fprintf(stderr, "VIDIOC_STREAMON failed: %s", strerror(errno));
        return -1;
    }

    return 0;
}

int v4l2_stream_off(int fd) {
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0) {
        fprintf(stderr, "VIDIOC_STREAMOFF failed: %s", strerror(errno));
        return -1;
    }

    return 0;
}

int v4l2_buffer_dequeue(frame_buffer_t* buffers, int fd, int buffer_count) {
    struct v4l2_buffer buf = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MMAP,
    };

    if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
        fprintf(stderr, "VIDIOC_DQBUF failed: %s", strerror(errno));
        return -1;
    }

    if (buf.index >= buffer_count) {
        fprintf(stderr, "VIDIOC_DQBUF failed: invalid buffer index");
        return -1;
    }

    buffers[buf.index].bytesused = buf.bytesused;
    buffers[buf.index].offset = buf.m.offset;

    return buf.index;

}

int v4l2_buffer_enqueue(int fd, int index) {
    struct v4l2_buffer buf = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MMAP,
        .index = index,
    };

    if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
        fprintf(stderr, "VIDIOC_QBUF failed: %s", strerror(errno));
        return -1;
    }

    return 0;
}

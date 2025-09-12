#define _POSIX_C_SOURCE  200308L

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>

#include "v4l2.h"
#include "metadata.h"
#include "show_image_global.h"


#define CLOSE_FD(_fd) if (_fd >= 0) { close(_fd); _fd = -1; }

static volatile sig_atomic_t g_stream_off_flag = 0;

ShowImageHandle g_show_image_handle = NULL;  // 全局变量

void handle_signal(int signo) {
    g_stream_off_flag = 1;
}

// 长选项定义
static struct option long_options[] = {
        {"dev", required_argument, 0, 'd'},
        {"video_size", required_argument, 0, 's'},
        {"frame_rate", required_argument, 0, 'f'},
        {"pixel_format", required_argument, 0, 'p'},
        {"output_file", optional_argument, 0, 'o'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}  // 结束标志
};

void print_usage(const char *proc) {
    printf("Version: 1.1.3\n");
    printf("Usage: %s [OPTIONS]\n", proc);
    printf("  -d, --dev=/dev/videoX\n");
    printf("  -s, --video_size=WIDTHxHEIGHT\n");
    printf("  -r, --frame_rate=FRAME_RATE\n");
    printf("  -f, --pixel_format=PIXEL_FORMAT\n");
    printf("  -o, --output_file=FILE\n");
    printf("  -h, --help\n");
    printf("Available pixel formats:\n");
    printf("  YUYV\n");
    printf("  MJPG\n");
    printf("  H264\n");
    printf("  HEVC\n");
    printf("  GREY\n");
}


void output(void *address, int width, int height, int64_t host_notify_time_nanos) {

    uint8_t *in = (uint8_t *) address;

    FRAME_META_DATA *meta_data_cv0 = get_metadata_ptr_by_frame(in, CAMERA_CV0_ID, width, height);
    FRAME_META_DATA *meta_data_cv1 = get_metadata_ptr_by_frame(in, CAMERA_CV1_ID, width, height);

    int image_width = 640;

    NRGrayscaleCameraFrameData NRframe = {}; // 通过 {} 初始化所有成员为零
    NRframe.notify_time_nanos = host_notify_time_nanos;


    NRframe.camera_count = 2;
    NRframe.data_bytes = width * height; // 计算数据字节数
    NRframe.pixel_format = NR_GRAYSCALE_CAMERA_PIXEL_FORMAT_YUV_420_888;
    NRframe.frame_id = meta_data_cv1->frame_id;
    NRframe.cameras[0].offset = 0;
    NRframe.cameras[0].camera_id = NR_GRAYSCALE_CAMERA_ID_1;
    NRframe.cameras[0].width = image_width; //640
    NRframe.cameras[0].height = UVC_CAMERA_HEIGHT; //UVC_CAMERA_HEIGHT = 512
    NRframe.cameras[0].stride = 0; // UVC_CAMERA_WIDTH/2 = 768
    NRframe.cameras[0].exposure_duration = meta_data_cv1->exposure_time_ns;
    NRframe.cameras[0].rolling_shutter_time = meta_data_cv1->rolling_shutter;
    NRframe.cameras[0].gain = meta_data_cv1->gain_value;
    NRframe.cameras[0].exposure_start_time_device = meta_data_cv1->timestamp;
    NRframe.cameras[0].exposure_start_time_system = meta_data_cv1->timestamp_system;
    NRframe.cameras[0].exposure_end_time_device = meta_data_cv1->exposure_end_time_ns;
    NRframe.cameras[0].uvc_send_time_device = meta_data_cv1->uvc_send_time_ns;

    NRframe.cameras[1].offset = 512 * 640;
    NRframe.cameras[1].camera_id = NR_GRAYSCALE_CAMERA_ID_0;
    NRframe.cameras[1].width = image_width; //640
    NRframe.cameras[1].height = UVC_CAMERA_HEIGHT; //UVC_CAMERA_HEIGHT = 512
    NRframe.cameras[1].stride = 0; // UVC_CAMERA_WIDTH/2 = 768
    NRframe.cameras[1].exposure_duration = meta_data_cv0->exposure_time_ns;
    NRframe.cameras[1].rolling_shutter_time = meta_data_cv0->rolling_shutter;
    NRframe.cameras[1].gain = meta_data_cv0->gain_value;
    NRframe.cameras[1].exposure_start_time_device = meta_data_cv0->timestamp;
    NRframe.cameras[1].exposure_start_time_system = meta_data_cv0->timestamp_system;
    NRframe.cameras[1].exposure_end_time_device = meta_data_cv0->exposure_end_time_ns;
    NRframe.cameras[1].uvc_send_time_device = meta_data_cv0->uvc_send_time_ns;

    //NRframe.data
    //////////////////////
    // 为 NRframe.data 分配足够的内存
    NRframe.data = (uint8_t *) malloc(
            NRframe.data_bytes); //data release in ShowImage::recordCamera()
    //NRframe.data = new uint8_t[NRframe->data_bytes];
    // 使用 memcpy 复制数据
    memcpy(NRframe.data, in, NRframe.data_bytes);
    //////////////////////

    ShowImage_Push(g_show_image_handle, &NRframe);
}

int main(int argc, char **argv) {
    video_fmt_t fmt = {0};
    const char *dev = "/dev/videoX\0";
    const char *output_file = NULL;
    int of_fd = -1;
    int opt_index = 0;
    int c;

    char *photo_path = "";

    while ((c = getopt_long(argc, argv, "d:s:r:f:o:h", long_options, &opt_index)) != -1) {
        switch (c) {
            case 'd':
                dev = optarg;
                break;
            case 's': { // video_size: 格式为 widthxheight
                sscanf(optarg, "%dx%d", &fmt.width, &fmt.height);
                break;
            }
            case 'r':  // frame_rate: 字符串形式
                fmt.frame_rate = atoi(optarg);
                break;
            case 'f':  // pixel_format: 如 YUYV、MJPG 等
                fmt.pixel_format = v4l2_pixel_format_from_string(optarg);
                break;
            case 'o':
                output_file = optarg;
                break;
            case 'h':
                print_usage(argv[0]);
                return 1;
                break;
            default:
                fprintf(stderr, "Unknown option\n");
                print_usage(argv[0]);
                return 1;
        }
    }

    printf("dev: %s\n", dev);
    printf("video_size: %dx%d\n", fmt.width, fmt.height);
    printf("frame_rate: %d\n", fmt.frame_rate);
    printf("pixel_format: %s\n", v4l2_pixel_format_to_string(fmt.pixel_format));




    g_show_image_handle = ShowImage_Create(10000, 50000);
    ShowImage_Start(g_show_image_handle, RECORD_SAVE_ALL, photo_path, "cam0", "cam1");
    if (output_file != NULL) {
        of_fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0666);
        if (of_fd < 0) {
            fprintf(stderr, "Open %s failed: %s\n", output_file, strerror(errno));
            return 1;
        }
    }

    int fd = v4l2_open(dev);
    if (fd < 0) {
        fprintf(stderr, "Failed to open device\n");
        CLOSE_FD(of_fd);
        return 1;
    }

    if (v4l2_config_stream(fd, &fmt) < 0) {
        fprintf(stderr, "Failed to set format\n");
        v4l2_close(fd);
        CLOSE_FD(of_fd);
        return 1;
    }

    const int buffer_count = 4;
    frame_buffer_t *buffers = NULL;
    if (v4l2_allocate_buffers(&buffers, fd, buffer_count) < 0) {
        fprintf(stderr, "Failed to allocate buffers\n");
        v4l2_close(fd);
        CLOSE_FD(of_fd);
        return 1;
    }

    if (v4l2_stream_on(fd, buffer_count) < 0) {
        fprintf(stderr, "Failed to start streaming\n");
        v4l2_free_buffers(buffers, fd, buffer_count);
        v4l2_close(fd);
        CLOSE_FD(of_fd);
        return 1;
    }

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = handle_signal;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        v4l2_free_buffers(buffers, fd, buffer_count);
        v4l2_close(fd);
        CLOSE_FD(of_fd);
        return 1;
    }

    int frame_count = 0;
    struct timespec start_time, current_time;
    float elapsed_seconds;

    clock_gettime(CLOCK_MONOTONIC, &start_time);

    while (0 == g_stream_off_flag) {
        int buffer_index = v4l2_buffer_dequeue(buffers, fd, buffer_count);
        if (buffer_index < 0) {
            fprintf(stderr, "Failed to dequeue buffer\n");
            break;
        }
        frame_count++;

        clock_gettime(CLOCK_MONOTONIC, &current_time);
        elapsed_seconds = (current_time.tv_sec - start_time.tv_sec) +
                          (current_time.tv_nsec - start_time.tv_nsec) / 1e9;

        void *frame_data = buffers[buffer_index].address;
        size_t frame_size = buffers[buffer_index].bytesused;


//        printf("buffer index = %d, frame data = %p, frame size = %d\n",
//               buffer_index, frame_data, frame_size);

        if(fmt.pixel_format == V4L2_PIX_FMT_GREY){
            // log output
            int64_t host_notify_time_nanos =
                    (int64_t) current_time.tv_sec * 1000000000LL + current_time.tv_nsec;
            output(frame_data, fmt.width, fmt.height, host_notify_time_nanos);
        }
        if (of_fd >= 0) {
            if (frame_size != write(of_fd, frame_data, frame_size)) {
                printf("Saved frame failed: %s\n", strerror(errno));
                break;
            }
        }

        if (v4l2_buffer_enqueue(fd, buffer_index) < 0) {
            fprintf(stderr, "Failed to queue buffer\n");
            break;
        }

        if (elapsed_seconds >= 1.0) {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            // 将秒部分转为本地时间
            struct tm tm_time;
            localtime_r(&ts.tv_sec, &tm_time);

            char buf[64];
            // 格式化到秒
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_time);
            float fps = frame_count / elapsed_seconds;
            printf("FPS: %.2f Time: %s\n", fps,buf);

            // 重置计数器和起始时间
            frame_count = 0;
            start_time = current_time;
        }
    }

    if (of_fd >= 0) {
        close(of_fd);
        printf("Video file saved successfully: %s\n", output_file);
    }

    if (v4l2_stream_off(fd) < 0) {
        fprintf(stderr, "Failed to stop streaming\n");
    }

    v4l2_free_buffers(buffers, fd, buffer_count);
    v4l2_close(fd);

    ShowImage_Stop(g_show_image_handle);
    ShowImage_Destroy(g_show_image_handle);
    return 0;
}

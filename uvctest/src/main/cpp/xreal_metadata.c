#include "metadata.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <pthread.h>

static pthread_mutex_t frame_id_mutex = PTHREAD_MUTEX_INITIALIZER;
static int cv0_last_frame_id = 0;
static int cv1_last_frame_id = 0;
static int cv0_total_lost_frame = 0;
static int cv1_total_lost_frame = 0;

FRAME_META_DATA *get_metadata_ptr_by_frame(uint8_t *raw_frame_data, CV_CAMERA_ID camera_id)
{
    FRAME_META_DATA *meta_data_ptr = NULL;

    if(camera_id == CAMERA_CV0_ID) {
        meta_data_ptr = (FRAME_META_DATA *)(raw_frame_data + GRAY_IMAGE_WIDTH * (GRAY_IMAGE_HEIGHT - 2));
    }
    else if(camera_id == CAMERA_CV1_ID) {
        meta_data_ptr = (FRAME_META_DATA *)(raw_frame_data + GRAY_IMAGE_WIDTH * (GRAY_IMAGE_HEIGHT - 1));
    }
    else {
        printf("error input camera id %d\n", camera_id);
    }

    return meta_data_ptr;
}


void check_frame_id(FRAME_META_DATA *meta_data, CV_CAMERA_ID camera_id)
{
    struct timespec ts;    
    if(meta_data == NULL)
        return;

    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        perror("clock_gettime");
        return;
    }
    uint64_t nanoseconds = (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;

    printf("CV%d current frame id=%d, frame timestamp=%lu ns, host timestamp=%lu ns\n", camera_id, meta_data->frame_id, meta_data->timestamp, nanoseconds);
    pthread_mutex_lock(&frame_id_mutex);

    if(camera_id == CAMERA_CV0_ID) {
        if(meta_data->frame_id - cv0_last_frame_id != 1) {
            printf("[Warning]CV%d lost frame, last frame id: %d current frame id:%d total lost frame:%d\n", \
            camera_id, cv0_last_frame_id, meta_data->frame_id, cv0_total_lost_frame);
            cv0_total_lost_frame += meta_data->frame_id - cv0_last_frame_id - 1;
        }
        cv0_last_frame_id = meta_data->frame_id;
    } else if(camera_id == CAMERA_CV1_ID) {
        if(meta_data->frame_id - cv1_last_frame_id != 1) {
            printf("[Warning]CV%d lost frame, last frame id: %d current frame id:%d total lost frame:%d\n", \
            camera_id, cv1_last_frame_id, meta_data->frame_id, cv1_total_lost_frame);
            cv1_total_lost_frame += meta_data->frame_id - cv1_last_frame_id - 1;
        }
        cv1_last_frame_id = meta_data->frame_id;
    }

    pthread_mutex_unlock(&frame_id_mutex);

    return;
}


void check_frame_timestamp(FRAME_META_DATA *meta_data, CV_CAMERA_ID camera_id)
{
    static uint64_t cv0_last_timestamp = 0;
    static uint64_t cv1_last_timestamp= 0;

    if(meta_data == NULL)
        return;

    if(camera_id == CAMERA_CV0_ID) {
        if((cv0_last_timestamp) && (meta_data->timestamp - cv0_last_timestamp > MAX_TIMESTAMP_DIFF_NS)) {
            printf("[Warning]CV%d frame:[%d], last frame timestamp: %lu ns, current frame timestamp:%lu ns, time diff is:%lu ns\n", \
            camera_id, meta_data->frame_id, cv0_last_timestamp, meta_data->timestamp, meta_data->timestamp - cv0_last_timestamp);
        }
        cv0_last_timestamp = meta_data->timestamp;
    } else if(camera_id == CAMERA_CV1_ID) {
        if((cv1_last_timestamp) && (meta_data->timestamp - cv1_last_timestamp > MAX_TIMESTAMP_DIFF_NS)) {
            printf("[Warning]CV%d frame:[%d], last frame timestamp: %lu ns, current frame timestamp:%lu ns, time diff is:%lu ns\n", \
            camera_id, meta_data->frame_id, cv1_last_timestamp, meta_data->timestamp, meta_data->timestamp - cv1_last_timestamp);
        }
        cv1_last_timestamp = meta_data->timestamp;
    }

    return;
}





#ifndef XREAL_FRAME_METADATA_H
#define XREAL_FRAME_METADATA_H

#include <stdint.h>
#include "frame_metadata.h"

#define FRAME_META_DATA_IMAGE_EN 1

#define GRAY_IMAGE_WIDTH 768
#define GRAY_IMAGE_HEIGHT 1026

#define MAX_TIMESTAMP_DIFF_NS 20000000

typedef enum {
    CAMERA_CV0_ID = 0,
    CAMERA_CV1_ID = 1
} CV_CAMERA_ID;

FRAME_META_DATA *get_metadata_ptr_by_frame(uint8_t *raw_frame_data, CV_CAMERA_ID camera_id);
void print_frame_metadata(FRAME_META_DATA *meta_data, CV_CAMERA_ID camera_id);
void check_frame_id(FRAME_META_DATA *meta_data, CV_CAMERA_ID camera_id);
void check_frame_timestamp(FRAME_META_DATA *meta_data, CV_CAMERA_ID camera_id);

#endif // METADATA_HPP

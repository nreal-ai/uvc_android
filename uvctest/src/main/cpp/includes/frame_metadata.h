#pragma once
#include <stdint.h>

#define UVC_CAMERA_WIDTH (1536)
#define UVC_CAMERA_HEIGHT (512)
#define UVC_CAMERA_FPS (30)
#define RGB_CAMERA_WIDTH (1920)
#define RGB_CAMERA_HEIGHT (1080)

// 保存类型
typedef enum {
    RECORD_SAVE_IMU = 0x01,   //0001
    RECORD_SAVE_CAM = 0x02,   //0010
    RECORD_SAVE_ALL = 0x03,   //0011
    RECORD_MKDIR_ONLY = 0x04, //0100
    SHOW_IMAGE_ONLY = 0x05    //0101
} RECORD_SAVE_TYPE;

typedef enum NRGrayscaleCameraPixelFormat {
    NR_GRAYSCALE_CAMERA_PIXEL_FORMAT_UNKNOWN = 0,
    NR_GRAYSCALE_CAMERA_PIXEL_FORMAT_YUV_420_888,
    NR_GRAYSCALE_CAMERA_PIXEL_FORMAT_RGB_BAYER_8BPP,
} NRGrayscaleCameraPixelFormat;

typedef enum NRGrayscaleCameraID {
    NR_GRAYSCALE_CAMERA_ID_0 = 0x0001,
    NR_GRAYSCALE_CAMERA_ID_1 = 0x0002,
    NR_GRAYSCALE_CAMERA_ID_2 = 0x0004,
    NR_GRAYSCALE_CAMERA_ID_3 = 0x0008,
} NRGrayscaleCameraID;

#pragma pack(1)
typedef struct FrameMetaData {
    uint32_t magic0;
    uint32_t frame_id;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint64_t timestamp;        // capture time
    uint64_t exposure_time_ns; // exposure duration
    uint64_t gain_value;       // gain
    uint32_t rolling_shutter;  // rolling shutter
    uint8_t reserved[76];
    uint32_t magic1;
} FRAME_META_DATA;

//typedef struct FrameMetaData {
//    uint32_t pool_id;
//    uint32_t frame_size;
//    uint32_t frame_id;
//    uint32_t width;
//    uint32_t height;
//    uint32_t stride;
//    uint64_t u64PhyAddr;
//    uint64_t timestamp;
//    uint64_t exposure_time_ns;
//    uint64_t gain_value;
//    int32_t gdc_vb_block;
//} FRAME_META_DATA;

// 包含两个FRAME_META_DATA的结构体，并加一个联合体padding（对齐到128字节）
typedef struct FrameMetaDataGroup {
    union {
        struct {
            uint64_t host_notify_time_nanos;
            FRAME_META_DATA meta1;
            FRAME_META_DATA meta2;
        };
        uint8_t padding[512]; // 2×128 + 8 =264, // 尽量和8字节对齐，因此padding对齐到512字节
    };
} FRAME_META_DATA_GROUP;

typedef struct NRGrayscaleCameraUnitData {
    union {
        struct {
            uint32_t offset;
            NRGrayscaleCameraID camera_id;
            uint32_t width;
            uint32_t height;
            uint32_t stride;
            uint64_t exposure_start_time_device;
            uint32_t exposure_duration;
            uint32_t rolling_shutter_time;
            uint32_t gain;
            uint64_t exposure_start_time_system;
        };
        uint8_t padding[64];
    };

} NRGrayscaleCameraUnitData;

typedef struct NRGrayscaleCameraFrameData {
    union {
        struct {
            NRGrayscaleCameraUnitData cameras[4];
            uint8_t * data;
            uint32_t data_bytes;
            uint8_t camera_count;
            uint32_t frame_id;
            NRGrayscaleCameraPixelFormat pixel_format;
            uint64_t notify_time_nanos;//如果运行在host上，就是host_notify_time_nanos, //如果运行在设备上，就是device_notify_time_nanos
            //uint8_t record_flag; // 0x01: 只录metadata.txt.  0x02: 只录image+timestamp.txt. 0x03: 录metadata.txt和image+timestamp.txt.
        };
        uint8_t padding[320];
    };

} NRGrayscaleCameraFrameData;
#pragma pack()

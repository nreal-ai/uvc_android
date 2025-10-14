#pragma once
#include "frame_metadata.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// C句柄类型（不暴露C++类细节）
typedef void* ShowImageHandle;

//// C风格元数据结构体（和C++结构体保持兼容）
//#pragma pack(1)
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
//
//typedef struct FrameMetaDataGroup {
//    union {
//        struct {
//            uint64_t host_notify_time_nanos;
//            FRAME_META_DATA meta1;
//            FRAME_META_DATA meta2;
//        };
//        uint8_t padding[128];
//    };
//} FRAME_META_DATA_GROUP;
//#pragma pack()

//// 保存类型
//typedef enum {
//    RECORD_SAVE_IMU = 0x01,
//    RECORD_SAVE_CAM = 0x02,
//    RECORD_SAVE_ALL = 0x03,
//    RECORD_MKDIR_ONLY = 0x04,
//    SHOW_IMAGE_ONLY = 0x05
//} RECORD_SAVE_TYPE;

// 创建/销毁
ShowImageHandle ShowImage_Create(uint32_t max_imu_buf_size, uint32_t max_cam_buf_size,int format_version);
void ShowImage_Destroy(ShowImageHandle handle);

// 推送FrameMetaDataGroup
void ShowImage_Push(ShowImageHandle handle, const NRGrayscaleCameraFrameData* frame);

// 启动、停止
//void ShowImage_Start(ShowImageHandle handle, RECORD_SAVE_TYPE type, const char* path, const char* cam0_dir, const char* cam1_dir, const char* ext);
void ShowImage_Start(ShowImageHandle handle, RECORD_SAVE_TYPE type, const char* path, const char* cam0_dir, const char* cam1_dir);
void ShowImage_Stop(ShowImageHandle handle);

// 获取记录路径
const char* ShowImage_GetRecordPath(ShowImageHandle handle);

#ifdef __cplusplus
}
#endif

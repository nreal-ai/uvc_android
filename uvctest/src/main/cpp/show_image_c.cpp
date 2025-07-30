#include "show_image_c.h"
#include "show_image.h"  // 你的C++类定义

#include <string>

extern "C" {

// 创建实例
ShowImageHandle ShowImage_Create(uint32_t max_imu_buf_size, uint32_t max_cam_buf_size) {
    return reinterpret_cast<ShowImageHandle>(new ShowImage(max_imu_buf_size, max_cam_buf_size));
}

// 销毁实例
void ShowImage_Destroy(ShowImageHandle handle) {
    if (handle) {
        delete reinterpret_cast<ShowImage*>(handle);
    }
}

// 推送数据
//
void ShowImage_Push(ShowImageHandle handle, const NRGrayscaleCameraFrameData* frame) {
    if (handle && frame) {
        reinterpret_cast<ShowImage*>(handle)->push(frame);
    }
}

// 启动
//void ShowImage_Start(ShowImageHandle handle, RECORD_SAVE_TYPE type, const char* path, const char* cam0_dir, const char* cam1_dir, const char* ext) {
void ShowImage_Start(ShowImageHandle handle, RECORD_SAVE_TYPE type, const char* path, const char* cam0_dir, const char* cam1_dir) {
    if (handle) {
        reinterpret_cast<ShowImage*>(handle)->start(
            type,
            path ? std::string(path) : "",
            cam0_dir ? std::string(cam0_dir) : "",
            cam1_dir ? std::string(cam1_dir) : ""
            //cam1_dir ? std::string(cam1_dir) : "",
            //ext ? std::string(ext) : ""
        );
    }
}

// 停止
void ShowImage_Stop(ShowImageHandle handle) {
    if (handle) {
        reinterpret_cast<ShowImage*>(handle)->stop();
    }
}

// 获取记录路径
const char* ShowImage_GetRecordPath(ShowImageHandle handle) {
    if (handle) {
        // 静态存储，线程不安全，如果需要多线程安全可改为外部分配
        static std::string path;
        path = reinterpret_cast<ShowImage*>(handle)->getRecordPath();
        return path.c_str();
    }
    return NULL;
}

} // extern "C"

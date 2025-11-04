#pragma once
#include <thread>
#include <memory>
#include <string>
#include <media/NdkImageReader.h>
#include <media/NdkMediaCodec.h>
#include "frame_metadata.h"
#include "record-threadsafe-queue.h"


class ShowImage {
  public:
    typedef std::shared_ptr<NRGrayscaleCameraFrameData> cam_ptr;

  public:
    ShowImage(uint32_t _max_imu_buf_size = 10000, uint32_t _max_cam_buf_size = 50000,int format_version = 0);
    virtual ~ShowImage();

    void push(const NRGrayscaleCameraFrameData *frame_meta_data_group);
    void start(RECORD_SAVE_TYPE type = RECORD_SAVE_ALL, const std::string& path = "", const std::string& cam0_file_path="cam0", const std::string& cam1_file_path="cam1", const std::string& ext = ".pgm");
    void stop();
    std::string getRecordPath();
    int createDir(const std::string &dir);

  protected:
    void init(const std::string& path, const std::string& cam0_file_path, const std::string& cam1_file_path, const std::string& ext);
    
    void recordCamera();
    // 拷贝文件
    bool copyFile(const std::string& src, const std::string& dst);

private:
    void startMediaCodec(int width, int height);
    void decodeAndSaveFrame(cam_ptr ptr,std::string image_name);
  protected:
    bool record_thread_running_{false};
    Record::ThreadSafeQueue<cam_ptr> cam_queue_;
    const uint32_t kMaxIMUSize_;
    const uint32_t kMaxCamSize_;
    std::shared_ptr<std::thread> frame_meta_data_group_thread_;


    char dir_name_[100];
    std::string imu_full_file_name_;
    std::string imu_file_name_;
    std::string cam_file_path_[2];

    std::string imu_buff_file_;
    std::string cam_buff_file_;
    std::string cam_0_sync_check_file_;
    std::string cam_1_sync_check_file_;


    std::string json_file_ = "record_config.json";
    std::string image_suffix_ = ".pgm";
    bool only_save_metadata_{false};
    std::string imu_data_format_;
    RECORD_SAVE_TYPE save_type_;
    int format_version_;

    AMediaCodec* codec_;
};

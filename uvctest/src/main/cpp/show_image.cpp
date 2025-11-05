#include "show_image.h"
#include <record-threadsafe-queue.h>
#include <thread>
#include <dirent.h>  // use : opendir
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <cmath>  // For log2 function
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <json.hpp> // json
#include "signal_handler.h"  // 引入signal_handler
#include <jpeglib.h>
#include <turbojpeg.h>

ShowImage::ShowImage(uint32_t _max_imu_buf_size, uint32_t _max_cam_buf_size, int format_version)
        : kMaxIMUSize_(_max_imu_buf_size), kMaxCamSize_(_max_cam_buf_size) {
    record_thread_running_ = false;
    format_version_ = format_version;
}

ShowImage::~ShowImage() {
    stop();
    while (!cam_queue_.queue_.empty()) {
        cam_ptr ptr = cam_queue_.Pop();
        if (ptr && ptr->data != nullptr) {
            delete[](uint8_t *) (ptr->data);
            ptr->data = nullptr;
        }
    }
}

void ShowImage::push(const NRGrayscaleCameraFrameData *frame) {
    if (!record_thread_running_ || (save_type_ & RECORD_SAVE_CAM) != RECORD_SAVE_CAM) {
        return;
    }
    cam_ptr ptr(new NRGrayscaleCameraFrameData), old_ptr;
    memcpy(ptr->padding, frame->padding, sizeof(frame->padding));

    // 深拷贝内容，避免内存泄漏
//    if(frame->data && frame->data_bytes > 0){
//        ptr->data = new uint8_t (frame->data_bytes);
//        ptr->data = (uint8_t *)malloc(frame->data_bytes);
//        memcpy(ptr->data , frame->data,frame->data_bytes);
//    }
    if (cam_queue_.PushNonBlockingDroppingIfFull(ptr, kMaxCamSize_, old_ptr)) {
        std::cout << "cam queue is full" << std::endl;
        if (old_ptr->data != nullptr) {
            std::cout << "delete old data" << std::endl;
            delete[](uint8_t *) (old_ptr->data);
            old_ptr->data = nullptr;
        }
    }
}


void
ShowImage::start(RECORD_SAVE_TYPE type, const std::string &path, const std::string &cam0_file_path,
                 const std::string &cam1_file_path, const std::string &ext_param) {
    if (true == record_thread_running_) {
        return;
    }

    std::string ext = ext_param;

    std::ifstream ifs(json_file_.c_str());
    if (!ifs) {
        std::cout << "无法打开配置文件: " << json_file_ << std::endl;
    } else {

        try {
            nlohmann::json record;
            ifs >> record;

            if (record.contains("only_metadata") && record["only_metadata"] == 1) {
                only_save_metadata_ = true;
            }
            std::cout << "only_save_metadata_:" << only_save_metadata_ << std::endl;

            if (record.contains("ext")) {
                ext = record["ext"];
            }

            std::cout << "读取配置文件: " << json_file_ << std::endl << record << std::endl;

        } catch (std::exception &e) {
            std::cout << "JSON 解析失败: " << e.what() << std::endl;
        }

    }

    save_type_ = type;

    //std::cout << __PRETTY_FUNCTION__ << std::endl;
    if (save_type_ != SHOW_IMAGE_ONLY) {
        init(path, cam0_file_path, cam1_file_path, ext);
    }

    record_thread_running_ = true;
    frame_meta_data_group_thread_.reset(new std::thread(std::bind(&ShowImage::recordCamera, this)));
}

void ShowImage::stop() {
    if (true == record_thread_running_) {
        record_thread_running_ = false;
        frame_meta_data_group_thread_->join();
        //std::cout << __PRETTY_FUNCTION__ << std::endl;
    }
}

void ShowImage::init(const std::string &path, const std::string &cam0_file_path,
                     const std::string &cam1_file_path, const std::string &ext) {
    //std::cout << __PRETTY_FUNCTION__ << std::endl;
    time_t t;
    time(&t);
    struct tm *sys_time = localtime(&t);
    memset(dir_name_, 0, sizeof(dir_name_));
    if (path.empty()) {
#ifdef __ANDROID__
        sprintf(dir_name_, "/sdcard/nreal_log/sensor_data_%02d%02d-%02d%02d-%02d",
                sys_time->tm_mon + 1, sys_time->tm_mday, sys_time->tm_hour, sys_time->tm_min,
                sys_time->tm_sec);
        if (0 != createDir("/sdcard/nreal_log")) {
            exit(-1);
        }
#else
        sprintf(dir_name_, "%02d%02d-%02d%02d-%02d", sys_time->tm_mon + 1, sys_time->tm_mday,
                sys_time->tm_hour, sys_time->tm_min, sys_time->tm_sec);
#endif
    } else {
        sprintf(dir_name_, "%s", path.c_str());
    }

    std::cout << "\033[32msave image dirname: " << dir_name_ << "\033[0m" << std::endl; //32m green

    if (imu_file_name_.empty()) {
        imu_file_name_ = "imu.txt";
    }
    //std::cout << "imu_file_name_:" << imu_file_name_ << __FILE__ << __LINE__ << std::endl;
    //std::cout << "dir_name_:" << dir_name_ << __FILE__ << __LINE__ << std::endl;
    //std::cout << "std::string(dir_name_):" << std::string(dir_name_) << __FILE__ << __LINE__ << std::endl;
    //std::cout << "std::string(dir_name_) + std::string(\"/\" + imu_file_name_):" << std::string(dir_name_) + std::string("/" + imu_file_name_) << __FILE__ << __LINE__ << std::endl;
    //imu_full_file_name_ = std::string(std::string(dir_name_) + std::string("/" + imu_file_name_));
    //std::cout << "imu_full_file_name_:" <<  imu_full_file_name_ << __FILE__ << __LINE__ << std::endl;
    //std::cout << __FILE__ << __LINE__ << std::endl;

    if (0 != createDir(std::string(dir_name_))) {
        exit(-1);
    }


    if (0 != createDir(std::string(dir_name_) + std::string("/" + cam0_file_path))) {
        exit(-1);
    }
    if (0 != createDir(std::string(dir_name_) + std::string("/" + cam0_file_path) + "/images/")) {
        exit(-1);
    }


    if (!cam1_file_path.empty()) {
        if (0 != createDir(std::string(dir_name_) + std::string("/" + cam1_file_path))) {
            exit(-1);
        }

        if (0 !=
            createDir(std::string(dir_name_) + std::string("/" + cam1_file_path) + "/images/")) {
            exit(-1);
        }
    }

    cam_file_path_[0] = std::string(dir_name_) + std::string("/" + cam0_file_path) + "/images/";
    cam_file_path_[1] = std::string(dir_name_) + std::string("/" + cam1_file_path) + "/images/";


    if (!ext.empty()) {
        image_suffix_ = ext;
    }

    copyFile(json_file_, std::string(dir_name_) + "/" + std::string(json_file_));

}

int ShowImage::createDir(const std::string &dir) {
    if (dir.empty()) return -1;
    if (opendir(dir.c_str()) == NULL) {
        std::cout << dir << " is not exist,create it now!" << std::endl;
#ifndef _WIN32
        if (mkdir(dir.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO)) {
            std::cout << dir << " create failed! errno: " << errno
                      << " (" << std::strerror(errno) << ")" << std::endl;
            return -2;
        } else
            std::cout << dir << " create successed!" << std::endl;
#else
        if (mkdir(dir.c_str())) {
            std::cout << dir << " create failed!" << std::endl;
            return -2;
        } else
            std::cout << dir << " create successed!" << std::endl;
#endif
    }
    return 0;
}


void ShowImage::recordCamera() {
    std::ofstream cam_buff_txt;
    cam_buff_file_.clear();
    cam_buff_file_ = std::string(dir_name_) + std::string("/cam_buff.txt");
    cam_buff_txt.open(cam_buff_file_, std::ios::out);


    std::ofstream cam_0_sync_check_txt;
    cam_0_sync_check_file_.clear();
    cam_0_sync_check_file_ = std::string(dir_name_) + std::string("/cam_0_sync_check.csv");
    cam_0_sync_check_txt.open(cam_0_sync_check_file_, std::ios::out);
    cam_0_sync_check_txt
            << "frame_id,image_filename,host_notify_time_nanos,cam0_exposure_start_time_device,cam0_exposure_duration,cam0_rolling_shutter_time,cam0_gain,cam0_exposure_end_time_device,cam0_uvc_send_time_device,cam0_exposure_start_time_system"
            << "\n";
    cam_0_sync_check_txt.flush();


    std::ofstream cam_1_sync_check_txt;
    cam_1_sync_check_file_.clear();
    cam_1_sync_check_file_ = std::string(dir_name_) + std::string("/cam_1_sync_check.csv");
    cam_1_sync_check_txt.open(cam_1_sync_check_file_, std::ios::out);
    cam_1_sync_check_txt
            << "frame_id,image_filename,host_notify_time_nanos,cam1_exposure_start_time_device,cam1_exposure_duration,cam1_rolling_shutter_time,cam1_gain,cam1_exposure_end_time_device,cam1_uvc_send_time_device,cam1_exposure_start_time_system"
            << "\n";
    cam_1_sync_check_txt.flush();

    static int64_t hmd_hw_exposure_middle_time = 0;
    static int64_t hmd_host_exposure_middle_time = 0;
    static int64_t exposure_end_time_device = 0;
    //int first_camid = -1;


    std::ofstream camera_timestamp_txt;
    std::ofstream metadata_txt;
    std::string image_name, time_stamp_name, metadata_name, tmp;
    uint64_t frame_index = 10000000;

    bool first_loop = true;

    while (record_thread_running_ || cam_queue_.Size() > 0) {
        cam_ptr ptr = cam_queue_.Pop();
        if (nullptr == ptr) {
            continue;
        }
//        std::cout << "frame_id:" << ptr->frame_id << " data size:" << ptr->data_bytes  << std::endl;
        if (ptr->frame_id <= 0) {
            if (ptr->data != 0) {
                delete[](uint8_t *) (ptr->data);
            }
            continue;
        }

        cam_buff_txt << cam_queue_.Size() << " " << kMaxCamSize_ << "\n";
        cam_buff_txt.flush();

        if (first_loop) {
            first_loop = false;
            std::cout << "start save image." << std::endl;
            for (int i = 0; i < ptr->camera_count; i++) {
                if (!only_save_metadata_) {
                    time_stamp_name = cam_file_path_[i] + std::string("timestamps.txt");
                    camera_timestamp_txt.open(time_stamp_name.c_str(),
                                              std::ios::out);
                    camera_timestamp_txt.close();
                }

                metadata_name = cam_file_path_[i] + std::string("metadata.txt");
                metadata_txt.open(metadata_name.c_str(),
                                  std::ios::out);
                metadata_txt.close();
            }

            if(ptr->pixel_format == NR_CAMERA_PIXEL_FORMAT_HEVC){
                createDir(cam_file_path_[0] + "odd/");
                createDir(cam_file_path_[0] + "even/");
            }
        }

        std::ostringstream o;
        o << frame_index;
        if (image_suffix_.empty()) {
            tmp = o.str() + std::string(".jpg");
        } else {
            tmp = o.str() + image_suffix_;
        }
        tmp.at(0) = 'm';

        for (int i = 0; i < ptr->camera_count; i++) {
            if (ptr->camera_count > 1) {
                image_name = cam_file_path_[i] + tmp;
                hmd_hw_exposure_middle_time = ptr->cameras[i].exposure_start_time_device +
                                              (ptr->cameras[i].exposure_duration / 2) +
                                              (ptr->cameras[i].rolling_shutter_time / 2);
                exposure_end_time_device = ptr->cameras[i].exposure_start_time_device +
                                           ptr->cameras[i].exposure_duration +
                                           ptr->cameras[i].rolling_shutter_time;
                if (ptr->cameras[i].exposure_start_time_system == 0) {
                    hmd_host_exposure_middle_time = 0;
                } else {
                    hmd_host_exposure_middle_time = ptr->cameras[i].exposure_start_time_system +
                                                    (ptr->cameras[i].exposure_duration / 2) +
                                                    (ptr->cameras[i].rolling_shutter_time / 2);
                }
                time_stamp_name = cam_file_path_[i] + std::string("timestamps.txt");
                metadata_name = cam_file_path_[i] + std::string("metadata.txt");
            } else {
                //if (first_camid == -1) {
                //  first_camid = ptr->cameras[i].camera_id;
                //}
                int idx = ptr->cameras[i].camera_id - 1; // cameraid 从1开始
                image_name = cam_file_path_[idx] + tmp;
                //std::cout << "==  "<< image_name << "  " <<  cam_file_path_[idx] <<  "  " << tmp  << "  " <<  idx  << " "<<  __FILE__ << __LINE__  << std::endl;
                hmd_hw_exposure_middle_time = ptr->cameras[i].exposure_start_time_device +
                                              (ptr->cameras[i].exposure_duration / 2) +
                                              (ptr->cameras[i].rolling_shutter_time / 2);
                exposure_end_time_device = ptr->cameras[i].exposure_start_time_device +
                                           ptr->cameras[i].exposure_duration +
                                           ptr->cameras[i].rolling_shutter_time;
                if (ptr->cameras[i].exposure_start_time_system == 0) {
                    hmd_host_exposure_middle_time = 0;
                } else {
                    hmd_host_exposure_middle_time = ptr->cameras[i].exposure_start_time_system +
                                                    (ptr->cameras[i].exposure_duration / 2) +
                                                    (ptr->cameras[i].rolling_shutter_time / 2);
                }
                time_stamp_name = cam_file_path_[idx] + std::string("timestamps.txt");
                metadata_name = cam_file_path_[idx] + std::string("metadata.txt");
            }

            if (!only_save_metadata_) {
                if (ptr->pixel_format == NR_CAMERA_PIXEL_FORMAT_RGB_BAYER_8BPP) {
                    cv::Mat cv_image = cv::Mat(ptr->cameras[i].height + ptr->cameras[i].height / 2,
                                               ptr->cameras[i].width, CV_8UC1,
                                               (void *) ((uint8_t *) ptr->data +
                                                         ptr->cameras[i].offset),
                                               ptr->cameras[i].stride);
                    // YUV420P/I420转BGR
                    cv::Mat bgrImg;
                    cv::cvtColor(cv_image, bgrImg, cv::COLOR_YUV2BGR_I420);
                    cv::imwrite(image_name, bgrImg);
                } else if (ptr->pixel_format == NR_CAMERA_PIXEL_FORMAT_YUV_420_888) {
                    cv::Mat cv_image = cv::Mat(ptr->cameras[i].height, ptr->cameras[i].width,
                                               CV_8UC1,
                                               (void *) ((uint8_t *) ptr->data +
                                                         ptr->cameras[i].offset),
                                               ptr->cameras[i].stride);
                    cv::imwrite(image_name, cv_image);
                } else if (ptr->pixel_format == NR_CAMERA_PIXEL_FORMAT_HEVC) {
                    tmp = std::to_string(frame_index) + std::string(".jpg");

                    if(ptr->cameras[0].exposure_start_time_device % 2 == 0){
                        image_name = cam_file_path_[0]  + "even/"+ tmp;
                    }else{
                        image_name = cam_file_path_[0]  + "odd/"+ tmp;
                    }
                    decodeFrame(ptr, image_name);
                }
            }

            camera_timestamp_txt.open(time_stamp_name.c_str(),
                                      std::ios::out | std::ios::app);
            camera_timestamp_txt.precision(16);
            camera_timestamp_txt << tmp.c_str() << " " << hmd_hw_exposure_middle_time * 1e-9
                                 << "\n";
            camera_timestamp_txt.close();

            metadata_txt.open(metadata_name.c_str(),
                              std::ios::out | std::ios::app);
            metadata_txt.precision(16);
            metadata_txt << tmp.c_str() << " " << hmd_hw_exposure_middle_time << " "
                         << ptr->cameras[i].exposure_duration << " " << ptr->cameras[i].gain << " "
                         << hmd_host_exposure_middle_time
                         << " " << ptr->cameras[i].exposure_start_time_device << " "
                         << ptr->cameras[i].rolling_shutter_time << " " << ptr->cameras[i].stride
                         << " " << exposure_end_time_device << " " << ptr->notify_time_nanos
                         << "\n"; //notify_time_nanos now is host_notify_time_nanos
            metadata_txt.close();
        }

        if (format_version_ == 0) {
            cam_0_sync_check_txt << ptr->frame_id << "," << tmp << "," << ptr->notify_time_nanos
                                 << ","
                                 << ptr->cameras[0].exposure_start_time_device << ","
                                 << ptr->cameras[0].exposure_duration << ","
                                 << ptr->cameras[0].rolling_shutter_time << ","
                                 << ptr->cameras[0].gain << ","
                                 << ptr->cameras[0].exposure_end_time_device << ","
                                 << ptr->cameras[0].uvc_send_time_device << ","
                                 << ptr->cameras[0].exposure_start_time_system << "\n";
            cam_0_sync_check_txt.flush();

            cam_1_sync_check_txt << ptr->frame_id << "," << tmp << "," << ptr->notify_time_nanos
                                 << ","
                                 << ptr->cameras[1].exposure_start_time_device << ","
                                 << ptr->cameras[1].exposure_duration << ","
                                 << ptr->cameras[1].rolling_shutter_time << ","
                                 << ptr->cameras[1].gain << ","
                                 << ptr->cameras[1].exposure_end_time_device << ","
                                 << ptr->cameras[1].uvc_send_time_device << ","
                                 << ptr->cameras[1].exposure_start_time_system << "\n";
            cam_1_sync_check_txt.flush();
        } else if (format_version_ == 1) {
            if (ptr->cameras[0].sensor_index == 0) {
                cam_0_sync_check_txt << ptr->frame_id << "," << tmp << "," << ptr->notify_time_nanos
                                     << ","
                                     << ptr->cameras[0].exposure_start_time_device << ","
                                     << ptr->cameras[0].exposure_duration << ","
                                     << ptr->cameras[0].rolling_shutter_time << ","
                                     << ptr->cameras[0].gain << ","
                                     << ptr->cameras[0].exposure_end_time_device << ","
                                     << ptr->cameras[0].uvc_send_time_device << ","
                                     << ptr->cameras[0].exposure_start_time_system << "\n";
                cam_0_sync_check_txt.flush();
            } else if (ptr->cameras[0].sensor_index == 1) {
                cam_1_sync_check_txt << ptr->frame_id << "," << tmp << "," << ptr->notify_time_nanos
                                     << ","
                                     << ptr->cameras[0].exposure_start_time_device << ","
                                     << ptr->cameras[0].exposure_duration << ","
                                     << ptr->cameras[0].rolling_shutter_time << ","
                                     << ptr->cameras[0].gain << ","
                                     << ptr->cameras[0].exposure_end_time_device << ","
                                     << ptr->cameras[0].uvc_send_time_device << ","
                                     << ptr->cameras[0].exposure_start_time_system << "\n";
                cam_1_sync_check_txt.flush();
            }
        }

        delete[](uint8_t *) (ptr->data);
        frame_index++;

        if (ptr->camera_count > 1) {
            if (frame_index % 300 == 0) {
                std::cout << "already save image to index: " << frame_index << std::endl;
            }
        } else {
            if (frame_index % 100 == 0) {
                std::cout << "already save image to index: " << frame_index << std::endl;
            }
        }
    }

    cam_buff_txt.close();
}

// 拷贝文件
bool ShowImage::copyFile(const std::string &src, const std::string &dst) {
    std::ifstream in(src.c_str(), std::ios::binary);
    if (!in) return false;
    std::ofstream out(dst.c_str(), std::ios::binary);
    if (!out) return false;
    out << in.rdbuf();
    return true;
}

std::string ShowImage::getRecordPath() { return std::string(dir_name_); }

void ShowImage::startMediaCodec(int width, int height) {
    std::cout << "startMediaCodec with:" << width << "x" << height << std::endl;
    codec_ = AMediaCodec_createDecoderByType("video/hevc");
    AMediaFormat *format = AMediaFormat_new();
    AMediaFormat_setString(format, AMEDIAFORMAT_KEY_MIME, "video/hevc");
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_WIDTH, width);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_HEIGHT, height);

    media_status_t status = AMediaCodec_configure(codec_, format, nullptr, nullptr, 0);
    if (status == AMEDIA_OK) {
        AMediaCodec_start(codec_);
    } else {
        std::cout << "AMediaCodec_configure failed: " << status << std::endl;
        codec_ = nullptr;
    }
}

void ShowImage::decodeFrame(cam_ptr ptr, std::string image_name) {
    if (codec_ == nullptr) {
        startMediaCodec(ptr->cameras[0].width, ptr->cameras[0].height);
    }
    if (codec_ == nullptr) {
        return;
    }


    if (ptr->data[0] == 0x00 && ptr->data[1] == 0x00 &&
        (ptr->data[2] == 0x01 || (ptr->data[2] == 0x00 && ptr->data[3] == 0x01))) {
        ssize_t bufIdx = AMediaCodec_dequeueInputBuffer(codec_, 10000);
//        printf("Decoding frame size: %zu bytes -> %ld\n", ptr->data_bytes, bufIdx);
        if (bufIdx >= 0) {
            size_t bufSize;
            uint8_t *buf = AMediaCodec_getInputBuffer(codec_, bufIdx, &bufSize);

            if (buf && ptr->data_bytes <= bufSize) {
//                printf("AMediaCodec_queueInputBuffer data.size()  bufSize %zu %zu\n", ptr->data_bytes, bufSize);
                memcpy(buf, ptr->data, ptr->data_bytes);
                AMediaCodec_queueInputBuffer(
                        codec_, bufIdx, 0, ptr->data_bytes, 0, 0);
            } else {
                printf("AMediaCodec_getInputBuffer failed or data.size() > bufSize %zu %zu\n",
                       ptr->data_bytes, bufSize);
            }
        } else {
            printf("AMediaCodec_dequeueInputBuffer failed %zu\n", bufIdx);
        }

        AMediaCodecBufferInfo info;
        ssize_t outIdx = AMediaCodec_dequeueOutputBuffer(codec_, &info, 10000);
//        printf("AMediaCodec_dequeueOutputBuffer outIdx: %ld\n", outIdx);
        while (outIdx >= 0) {
            size_t out_size = 0;
            uint8_t *buffer = AMediaCodec_getOutputBuffer(codec_, outIdx,
                                                          &out_size);
//            printf("AMediaCodec_getOutputBuffer getbuffer size: %zu\n", out_size );
//            printf("frame write to %s\n", image_name.c_str() );

            // save raw data to file
//            std::ofstream out(image_name, std::ios::binary | std::ios::out);
//            out.write(reinterpret_cast<char *>(buffer), out_size);
//            out.close();

            // sava data as jpeg
            saveData(buffer,ptr->cameras[0].width,ptr->cameras[0].height,image_name.c_str());


            //写完文件release
            AMediaCodec_releaseOutputBuffer(codec_, outIdx, false);
            outIdx = AMediaCodec_dequeueOutputBuffer(codec_, &info, 0);

        }
    }
}

void ShowImage::saveData(const uint8_t* data,int width,int height,const char* path) {
    tjhandle handle = tjInitCompress();
    unsigned char *jpg_buffer = NULL;
    long unsigned int jpg_size = 0;

    int ret = tjCompressFromYUV(handle, data, width, 1, height, TJSAMP_420, &jpg_buffer, &jpg_size, 80, 0);
    FILE *fd = fopen(path,"wb");
    fwrite(jpg_buffer,1,jpg_size,fd);
    fclose(fd);

    free(jpg_buffer);
    tjDestroy(handle);
}
//void ShowImage::saveData(const uint8_t* data,int width,int height,const char* path){
//    FILE* outfile = fopen(path, "wb");
//    // 拆分 I420 平面
//    const uint8_t* yPlane = data;
//    const uint8_t* uPlane = yPlane + width * height;
//    const uint8_t* vPlane = uPlane + (width / 2) * (height / 2);
//
//    jpeg_compress_struct cinfo;
//    jpeg_error_mgr jerr;
//    cinfo.err = jpeg_std_error(&jerr);
//    jpeg_create_compress(&cinfo);
//    jpeg_stdio_dest(&cinfo, outfile);
//
//    cinfo.image_width = width;
//    cinfo.image_height = height;
//    cinfo.input_components = 3;
//    cinfo.in_color_space = JCS_YCbCr;
//
//    jpeg_set_defaults(&cinfo);
//    jpeg_set_quality(&cinfo, 70, true);
//
//    // 输入为 YUV420P（I420）
//    cinfo.raw_data_in = true;
//
//    // 设置采样率（4:2:0）
//    cinfo.comp_info[0].h_samp_factor = 2;
//    cinfo.comp_info[0].v_samp_factor = 2;
//    cinfo.comp_info[1].h_samp_factor = 1;
//    cinfo.comp_info[1].v_samp_factor = 1;
//    cinfo.comp_info[2].h_samp_factor = 1;
//    cinfo.comp_info[2].v_samp_factor = 1;
//
//    jpeg_start_compress(&cinfo, true);
//    // 构造行指针数组
//    JSAMPROW y[height];
//    JSAMPROW u[height / 2];
//    JSAMPROW v[height / 2];
//
//    for (int i = 0; i < height; i++) {
//        y[i] = (JSAMPROW)(yPlane + i * width);
//    }
//    for (int i = 0; i < height / 2; i++) {
//        u[i] = (JSAMPROW)(uPlane + i * (width / 2));
//        v[i] = (JSAMPROW)(vPlane + i * (width / 2));
//    }
//
//    JSAMPARRAY planes[3] = { y, u, v };
//    // 编码
//    while (cinfo.next_scanline < cinfo.image_height) {
//        jpeg_write_raw_data(&cinfo, planes, 16);
//    }
//
//    jpeg_finish_compress(&cinfo);
//    jpeg_destroy_compress(&cinfo);
//    fclose(outfile);
//}

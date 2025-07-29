#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <csignal>
//#include <log.h>

// 这里用 extern 只是声明，不分配内存
extern volatile sig_atomic_t sig_caught;

// 信号处理函数的声明
void signal_handler(int signal);


// 这里是真正的定义，分配内存
volatile sig_atomic_t sig_caught = 0;

void signal_handler(int signal) {
    if (signal == SIGINT) {
        sig_caught = 1;  // 设置信号捕获标志
        //TRACKING_LOG_INFO("received SIGINT!!! {}", sig_caught);
    }
}


#endif  // SIGNAL_HANDLER_H

#ifndef UTILITIES_RECORD_THREADSAFE_QUEUE_H_
#define UTILITIES_RECORD_THREADSAFE_QUEUE_H_

#include <assert.h>
#include <pthread.h>
#include <sys/time.h>
#include <queue>
#include <string>
#include <string>

namespace Record {

template <typename T>
class ThreadSafeQueue {
  public:
    ThreadSafeQueue() {
        pthread_mutex_init(&mutex_, NULL);
        pthread_cond_init(&condition_empty_, NULL);
        pthread_cond_init(&condition_full_, NULL);
    }

    ~ThreadSafeQueue() {
        pthread_mutex_destroy(&mutex_);
        pthread_cond_destroy(&condition_empty_);
        pthread_cond_destroy(&condition_full_);
    }

    // Push to the queue.
    void Push(const T& value) { PushNonBlocking(value); }

    // Push to the queue.
    void PushNonBlocking(const T& value) {
        pthread_mutex_lock(&mutex_);
        queue_.push(value);
        pthread_cond_signal(&condition_empty_);  // Signal that data is available.
        pthread_mutex_unlock(&mutex_);
    }

    size_t Size() { return queue_.size(); }

    // Push to the queue if the size is less than max_queue_size, else block.
    void PushBlockingIfFull(const T& value, size_t max_queue_size) {
        while (true) {
            pthread_mutex_lock(&mutex_);
            size_t size = queue_.size();
            if (size >= max_queue_size) {
                pthread_cond_wait(&condition_full_, &mutex_);
            }
            if (size >= max_queue_size) {
                pthread_mutex_unlock(&mutex_);
                continue;
            }
            queue_.push(value);
            pthread_cond_signal(&condition_empty_);  // Signal that data is available.
            pthread_mutex_unlock(&mutex_);
            break;
        }
    }

    // Returns true if oldest was dropped because queue was full.
    bool PushNonBlockingDroppingIfFull(const T& value, size_t max_queue_size) {
        pthread_mutex_lock(&mutex_);
        bool result = false;
        if (queue_.size() >= max_queue_size) {
            queue_.pop();
            result = true;
        }
        queue_.push(value);
        pthread_cond_signal(&condition_empty_);  // Signal that data is available.
        pthread_mutex_unlock(&mutex_);
        return result;
    }

    // Returns true if oldest was dropped because queue was full.
    bool PushNonBlockingDroppingIfFull(const T& value, size_t max_queue_size,
                                       T& value_pop) {
        pthread_mutex_lock(&mutex_);
        bool result = false;
        if (queue_.size() >= max_queue_size) {
            value_pop = queue_.front();
            queue_.pop();
            result = true;
        }
        queue_.push(value);
        pthread_cond_signal(&condition_empty_);  // Signal that data is available.
        pthread_mutex_unlock(&mutex_);
        return result;
    }

    // Pops from the queue blocking if queue is empty.
    T Pop() { return PopBlocking(); }

    // Pops from the queue blocking if queue is empty.
    T PopBlocking() {
        while (true) {
            pthread_mutex_lock(&mutex_);
            if (queue_.empty()) {
                updateTimeout();
                pthread_cond_timedwait(&condition_empty_, &mutex_, &abstime_);
            }
            if (queue_.empty()) {
                pthread_mutex_unlock(&mutex_);
                return nullptr;
            }
            T value = queue_.front();
            queue_.pop();
            pthread_cond_signal(&condition_full_);  // Notify that space is available.
            pthread_mutex_unlock(&mutex_);
            return value;
        }
        // return NULL;
    }

    // Check queue is empty, if yes return false, not altering value. If queue not
    // empty update value and return true.
    bool PopNonBlocking(T& value) {
        pthread_mutex_lock(&mutex_);
        if (queue_.empty()) {
            pthread_mutex_unlock(&mutex_);
            return false;
        }
        value = queue_.front();
        queue_.pop();
        pthread_mutex_unlock(&mutex_);
        return true;
    }
    pthread_mutex_t mutex_;
    pthread_cond_t condition_empty_;
    pthread_cond_t condition_full_;
    std::queue<T> queue_;

  protected:
    struct timespec abstime_;
    void updateTimeout() {
        struct timeval now;
        gettimeofday(&now, NULL);
        abstime_.tv_sec = now.tv_sec;
        abstime_.tv_nsec = (100000 + now.tv_usec) * 1000;
    }
};
}
#endif  // UTILITIES_RECORD_THREADSAFE_QUEUE_H_

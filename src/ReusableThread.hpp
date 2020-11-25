/**
* @file CommandFacility.cpp Reusable thread wrapper
* The same thread instance can be used with different tasks to be executed
* Inspired by: 
* https://codereview.stackexchange.com/questions/134214/reuseable-c11-thread
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef UDAQ_READOUT_SRC_REUSABLETHREAD_HPP_
#define UDAQ_READOUT_SRC_REUSABLETHREAD_HPP_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <string>

namespace dunedaq {
namespace readout {

class ReusableThread {
public:
  explicit ReusableThread(int thread_id)
    : thread_id_(thread_id)
    , task_executed_(true)
    , task_assigned_(false)
    , thread_quit_(false)
    , worker_done_(false)
    , thread_(&ReusableThread::thread_worker, this) 
  { }

  ~ReusableThread() {
    while (task_assigned_) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    thread_quit_ = true;
    while (!worker_done_) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      cv_.notify_all();
    }
    thread_.join();
  }

  ReusableThread(const ReusableThread&) 
    = delete; ///< ReusableThread is not copy-constructible
  ReusableThread& operator=(const ReusableThread&) 
    = delete; ///< ReusableThread is not copy-assginable
  ReusableThread(ReusableThread&&) 
    = delete; ///< ReusableThread is not move-constructible
  ReusableThread& operator=(ReusableThread&&) 
    = delete; ///< ReusableThread is not move-assignable

  // Set thread ID
  void set_thread_id(int tid) { thread_id_ = tid; }

  // Get thread ID
  int get_thread_id() const { return thread_id_; }

  // Set name for pthread handle
  void set_name(const std::string& name, int tid) {
    char tname[16];
    snprintf(tname, 16, "%s-%d", name.c_str(), tid); // NOLINT 
    auto handle = thread_.native_handle();
    pthread_setname_np(handle, tname);
  }

  // Check for completed task execution
  bool get_readiness() const { return task_executed_; }

  // Set task to be executed
  template <typename Function, typename... Args> 
  bool set_work(Function &&f, Args &&... args) {
    if (!task_assigned_ && task_executed_.exchange(false)) {
      task_ = std::bind(f, args...);
      task_assigned_ = true;
      cv_.notify_all();
      return true;
    }
    return false;
  }

private:
  // Internals
  int thread_id_;
  std::atomic<bool> task_executed_;
  std::atomic<bool> task_assigned_;
  std::atomic<bool> thread_quit_;
  std::atomic<bool> worker_done_;
  std::function<void()> task_;

  // Locks
  std::mutex mtx_;
  std::condition_variable cv_;
  std::thread thread_;

  // Actual worker thread
  void thread_worker() {
    std::unique_lock<std::mutex> lock(mtx_);

    while (!thread_quit_) {
      if (!task_executed_ && task_assigned_) {
        task_();
        task_executed_ = true;
        task_assigned_ = false;
      } else {
        cv_.wait(lock);
      }
    }

    worker_done_ = true;
  }
};

}
} // namespace dunedaq::readout

#endif // UDAQ_READOUT_SRC_REUSABLETHREAD_HPP_

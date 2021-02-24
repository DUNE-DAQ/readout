/**
* @file ReusableThread.hpp Reusable thread wrapper
* The same thread instance can be used with different tasks to be executed
* Inspired by: 
* https://codereview.stackexchange.com/questions/134214/reuseable-c11-thread
*
* This is part of the DUNE DAQ , copyright 2020.
* Licensing/copyright details are in the COPYING file that you should have
* received with this code.
*/
#ifndef READOUT_INCLUDE_READOUT_REUSABLETHREAD_HPP_
#define READOUT_INCLUDE_READOUT_REUSABLETHREAD_HPP_

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
  explicit ReusableThread(int m_threadid)
    : m_m_threadid(m_threadid)
    , m_m_taskexecuted(true)
    , m_m_taskassigned(false)
    , m_m_threadquit(false)
    , m_worker_done(false)
    , m_thread(&ReusableThread::m_threadworker, this) 
  { }

  ~ReusableThread() {
    while (m_m_taskassigned) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    m_m_threadquit = true;
    while (!m_worker_done) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      m_cv.notify_all();
    }
    m_thread.join();
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
  void set_thread_id(int tid) { m_m_threadid = tid; }

  // Get thread ID
  int get_thread_id() const { return m_m_threadid; }

  // Set name for pthread handle
  void set_name(const std::string& name, int tid) {
    set_thread_id(tid);
    char tname[16];
    snprintf(tname, 16, "%s-%d", name.c_str(), tid); // NOLINT 
    auto handle = m_thread.native_handle();
    pthread_setname_np(handle, tname);
  }

  // Check for completed task execution
  bool get_readiness() const { return m_m_taskexecuted; }

  // Set task to be executed
  template <typename Function, typename... Args> 
  bool set_work(Function &&f, Args &&... args) {
    if (!m_m_taskassigned && m_m_taskexecuted.exchange(false)) {
      m_task = std::bind(f, args...);
      m_m_taskassigned = true;
      m_cv.notify_all();
      return true;
    }
    return false;
  }

private:
  // Internals
  int m_m_threadid;
  std::atomic<bool> m_m_taskexecuted;
  std::atomic<bool> m_m_taskassigned;
  std::atomic<bool> m_m_threadquit;
  std::atomic<bool> m_worker_done;
  std::function<void()> m_task;

  // Locks
  std::mutex m_mtx;
  std::condition_variable m_cv;
  std::thread m_thread;

  // Actual worker thread
  void m_threadworker() {
    std::unique_lock<std::mutex> lock(m_mtx);

    while (!m_m_threadquit) {
      if (!m_m_taskexecuted && m_m_taskassigned) {
        m_task();
        m_m_taskexecuted = true;
        m_m_taskassigned = false;
      } else {
        m_cv.wait(lock);
      }
    }

    m_worker_done = true;
  }
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_REUSABLETHREAD_HPP_

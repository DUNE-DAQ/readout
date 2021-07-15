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
#ifndef READOUT_INCLUDE_READOUT_UTILS_REUSABLETHREAD_HPP_
#define READOUT_INCLUDE_READOUT_UTILS_REUSABLETHREAD_HPP_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

namespace dunedaq {
namespace readout {

class ReusableThread
{
public:
  explicit ReusableThread(int threadid)
    : m_thread_id(threadid)
    , m_task_executed(true)
    , m_task_assigned(false)
    , m_thread_quit(false)
    , m_worker_done(false)
    , m_thread(&ReusableThread::thread_worker, this)
  {}

  ~ReusableThread()
  {
    while (m_task_assigned) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    m_thread_quit = true;
    while (!m_worker_done) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      m_cv.notify_all();
    }
    m_thread.join();
  }

  ReusableThread(const ReusableThread&) = delete;            ///< ReusableThread is not copy-constructible
  ReusableThread& operator=(const ReusableThread&) = delete; ///< ReusableThread is not copy-assginable
  ReusableThread(ReusableThread&&) = delete;                 ///< ReusableThread is not move-constructible
  ReusableThread& operator=(ReusableThread&&) = delete;      ///< ReusableThread is not move-assignable

  // Set thread ID
  void set_thread_id(int tid) { m_thread_id = tid; }

  // Get thread ID
  int get_thread_id() const { return m_thread_id; }

  // Set name for pthread handle
  void set_name(const std::string& name, int tid)
  {
    set_thread_id(tid);
    char tname[16];
    snprintf(tname, 16, "%s-%d", name.c_str(), tid); // NOLINT
    auto handle = m_thread.native_handle();
    pthread_setname_np(handle, tname);
  }

  // Check for completed task execution
  bool get_readiness() const { return m_task_executed; }

  // Set task to be executed
  template<typename Function, typename... Args>
  bool set_work(Function&& f, Args&&... args)
  {
    if (!m_task_assigned && m_task_executed.exchange(false)) {
      m_task = std::bind(f, args...);
      m_task_assigned = true;
      m_cv.notify_all();
      return true;
    }
    return false;
  }

private:
  // Internals
  int m_thread_id;
  std::atomic<bool> m_task_executed;
  std::atomic<bool> m_task_assigned;
  std::atomic<bool> m_thread_quit;
  std::atomic<bool> m_worker_done;
  std::function<void()> m_task;

  // Locks
  std::mutex m_mtx;
  std::condition_variable m_cv;
  std::thread m_thread;

  // Actual worker thread
  void thread_worker()
  {
    std::unique_lock<std::mutex> lock(m_mtx);

    while (!m_thread_quit) {
      if (!m_task_executed && m_task_assigned) {
        m_task();
        m_task_executed = true;
        m_task_assigned = false;
      } else {
        m_cv.wait(lock);
      }
    }

    m_worker_done = true;
  }
};

} // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_UTILS_REUSABLETHREAD_HPP_

#ifndef APPFWK_UDAQ_READOUT_REUSABLETHREAD_HPP
#define APPFWK_UDAQ_READOUT_REUSABLETHREAD_HPP

/// \cond
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
/// \endcond

/********************************
 * ReusableThread
 * Adapted from:
 *   // https://codereview.stackexchange.com/questions/134214/reuseable-c11-thread
 * Description: Pausable/Reusable thread
 * Author: Roland Sipos rsipos@cern.ch
 * Date: November 2017
 *********************************/

namespace dunedaq {
namespace appfwk {
namespace readout {

using namespace std::chrono_literals;

class ReusableThread {
public:
  ReusableThread(unsigned int threadID)
      : m_thread_id(threadID), m_task_executed(true), m_task_assigned(false), m_thread_quit(false),
        m_worker_done(false), m_thread(&ReusableThread::thread_worker, this) {}

  ~ReusableThread() {
    while (m_task_assigned) {
      std::this_thread::sleep_for(1ms);
    }
    m_thread_quit = true;
    while (!m_worker_done) {
      std::this_thread::sleep_for(1ms);
      m_cv.notify_all();
    }
    m_thread.join();
  }

  ReusableThread(const ReusableThread &) = delete;
  ReusableThread &operator=(const ReusableThread &) = delete;

  unsigned int get_thread_id() const { return m_thread_id; }

  bool get_readiness() const { return m_task_executed; }

  void set_name(const std::string& name, uint16_t tid) {
    char tname[16];
    snprintf(tname, 16, "%s-%d", name.c_str(), tid);
    auto handle = m_thread.native_handle();
    pthread_setname_np(handle, tname);
  }


  template <typename Function, typename... Args> bool set_work(Function &&f, Args &&... args) {
    if (!m_task_assigned && m_task_executed.exchange(false)) {
      m_task = std::bind(f, args...);
      m_task_assigned = true;
      m_cv.notify_all();
      return true;
    }
    return false;
  }

private:
  unsigned int m_thread_id;
  std::atomic<bool> m_task_executed;
  std::atomic<bool> m_task_assigned;
  std::atomic<bool> m_thread_quit;
  std::atomic<bool> m_worker_done;
  std::function<void()> m_task;

  std::mutex m_mtx;
  std::condition_variable m_cv;
  std::thread m_thread;

  void thread_worker() {
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
} // namespace appfwk
} // namespace dunedaq

#endif //APPFWK_UDAQ_READOUT_REUSABLETHREAD_HPP

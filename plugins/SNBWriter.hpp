/**
 * @file SNBWriter.hpp Module to write snb data
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
*/

#ifndef READOUT_PLUGINS_SNBWRITER_HPP_
#define READOUT_PLUGINS_SNBWRITER_HPP_

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSource.hpp"
#include "appfwk/ThreadHelper.hpp"
#include "readout/snbwriter/Structs.hpp"
#include "readout/ReadoutTypes.hpp"
#include <boost/align/aligned_allocator.hpp>

#include <iostream>
#include <fstream>
#include <atomic>

namespace dunedaq {
  namespace readout {

    class SNBWriter : public dunedaq::appfwk::DAQModule {
    public:
      explicit SNBWriter(const std::string& name);

      SNBWriter(const SNBWriter&) = delete;
      SNBWriter& operator=(const SNBWriter&) = delete;
      SNBWriter(SNBWriter&&) = delete;
      SNBWriter& operator=(SNBWriter&&) = delete;

      void init(const nlohmann::json& obj) override;
      void get_info(opmonlib::InfoCollector& ci, int level) override;

    private:
      void do_conf(const nlohmann::json& obj);
      void do_start(const nlohmann::json& obj);
      void do_stop(const nlohmann::json& obj);

      dunedaq::appfwk::ThreadHelper m_thread;
      void do_work(std::atomic<bool>&);

      using source_t = dunedaq::appfwk::DAQSource<types::WIB_SUPERCHUNK_STRUCT>;
      std::unique_ptr<source_t> m_input_queue;

      int fd;
      std::string m_output_file;
      std::mutex m_start_lock;
    };
  }
}

#endif //READOUT_PLUGINS_SNBWRITER_HPP_

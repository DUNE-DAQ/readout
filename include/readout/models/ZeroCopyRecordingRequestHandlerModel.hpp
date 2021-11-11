/**
 * @file DefaultRequestHandlerModel.hpp Default mode of operandy for
 * request handling:
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef READOUT_INCLUDE_READOUT_MODELS_ZEROCOPYRECORDINGREQUESTHANDLERMODEL_HPP_
#define READOUT_INCLUDE_READOUT_MODELS_ZEROCOPYRECORDINGREQUESTHANDLERMODEL_HPP_

#include "readout/models/DefaultRequestHandlerModel.hpp"

namespace dunedaq {
  namespace readout {

    template<class ReadoutType, class LatencyBufferType>
    class ZeroCopyRecordingRequestHandlerModel : public DefaultRequestHandlerModel<ReadoutType, LatencyBufferType>
    {
    public:
      explicit ZeroCopyRecordingRequestHandlerModel(std::unique_ptr<LatencyBufferType>& latency_buffer,
                                          std::unique_ptr<FrameErrorRegistry>& error_registry)
                                          : DefaultRequestHandlerModel<ReadoutType, LatencyBufferType>(latency_buffer, error_registry)
      {
        TLOG_DEBUG(TLVL_WORK_STEPS) << "ZeroCopyRecordingRequestHandlerModel created...";
      }

      using inherited = DefaultRequestHandlerModel<ReadoutType, LatencyBufferType>;


      void conf(const nlohmann::json& args) override
      {

        auto conf = args["requesthandlerconf"].get<readoutconfig::RequestHandlerConf>();
        if (conf.enable_raw_recording) {
          inherited::m_geoid.element_id = conf.element_id;
          inherited::m_geoid.region_id = conf.region_id;
          inherited::m_geoid.system_type = ReadoutType::system_type;
          
          // Check for alignment restrictions
          if (inherited::m_latency_buffer->get_alignment_size() == 0 || sizeof(ReadoutType) * inherited::m_latency_buffer->get_size() % 4096) {
            ers::error(ConfigurationError(ERS_HERE, inherited::m_geoid, "Latency buffer is not 4k aligned"));
          }
          
          if (remove(conf.output_file.c_str()) == 0) {
            TLOG(TLVL_WORK_STEPS) << "Removed existing output file from previous run: " << conf.output_file;
          }

          m_oflag = O_CREAT | O_WRONLY;
          if (conf.use_o_direct) {
              m_oflag |= O_DIRECT;
          }
          m_fd = ::open(conf.output_file.c_str(), m_oflag, 0644);
          inherited::m_recording_configured = true;
        }
        inherited::conf(args);
      }

      void record(const nlohmann::json& args) override
      {
        if (inherited::m_recording.load()) {
          ers::error(CommandError(ERS_HERE, inherited::m_geoid, "A recording is still running, no new recording was started!"));
          return;
        }        
        inherited::m_recording_thread.set_work(
            [&](int duration) {
              size_t chunk_size = inherited::m_stream_buffer_size;
              size_t alignment_size = inherited::m_latency_buffer->get_alignment_size();
              TLOG() << "Start recording for " << duration << " second(s)" << std::endl;
              inherited::m_recording.exchange(true);
              auto start_of_recording = std::chrono::high_resolution_clock::now();
              auto current_time = start_of_recording;
              inherited::m_next_timestamp_to_record = 0;
              
              const char* current_write_pointer = nullptr;
              const char* start_of_buffer_pointer = reinterpret_cast<const char*>(inherited::m_latency_buffer->start_of_buffer());
	          const char* current_end_pointer;
              const char* end_of_buffer_pointer = reinterpret_cast<const char*>(inherited::m_latency_buffer->end_of_buffer());

              size_t bytes_written = 0;

              while (std::chrono::duration_cast<std::chrono::seconds>(current_time - start_of_recording).count() < duration) {
                if (!inherited::m_cleanup_requested || (inherited::m_next_timestamp_to_record == 0)) {
                  size_t considered_chunks_in_loop = 0;

                  // Wait for potential running cleanup to finish first
                  {
                    std::unique_lock<std::mutex> lock(inherited::m_cv_mutex);
                    inherited::m_cv.wait(lock, [&] { return !inherited::m_cleanup_requested; });
                  }
                  inherited::m_cv.notify_all();

                  // Some frames have to be skipped to start copying from an aligned piece of memory
                  // These frames cannot be written without O_DIRECT as this would mess up the alignment of the write pointer into the target file
                  if (inherited::m_next_timestamp_to_record == 0) {
                    auto begin = inherited::m_latency_buffer->begin();
                    if (begin == inherited::m_latency_buffer->end()) {
                        // There are no elements in the buffer, update time and try again
                        current_time = std::chrono::high_resolution_clock::now();
                        continue;
                    }
                    inherited::m_next_timestamp_to_record = begin->get_first_timestamp();
                    size_t skipped_frames = 0;
                    while (reinterpret_cast<std::uintptr_t>(&(*begin)) % alignment_size) {
                        ++begin;
                        skipped_frames++;
                        if (!begin.good()) {
                            // We reached the end of the buffer without finding an aligned element
                            // Reset the next timestamp to record and try again
                            current_time = std::chrono::high_resolution_clock::now();
                            inherited::m_next_timestamp_to_record = 0;
                            continue;
                        }
                    }
                    TLOG() << "Skipped " << skipped_frames << " frames";
                    current_write_pointer = reinterpret_cast<const char*>(&(*begin));
                  }

	              current_end_pointer = reinterpret_cast<const char*>(inherited::m_latency_buffer->back());

                  // Break the loop from time to time to update the timestamp and check if we should stop recording
                  while (considered_chunks_in_loop < 100) {
                    auto iptr = reinterpret_cast<std::uintptr_t>(current_write_pointer);
                    if (iptr % alignment_size) {
                        // This should never happen
                        TLOG() << "Error: Write pointer is not aligned";
                    } 
                    bool failed_write = false;
                    if (current_write_pointer + chunk_size < current_end_pointer) {
                      // We can write a whole chunk to file
                      failed_write |= !::write(m_fd, current_write_pointer, chunk_size);
                      if (!failed_write) {
                          bytes_written += chunk_size;
                      }
                      current_write_pointer += chunk_size;
                    } else if (current_end_pointer < current_write_pointer) {
                      if (current_write_pointer + chunk_size < end_of_buffer_pointer) {
                        // Write whole chunk to file
                        failed_write |= !::write(m_fd, current_write_pointer, chunk_size);
                        if (!failed_write) {
                            bytes_written += chunk_size;
                        }
                        current_write_pointer += chunk_size;
                      } else {
                        // Write the last bit of the buffer without using O_DIRECT as it possibly doesn't fulfill the alignment requirement
                        fcntl(m_fd, F_SETFL, O_CREAT | O_WRONLY);
                        failed_write |= !::write(m_fd, current_write_pointer, end_of_buffer_pointer - current_write_pointer);
                        fcntl(m_fd, F_SETFL, m_oflag);
                        if (!failed_write) {
                            bytes_written += end_of_buffer_pointer - current_write_pointer;
                        }
                        current_write_pointer = start_of_buffer_pointer;
                      }
                    }

                    if (current_write_pointer == end_of_buffer_pointer) {
                      current_write_pointer = start_of_buffer_pointer;
                    }
                    
                    if (failed_write) {
                      TLOG() << "Failed write!";
                      ers::warning(CannotWriteToFile(ERS_HERE, inherited::m_output_file));
                    } 
                    considered_chunks_in_loop++;
                    // This expression is "a bit" complicated as it finds the last frame that was written to file completely
                    inherited::m_next_timestamp_to_record = reinterpret_cast<const ReadoutType*>(start_of_buffer_pointer + (((current_write_pointer - start_of_buffer_pointer) / ReadoutType::fixed_payload_size) * ReadoutType::fixed_payload_size))->get_first_timestamp();

                  }
                }
                current_time = std::chrono::high_resolution_clock::now();
              }
	          
              // Complete writing the last frame to file
              if (current_write_pointer != nullptr) {
                const char* last_started_frame = start_of_buffer_pointer + (((current_write_pointer - start_of_buffer_pointer) / ReadoutType::fixed_payload_size) * ReadoutType::fixed_payload_size); 
                if (last_started_frame != current_write_pointer) {
                  fcntl(m_fd, F_SETFL, O_CREAT | O_WRONLY);
                  if (!::write(m_fd, current_write_pointer, (last_started_frame + ReadoutType::fixed_payload_size) - current_write_pointer)) {
                    ers::warning(CannotWriteToFile(ERS_HERE, inherited::m_output_file));
                  } else {
                    bytes_written += (last_started_frame + ReadoutType::fixed_payload_size) - current_write_pointer;
                  }
                }
              }
              ::close(m_fd);

              inherited::m_next_timestamp_to_record = std::numeric_limits<uint64_t>::max(); // NOLINT (build/unsigned)

              TLOG() << "Stopped recording, wrote " << bytes_written << " bytes";
              inherited::m_recording.exchange(false);
            },
            args.get<readoutconfig::RecordingParams>().duration);
      }

    private:
      int m_fd;
      int m_oflag;
    };

  } // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_MODELS_ZEROCOPYRECORDINGREQUESTHANDLERMODEL_HPP_

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

      using base = DefaultRequestHandlerModel<ReadoutType, LatencyBufferType>;

      void record(const nlohmann::json& args) override
      {
        size_t chunk_size = 8388608;
        auto oflag = O_CREAT | O_WRONLY | O_DIRECT;
        auto fd = ::open(base::m_output_file.c_str(), oflag, 0644);

        auto conf = args.get<readoutconfig::RecordingParams>();
        if (base::m_recording.load()) {
          ers::error(CommandError(ERS_HERE, base::m_geoid, "A recording is still running, no new recording was started!"));
          return;
        }
        base::m_recording_thread.set_work(
            [&](int duration) {
              TLOG() << "Start recording for " << duration << " second(s)" << std::endl;
              base::m_recording.exchange(true);
              auto start_of_recording = std::chrono::high_resolution_clock::now();
              auto current_time = start_of_recording;
              base::m_next_timestamp_to_record = 0;
              ReadoutType element_to_search;
              
              const char* current_write_pointer;
              const char* start_of_buffer_pointer = reinterpret_cast<const char*>(base::m_latency_buffer->start_of_buffer());
	      const char* current_end_pointer;
              const char* end_of_buffer_pointer = reinterpret_cast<const char*>(base::m_latency_buffer->end_of_buffer());

              while (std::chrono::duration_cast<std::chrono::seconds>(current_time - start_of_recording).count() < duration) {
                if (!base::m_cleanup_requested || (base::m_next_timestamp_to_record == 0)) {
                  size_t considered_chunks_in_loop = 0;
                  size_t overhang = 0;

                  // Wait for potential running cleanup to finish first
                  {
                    std::unique_lock<std::mutex> lock(base::m_cv_mutex);
                    base::m_cv.wait(lock, [&] { return !base::m_cleanup_requested; });
                  }
                  base::m_cv.notify_all();

                  if (base::m_next_timestamp_to_record == 0) {
                    auto front = base::m_latency_buffer->front();
                    base::m_next_timestamp_to_record = front == nullptr ? 0 : front->get_first_timestamp();
                    current_write_pointer = reinterpret_cast<const char*>(base::m_latency_buffer->front());
                  }

	          current_end_pointer = reinterpret_cast<const char*>(base::m_latency_buffer->back());

                  while (considered_chunks_in_loop < 100) {
                    bool failed_write = false;
                    if (overhang > 0) {
                      size_t write_size = 8192 - (overhang % 8192);
                      if (current_write_pointer + write_size < current_end_pointer) {
                        // Align the file offset
                        TLOG() << "Align";
                        fcntl(fd, F_SETFL, O_CREAT | O_WRONLY);
                        failed_write |= !::write(fd, current_write_pointer, write_size);
                        fcntl(fd, F_SETFL, oflag);
                        current_write_pointer += write_size;
                        overhang = 0;
                      }
                    } else if (current_write_pointer + chunk_size < current_end_pointer) {
                      // Write whole chunk to file
                      TLOG() << "Whole chunk";
                      failed_write |= !::write(fd, current_write_pointer, chunk_size);
                      current_write_pointer += chunk_size;
                    } else if (current_end_pointer < current_write_pointer) {
                      if (current_write_pointer + chunk_size < end_of_buffer_pointer) {
                        // Write whole chunk to file
                        TLOG() << "Whole chunk";
                        failed_write |= !::write(fd, current_write_pointer, chunk_size);
                        current_write_pointer += chunk_size;
                      } else {
                        // Write the last bit of the buffer without using O_DIRECT
                        TLOG() << "Last bit";
                        fcntl(fd, F_SETFL, O_CREAT | O_WRONLY);
                        failed_write |= !::write(fd, current_write_pointer, end_of_buffer_pointer - current_write_pointer);
                        fcntl(fd, F_SETFL, oflag);
                        current_write_pointer = start_of_buffer_pointer;
                        overhang = end_of_buffer_pointer - current_write_pointer;
                      }
                    }

                    if (current_write_pointer == end_of_buffer_pointer) {
                      current_write_pointer = start_of_buffer_pointer;
                    }
                    
                    if (failed_write) {
                      ers::warning(CannotWriteToFile(ERS_HERE, base::m_output_file));
                    } 
                    considered_chunks_in_loop++;
                    base::m_next_timestamp_to_record = reinterpret_cast<const ReadoutType*>(start_of_buffer_pointer + (((current_write_pointer - start_of_buffer_pointer) / ReadoutType::fixed_payload_size) * ReadoutType::fixed_payload_size))->get_first_timestamp();


                  }
                }
                current_time = std::chrono::high_resolution_clock::now();
              }
	      // Complete the last frame
              const char* last_started_frame = start_of_buffer_pointer + (((current_write_pointer - start_of_buffer_pointer) / ReadoutType::fixed_payload_size) * ReadoutType::fixed_payload_size); 
              if (last_started_frame != current_write_pointer) {
                fcntl(fd, F_SETFL, O_CREAT | O_WRONLY);
                ::write(fd, current_write_pointer, (last_started_frame + ReadoutType::fixed_payload_size) - current_write_pointer);
              }
              
              ::close(fd);

              base::m_next_timestamp_to_record = std::numeric_limits<uint64_t>::max(); // NOLINT (build/unsigned)

              TLOG() << "Stop recording" << std::endl;
              base::m_recording.exchange(false);
            },
            conf.duration);
      }
    };

  } // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_MODELS_ZEROCOPYRECORDINGREQUESTHANDLERMODEL_HPP_

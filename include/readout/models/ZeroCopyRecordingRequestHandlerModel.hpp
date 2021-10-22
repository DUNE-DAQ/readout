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

      void record(const nlohmann::json& args) override
      {
        auto conf = args.get<readoutconfig::RecordingParams>();

        if (inherited::m_recording.load()) {
          ers::error(CommandError(ERS_HERE, inherited::m_geoid, "A recording is still running, no new recording was started!"));
          return;
        }
        inherited::m_recording_thread.set_work(
            [&](int duration, const nlohmann::json& args) {
              auto oflag = O_CREAT | O_WRONLY | O_DIRECT;
              auto fd = ::open(inherited::m_output_file.c_str(), oflag, 0644);

              auto conf = args.get<readoutconfig::RecordingParams>();

              size_t chunk_size = inherited::m_stream_buffer_size;
              size_t alignment_size = inherited::m_latency_buffer->get_alignment_size();
              TLOG() << "Start recording for " << duration << " second(s)" << std::endl;
              inherited::m_recording.exchange(true);
              auto start_of_recording = std::chrono::high_resolution_clock::now();
              auto current_time = start_of_recording;
              inherited::m_next_timestamp_to_record = 0;
              ReadoutType element_to_search;
              
              const char* current_write_pointer;
              const char* start_of_buffer_pointer = reinterpret_cast<const char*>(inherited::m_latency_buffer->start_of_buffer());
	      const char* current_end_pointer;
              const char* end_of_buffer_pointer = reinterpret_cast<const char*>(inherited::m_latency_buffer->end_of_buffer());

              size_t bytes_written = 0;

              while (std::chrono::duration_cast<std::chrono::seconds>(current_time - start_of_recording).count() < duration) {
                if (!inherited::m_cleanup_requested || (inherited::m_next_timestamp_to_record == 0)) {
                  size_t considered_chunks_in_loop = 0;
                  size_t overhang = 0;

                  // Wait for potential running cleanup to finish first
                  {
                    std::unique_lock<std::mutex> lock(inherited::m_cv_mutex);
                    inherited::m_cv.wait(lock, [&] { return !inherited::m_cleanup_requested; });
                  }
                  inherited::m_cv.notify_all();

                  if (inherited::m_next_timestamp_to_record == 0) {
                    auto begin = inherited::m_latency_buffer->begin();
                    inherited::m_next_timestamp_to_record = begin == inherited::m_latency_buffer->end() ? 0 : begin->get_first_timestamp();
                    size_t skipped_frames = 0;
                    while (reinterpret_cast<std::uintptr_t>(&(*begin)) % alignment_size) {
                        ++begin;
                        skipped_frames++;
                    }
                    std::cout << "Skipped " << skipped_frames << " frames" << std::endl;
                    current_write_pointer = reinterpret_cast<const char*>(&(*begin));
                  }

	          current_end_pointer = reinterpret_cast<const char*>(inherited::m_latency_buffer->back());
                  //printf("current_write_pointer:   %p\n", (void *) current_write_pointer);
                  //printf("current_end_pointer:     %p\n", (void *) current_end_pointer);
                  //printf("start_of_buffer_pointer: %p\n", (void *) start_of_buffer_pointer);
                  //printf("end_of_buffer_pointer:   %p\n", (void *) end_of_buffer_pointer);

                  while (considered_chunks_in_loop < 100) {
                    auto iptr = reinterpret_cast<std::uintptr_t>(current_write_pointer);
                    if (iptr % alignment_size) {
                       TLOG() << "Write pointer is not aligned";
                    } 
                    bool failed_write = false;
                    if (overhang > 0) {
                      size_t write_size = alignment_size - (overhang % alignment_size);
                      if (current_write_pointer + write_size < current_end_pointer) {
                        // Align the file offset
                        fcntl(fd, F_SETFL, O_CREAT | O_WRONLY);
                        failed_write |= !::write(fd, current_write_pointer, write_size);
                        fcntl(fd, F_SETFL, oflag);
                        bytes_written += write_size;
                        current_write_pointer += write_size;
                        overhang = 0;
                      }
                    } else if (current_write_pointer + chunk_size < current_end_pointer) {
                      // Write whole chunk to file
                      failed_write |= !::write(fd, current_write_pointer, chunk_size);
                      bytes_written += chunk_size;
                      current_write_pointer += chunk_size;
                    } else if (current_end_pointer < current_write_pointer) {
                      if (current_write_pointer + chunk_size < end_of_buffer_pointer) {
                        // Write whole chunk to file
                        failed_write |= !::write(fd, current_write_pointer, chunk_size);
                        //std::cout << "Bytes written: " << bytes_written << std::endl;
                        bytes_written = bytes_written + chunk_size;
                        //std::cout << "Added here: " << chunk_size << std::endl;
                        //std::cout << "Bytes written after: " << bytes_written << std::endl;
                        current_write_pointer += chunk_size;
                      } else {
                        // Write the last bit of the buffer without using O_DIRECT
                        fcntl(fd, F_SETFL, O_CREAT | O_WRONLY);
                        failed_write |= !::write(fd, current_write_pointer, end_of_buffer_pointer - current_write_pointer);
                        fcntl(fd, F_SETFL, oflag);
                        //std::cout << "Bytes written: " << bytes_written << std::endl;
                        bytes_written = bytes_written + end_of_buffer_pointer - current_write_pointer;
                        //std::cout << "Added there: " << end_of_buffer_pointer - current_write_pointer << std::endl;
                        //std::cout << "Bytes written after: " << bytes_written << std::endl;
                        current_write_pointer = start_of_buffer_pointer;
                        overhang = end_of_buffer_pointer - current_write_pointer;
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
                    inherited::m_next_timestamp_to_record = reinterpret_cast<const ReadoutType*>(start_of_buffer_pointer + (((current_write_pointer - start_of_buffer_pointer) / ReadoutType::fixed_payload_size) * ReadoutType::fixed_payload_size))->get_first_timestamp();
                    //TLOG() << "Next timestamp to record: " << inherited::m_next_timestamp_to_record;

                  }
                }
                current_time = std::chrono::high_resolution_clock::now();
              }
	      // Complete the last frame
              const char* last_started_frame = start_of_buffer_pointer + (((current_write_pointer - start_of_buffer_pointer) / ReadoutType::fixed_payload_size) * ReadoutType::fixed_payload_size); 
              if (last_started_frame != current_write_pointer) {
                fcntl(fd, F_SETFL, O_CREAT | O_WRONLY);
                ::write(fd, current_write_pointer, (last_started_frame + ReadoutType::fixed_payload_size) - current_write_pointer);
                bytes_written += (last_started_frame + ReadoutType::fixed_payload_size) - current_write_pointer;
              }
              
              ::close(fd);

              inherited::m_next_timestamp_to_record = std::numeric_limits<uint64_t>::max(); // NOLINT (build/unsigned)

              TLOG() << "Stop recording, wrote " << bytes_written << " bytes";
              inherited::m_recording.exchange(false);
            },
            conf.duration, args);
      }
    };

  } // namespace readout
} // namespace dunedaq

#endif // READOUT_INCLUDE_READOUT_MODELS_ZEROCOPYRECORDINGREQUESTHANDLERMODEL_HPP_

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
        } else if (!base::m_buffered_writer.is_open()) {
          ers::error(CommandError(ERS_HERE, base::m_geoid, "DLH is not configured for recording"));
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
              while (std::chrono::duration_cast<std::chrono::seconds>(current_time - start_of_recording).count() < duration) {
                if (!base::m_cleanup_requested || (base::m_next_timestamp_to_record == 0)) {
                  if (base::m_next_timestamp_to_record == 0) {
                    auto front = base::m_latency_buffer->front();
                    base::m_next_timestamp_to_record = front == nullptr ? 0 : front->get_first_timestamp();
                  }
                  size_t processed_chunks_in_loop = 0;

                  // Wait for potential running cleanup to finish first
                  {
                    std::unique_lock<std::mutex> lock(base::m_cv_mutex);
                    base::m_cv.wait(lock, [&] { return !base::m_cleanup_requested; });
                  }
                  base::m_cv.notify_all();

                  auto iter = base::m_latency_buffer->front();
                  auto end = base::m_latency_buffer->end();
                  auto end_of_memory = base::m_latency_buffer->

                  for (; chunk_iter != end && chunk_iter.good() && processed_chunks_in_loop < 100000;) {
                    if ((*chunk_iter).get_first_timestamp() >= base::m_next_timestamp_to_record) {
                      if (!base::m_buffered_writer.write(reinterpret_cast<char*>(chunk_iter->begin()), chunk_iter->get_payload_size())) {
                        ers::warning(CannotWriteToFile(ERS_HERE, base::m_output_file));
                      }
                      base::m_payloads_written++;
                      processed_chunks_in_loop++;
                      base::m_next_timestamp_to_record =
                          (*chunk_iter).get_first_timestamp() + ReadoutType::expected_tick_difference * (*chunk_iter).get_num_frames();
                    }
                    ++chunk_iter;
                  }
                }
                current_time = std::chrono::high_resolution_clock::now();
              }
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

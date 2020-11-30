// From Module
#include "ContinousLatencyBuffer.hpp"
#include "ReadoutTypes.hpp"

#include <ers/ers.h>

// From STD
#include <iomanip>
#include <chrono>

using namespace std::chrono_literals;

namespace dunedaq {
namespace readout {

/*
inline void 
dump_to_buffer(const char* data, long unsigned size,
                      void* buffer, long unsigned buffer_pos, 
                      const long unsigned& buffer_size) {
  long unsigned bytes_to_copy = size;
  while(bytes_to_copy > 0) {
    unsigned int n = std::min(bytes_to_copy, buffer_size-buffer_pos);
    memcpy(static_cast<char*>(buffer)+buffer_pos, data, n);
    buffer_pos += n;
    bytes_to_copy -= n;
    if (buffer_pos == buffer_size) {
      buffer_pos = 0;                                
    }                            
  } 
}

template<> //class RawDataType>
void
ContinousLatencyBuffer<types::WIB_SUPERCHUNK_STRUCT>::write_data_to_buffer(const felix::packetformat::chunk& chunk)
{
  long unsigned bytes_copied_chunk = 0;
  auto subchunk_data = chunk.subchunks();
  auto subchunk_sizes = chunk.subchunk_lengths();
  unsigned n_subchunks = chunk.subchunk_number();

  //RawDataType data;
  types::WIB_SUPERCHUNK_STRUCT superchunk;
  // CROSS CHECK LENGTH
  for(unsigned i=0; i<n_subchunks; i++) {
    dump_to_buffer(subchunk_data[i], subchunk_sizes[i],
      static_cast<void*>(&superchunk), bytes_copied_chunk, chunk.length());
    bytes_copied_chunk += subchunk_sizes[i];
  }
  this->write( std::move(superchunk)  );
}

template<> //class RawDataType>
void
ContinousLatencyBuffer<std::vector<uint8_t>>::write_data_to_buffer(const felix::packetformat::chunk& chunk)
{
  std::cout << " NO KNOWN CONVERSION OF CHUNK VECTOR. \n"; 
}
*/

}
} // namespace dunedaq::readout

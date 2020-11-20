/*
 * This file is 100% generated.  Any manual edits will likely be lost.
 *
 * This contains struct and other type definitions for shema in 
 * namespace dunedaq::readout::cardreader.
 */
#ifndef DUNEDAQ_READOUT_CARDREADER_STRUCTS_HPP
#define DUNEDAQ_READOUT_CARDREADER_STRUCTS_HPP

#include <cstdint>


namespace dunedaq::readout::cardreader {

    // @brief A count of not too many things
    using Count = int32_t;

    // @brief Upstream FELIX CardReader DAQ Module Configuration
    struct Conf {

        // @brief Physical card identifier (in the same host)
        Count card_id;

        // @brief Superlogic region of selected card
        Count card_offset;

        // @brief DMA descriptor to use
        Count dma_id;

        // @brief CMEM_RCC NUMA region selector
        Count numa_id;

        // @brief Read a single superlogic region, or both
        Count num_sources;

        // @brief Number of elinks configured
        Count num_links;
    };

    // @brief A count of very many things
    using Size = uint64_t;

} // namespace dunedaq::readout::cardreader

#endif // DUNEDAQ_READOUT_CARDREADER_STRUCTS_HPP

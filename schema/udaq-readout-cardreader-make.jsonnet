// hand written helpers to make object compliant with udaq-readout-cardreader
{
    // The internally known name of the only queue used
    queue: "blocks-0", 
           "blocks-64", 
           "blocks-128", 
           "blocks-192", 
           "blocks-256", 
           "blocks-320"

    // Make a conf object for cardreader
    conf(cid, coff, did, nid, nums, numl) :: {
        card_id: cid, 
        card_offset: coff, 
        dma_id: did, 
        numa_id: nid,
        num_sources: nums, 
        num_links: numl
    },
}


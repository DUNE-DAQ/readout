// hand written helpers to make object compliant with readout-CardReader
{
    // The internally known name of the only queue used
    queue: "fake-elink"

    // Make a conf object for cardreader
    conf(lid=0, inplim=10485100, rate=166, rawtype="wib", datfile="/tmp/frames.bin", qtms=2000, st0=-1) :: {
        link_ids: [lid],
        input_limit: inplim, 
        rate_khz: rate, 
        data_filename: datfile, 
        queue_timeout_ms: qtms,
        set_t0_to: st0
    },
}


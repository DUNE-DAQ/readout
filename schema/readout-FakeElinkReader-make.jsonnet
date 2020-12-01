// hand written helpers to make object compliant with readout-CardReader
{
    // The internally known name of the only queue used
    queue: "fake-elink"

    // Make a conf object for cardreader
    conf(lid, inplim, rate, rawtype, datfile, qtms) :: {
        link_id: lid,
        input_limit: inplim, 
        rate_khz: rate, 
        data_filename: datfile, 
        queue_timeout_ms: qtms
    },
}


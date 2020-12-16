// hand written helpers to make object compliant with appfwk-fdp-schema
{
    // The internally known name of the only queue used
    queue: "input",

    // Make a conf object for FDP
    conf(rtype, sqtms=2000, lbs=100000, plpct=0.5, pspct=0.8) :: {
        raw_type: rtype,
        source_queue_timeout_ms: sqtms,
        latency_buffer_size: lbs,
        pop_limit_pct: plpct,
        pop_size_pct: pspct
    },
}


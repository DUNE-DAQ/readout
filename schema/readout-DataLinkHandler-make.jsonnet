// hand written helpers to make object compliant with appfwk-fdp-schema
{
    // The internally known name of the only queue used
    queue: "input",

    // Make a conf object for FDP
    conf(rtype, sqtms) :: {
        raw_type: rtype,
        source_queue_timeout_ms: sqtms
    },
}


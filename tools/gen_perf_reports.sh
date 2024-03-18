#!/bin/bash

PERF_REPORT="perf.data"

perf_mem_reports() {
    # Assumes we ran "perf mem record $PROGRAM"

    if [ ! -f "./$PERF_REPORT" ]; then
        echo "ERROR: no perf report found ($PERF_REPORT)."
        exit 1
    fi

    # Change ownership
    sudo chown $USER:$USER $PERF_REPORT

    # Generate the general report
    perf mem report --stdio > report.txt

    # Sort by samples
    perf mem report --sort=sample --stdio > report-sorted-by-sample.txt

    # Sort by symbols
    perf mem report --sort=symbol --stdio > report-sorted-by-symbol.txt

    # Sort by mem
    perf mem report --sort=mem --stdio > report-sorted-by-mem.txt
}

perf_mem_reports
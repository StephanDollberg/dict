#!/usr/bin/env bash

set -x

for branch in "$@"
do
    git co $branch
    make clean ; make;
    ./test_main --benchmark_repetitions=3 \
    			--benchmark_report_aggregates_only=true \
    			--benchmark_out=$branch.json \
    			--benchmark_out_format=json \
    			--benchmark_filter=dict_
done

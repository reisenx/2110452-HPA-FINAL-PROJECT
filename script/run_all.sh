#!/bin/bash

# DEV NOTE: PLEASE RUN THIS AT THE PROJECT ROOT DIRECTORY

mkdir -p logs
mkdir -p output

FILENAMES=(
    "run_baseline"
    "run_optimize_v1"
    "run_optimize_v2"
    "run_optimize_v3"
    "run_optimize_v4"
    "run_optimize_v5"
    "run_optimize_v6"
    "run_optimize_v7"
    "run_optimize_v8"
    "run_solution"
)

for FILENAME in "${FILENAMES[@]}"
do
    ./script/$FILENAME.sh
done

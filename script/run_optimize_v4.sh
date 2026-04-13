#!/bin/bash

# DEV NOTE: PLEASE RUN THIS AT THE PROJECT ROOT DIRECTORY

mkdir -p logs
mkdir -p output

CODE="optimize_v4"
LOG_FILE="logs/$CODE.txt"

FILENAMES=(
    "grid-6-7"
    "grid-9-12"
    "grid-12-17"
    "grid-16-24"
    "grid-20-31"
    "grid-25-40"
    "grid-30-49"
    "grid-40-67"
    "grid-49-84"
    "grid-56-97"
    "grid-60-104"
    # "grid-72-127"
    # "grid-81-144"
    # "grid-100-180"
    "rand-5-7"
    "rand-10-15"
    "rand-15-28"
    "rand-20-40"
    "rand-25-50"
    "rand-30-60"
    "rand-30-120"
    "rand-35-140"
    "rand-40-80"
    "rand-40-160"
    "rand-45-180"
    "rand-50-200"
    "rand-60-250"
    # "rand-70-300"
    # "rand-80-350"
    "ring-5-5"
    "ring-10-10"
    "ring-15-15"
    "ring-20-20"
    "ring-25-25"
    "ring-30-30"
    "ring-35-35"
    "ring-40-40"
    "ring-50-50"
    "ring-60-60"
    # "ring-75-75"
    # "ring-100-100"
    "spec-1"
    "spec-2"
    "spec-3"
    "spec-4"
    "spec-5"
    "spec-6"
    "spec-7"
    "tree-5-4"
    "tree-10-9"
    "tree-15-14"
    "tree-20-19"
    "tree-25-24"
    "tree-30-29"
    "tree-35-34"
    "tree-40-39"
    "tree-50-49"
    "tree-60-59"
    # "tree-75-74"
    # "tree-100-99"
)

> "$LOG_FILE"
{
    printf "\nCompiling the code...\n"

    g++ -O3 -mavx2 src/$CODE.cpp -o script/$CODE

    printf "%s\n==============================\n"
    printf "$CODE.cpp RUNTIME RESULTS\n"
    date
    printf "%s==============================\n"
    for FILENAME in "${FILENAMES[@]}"
    do
        printf "Running $FILENAME.txt"
        time ./script/$CODE input/$FILENAME.txt output/$FILENAME.out

        printf "\nResult of $FILENAME.out\n"
        cat output/$FILENAME.out
        printf "%s------------------------------\n"
    done
} 2>&1 | tee -a "$LOG_FILE"

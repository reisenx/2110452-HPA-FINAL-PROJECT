#!/bin/bash

# DEV NOTE: PLEASE RUN THIS AT THE PROJECT ROOT DIRECTORY

set -e

DOCKER_IMAGE_NAME="hpa-final-project"
DOCKER_FILE_PATH="build/Dockerfile"

printf "Creating Docker image '$DOCKER_IMAGE_NAME'...\n"

docker build -f $DOCKER_FILE_PATH -t $DOCKER_IMAGE_NAME .

printf "Successfully create '$DOCKER_IMAGE_NAME' Docker image\n"

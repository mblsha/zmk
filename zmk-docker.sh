#!/bin/bash
# Helper script to run ZMK build commands in Docker

# Check if we need interactive mode
if [ -t 0 ]; then
  DOCKER_FLAGS="-it"
else
  DOCKER_FLAGS=""
fi

docker run --rm $DOCKER_FLAGS \
  -v "$PWD":/work \
  -w /work \
  -e USER_ID=$(id -u) \
  -e GROUP_ID=$(id -g) \
  zmkfirmware/zmk-build-arm:stable \
  "$@"
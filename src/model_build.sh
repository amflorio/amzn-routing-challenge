#!/bin/sh
readonly BASE_DIR=$(dirname $0)
readonly OUT_FILE="$(dirname ${BASE_DIR})/data/model_build_outputs/model.json"

cd src/cpp
make clean
make -f Makefile.linux -j3
cd ../../
src/cpp/main 0      # 0: model_build

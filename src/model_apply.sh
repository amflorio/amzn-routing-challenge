#!/bin/sh
readonly BASE_DIR=$(dirname $0)
readonly OUTPUTS_DIR="$(dirname ${BASE_DIR})/data/model_apply_outputs"

# Remove any old solution if it exists
rm -rf ${OUTPUTS_DIR}/proposed_sequences.json 2> /dev/null

cd src/cpp
make clean
make -f Makefile.linux -j3
cd ../../
src/cpp/main 1      # 1: model_apply

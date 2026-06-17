#!/usr/bun/env bash

set -e

bash setupPlugins.sh

cd guest
bash config.sh genconfig
mkdir build
cd build
cmake ..
make
cd ../../

python3 premap.py

cd WorkhorseRT
bash config.sh genconfig
mkdir build
cd build
cmake ..
make runKvm
cd ../../
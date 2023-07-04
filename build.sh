#!/bin/bash

sudo apt-get install libglfw3-dev

mkdir build
cd build
cmake ..
make -j

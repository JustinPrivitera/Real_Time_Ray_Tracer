#!/bin/bash

rm *.ppm
rm -rf build
mkdir build
cd build
cmake ..
make -j8
timeout -s SIGKILL 1000 ./lab3
cd ../


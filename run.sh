#!/bin/bash

rm *.ppm
rm -rf build
mkdir build
cd build
cmake ..
make -j8
timeout -s SIGKILL 10 ./lab3
cd ../


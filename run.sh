#!/bin/bash

rm -rf build
mkdir build
cd build
cmake ..
make -j8
./lab3
cd ../


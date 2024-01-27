#!/bin/bash

if [[ ! -d build ]]
then
  mkdir build
fi
cd build
rm -rf *
cmake ../diet_csound
make
sudo make install
sudo ldconfig

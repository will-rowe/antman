#!/usr/bin/env bash

# a helper script to download, configure and compile an fswatch release
 
mkdir fswatch && cd $_
wget -O fswatch.tar.gz https://github.com/emcrisostomo/fswatch/releases/download/1.13.0/fswatch-1.13.0.tar.gz
tar -xvf fswatch.tar.gz -C ./ --strip-components 1
./configure CFLAGS="-g -Wall -O2 -Wc++-compat -std=c99"
make
make install
cd ..
rm -rf fswatch*
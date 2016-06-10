#!/bin/sh
cd /Users/hl/Github/os161-base-2.0.2
echo "Configuring userland..."
./configure
echo "Making kernel ... "
bmake
bmake install

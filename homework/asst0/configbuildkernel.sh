#!/bin/sh
cd /Users/hl/Github/os161-base-2.0.2/kern/conf
echo "Configuring dumbvm ..."
./config DUMBVM
echo "Making kernel ... "
cd ../compile/DUMBVM
bmake depend
bmake
bmake install

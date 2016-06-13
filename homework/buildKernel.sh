#!/bin/sh
cd /Users/hl/Github/os161-base-2.0.2/kern/conf
echo "Configuring dumbvm ..."
./config $1
echo "Making kernel ... "
cd ../compile/$1
bmake depend
bmake
bmake install


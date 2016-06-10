# DONE
# brew install mpfr gmp

# DONE Gcc 4.8
cd gcc-4.8.3+os161-2.1
find . -name '*.info' | xargs touch
touch intl/plural.c
cd ..
mkdir buildgcc
cd buildgcc
../gcc-4.8.3+os161-2.1/configure \
--enable-languages=c,lto \
--nfp --disable-shared --disable-threads \
--disable-libmudflap --disable-libssp \
--disable-libstdcxx --disable-nls \
--target=mips-harvard-os161 \
--prefix=$HOME/os161/tools \
 --with-gmp=/usr/local/Cellar/gmp/ --with-mpfr=/usr/local/Cellar/mpfr/ \
--with-mpc=/usr/local/Cellar/libmpc/1.0.3
cd ..

cd buildgcc
make
make install
cd ..

# GDB
cd gdb-7.8+os161-2.1
find . -name '*.info' | xargs touch
touch intl/plural.c
cd ..

cd gdb-7.8+os161-2.1
./configure --target=mips-harvard-os161 --prefix=$HOME/os161/tools
make
make install
cd ..

# GDB build instruction failed. 
# try instead https://github.com/benesch/homebrew-os161

# DONE Installing System/161
tar -xf sys161-2.0.8.tar
cd sys161-2.0.8
./configure --prefix=$HOME/os161/tools mipseb
make
make install
cd ..

# DONE Bmake
tar -xf bmake-20101215.tar
cd bmake
tar -xvf ../mk-20100612.tar
cd ..

cd bmake
./configure --prefix=$HOME/os161/tools --with-default-sys-path=$HOME/os161/tools/share/mk
sh ./make-bootstrap.sh
mkdir -p $HOME/os161/tools/bin
mkdir -p $HOME/os161/tools/share/man/man1
mkdir -p $HOME/os161/tools/share/mk
cp bmake $HOME/os161/tools/bin/
cp bmake.1 $HOME/os161/tools/share/man/man1/
sh mk/install-mk $HOME/os161/tools/share/mk
cd ..

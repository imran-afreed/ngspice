#!/bin/bash
# ngspice build script for release version of shared ngspice, 32 or 64 bit
# compile_shared.sh
# tested with TDM MINGW or MINGW-w64, or LINUX

#Procedure:
# Install plus bison, flex, auto tools, perl, libiconv, libintl
# For Windows: Install MSYS2, MINGW-w64, activate OpenMP support
#     See either http://mingw-w64.sourceforge.net/ or http://tdm-gcc.tdragon.net/
#     (allows to generate either 32 or 64 bit executables by setting flag -m32 or -m64)
# msys: set path to compiler in msys/xx/etc/fstab (e.g. c:/MinGW64 /mingw)
# msys2: set path variable to gcc in msys2
# start compiling with
# './compile_shared.sh' or './compile_shared.sh 64'

# LINUX users: remove (or check) entry 'prefix="..." in calls ../configure ... (see below).

# Options:
# --adms and --enable-adms will install extra HICUM, EKV and MEXTRAM models via the 
# adms interface.
# Please see http://ngspice.sourceforge.net/admshowto.html for more info on adms.
# CIDER, XSPICE, and OpenMP may be selected at will.
# --disable-debug will give O2 optimization (versus O0 for debug) and removes all debugging info.


if test "$1" = "64"; then
   if [ ! -d "release64-sh" ]; then
      mkdir release64-sh
      if [ $? -ne 0 ]; then  echo "mkdir release64-sh failed"; exit 1 ; fi
   fi   
else
   if [ ! -d "release-sh" ]; then
      mkdir release-sh
      if [ $? -ne 0 ]; then  echo "mkdir release-sh failed"; exit 1 ; fi
   fi
fi

# If compiling sources from git, you may need to uncomment the following two lines:
./autogen.sh
if [ $? -ne 0 ]; then  echo "./autogen.sh failed"; exit 1 ; fi

# Alternatively, if compiling sources from git, and want to add adms created devices,
# you may need to uncomment the following two lines (and don't forget to add adms option
# to the ../configure statement):
#./autogen.sh --adms
#if [ $? -ne 0 ]; then  echo "./autogen.sh failed"; exit 1 ; fi

# In the following ../configure commands you will find an additional entry to the CFLAGS
# '-fno-omit-frame-pointer'. This entry compensates for a compiler bug of actual mingw gcc 4.6.1. 

#libtool versioning current:revision:age
#current The most recent interface number that this library implements.
#revision The implementation number of the current interface.
#age The difference between the newest and oldest interfaces that this library implements.
#In other words, the library implements all the interface numbers in the
#range from numbers (current - age) to current.
#libtool in msys2 generates a single version number (current - age)
#libtool in LINUX generates the numbers as: (current-age).age.revision

echo
if test "$1" = "64"; then
   cd release64-sh
   if [ $? -ne 0 ]; then  echo "cd release64-sh failed"; exit 1 ; fi
  echo "configuring for 64 bit"
  echo
# You may add  --enable-adms to the following command for adding adms generated devices 
  ../configure --with-ngshared --enable-xspice --enable-cider --enable-openmp --enable-relpath --disable-debug prefix="C:/Progra~1/KiCad" CFLAGS="-m64 -O2"  LDFLAGS="-m64 -s" LIB_VERSION="1:0:0"
else
   cd release-sh
   if [ $? -ne 0 ]; then  echo "cd release-sh failed"; exit 1 ; fi
  echo "configuring for 32 bit"
  echo
# You may add  --enable-adms to the following command for adding adms generated devices 
  ../configure --with-ngshared  --enable-xspice --enable-cider --enable-openmp --enable-relpath --disable-debug prefix="C:/Spice" CFLAGS="-m32 -O2" LDFLAGS="-m32 -s" LIB_VERSION="1:0:0"
fi
if [ $? -ne 0 ]; then  echo "../configure failed"; exit 1 ; fi

echo
# make clean is required for properly making the code models
echo "cleaning (see make_clean.log)"
make clean 2>&1 -j8 | tee make_clean.log
exitcode=${PIPESTATUS[0]}
if [ $exitcode -ne 0 ]; then  echo "make clean failed"; exit 1 ; fi
echo "compiling (see make.log)"
make 2>&1 -j8 | tee make.log
exitcode=${PIPESTATUS[0]}
if [ $exitcode -ne 0 ]; then  echo "make failed"; exit 1 ; fi
# 32 bit: Install to C:\Spice
# 64 bit: Install to C:\Spice64
echo "installing (see make_install.log)"
make install 2>&1 | tee make_install.log 
exitcode=${PIPESTATUS[0]}
if [ $exitcode -ne 0 ]; then  echo "make install failed"; exit 1 ; fi

echo "success"
exit 0

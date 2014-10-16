# Choose cc or c++ based on the executable name.
compiler=$0
if [ "${compiler#*++}" != "$compiler" ]; then
    cc=c++
else
    cc=cc
fi
ccpath=$base/bin/$cc

# Construct Clang arguments.
if [ `uname` = "Darwin" ]; then
    libext=dylib
else
    libext=so
fi

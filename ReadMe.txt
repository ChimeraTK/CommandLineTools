Compiling with cmake:

1. Create a build directory. I usually create it inside the source directory 
   which technically is an out of source build, though actually the 
   directory is in the source tree, but the build files are not mixing with
   the source.
   Change into the build directory afterward

$ mkdir build
$ cd build

2. Run CMake to create the Makefile. Pay attention to the two dots. They must 
   be present because they point to the source directory with the
   CMakeLists.txt file.

$ cmake ..

   In case mtca4u is not installed as a system library you probably have
   to define the path to MtcaMappedDevice

$ cmake .. -DMtcaMappedDeviceDir=${HOME}/mtca4u/00.19.00/MtcaMappedDevice/00.15.00

   By default, the installation will happen to the bin directory of the
   source tree. If you want to specify a different directory, specify the
   CMAKE_INSTALL_PREFIX variable. Note that the binary will be places in 
   a 'bin' directory inside of this install path.

$ cmake .. -DCMAKE_INSTALL_PREFIX=${HOME}

   The previous example will install the mtca4u exetuable to ${HOME}/bin/mtca4u

3. Compile and install

$ make
$ make install

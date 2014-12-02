INCLUDE(CMakeForceCompiler)

SET(CMAKE_SYSTEM_NAME Linux) # this one is important
SET(CMAKE_SYSTEM_VERSION 1)  # this one not so much

SET(CMAKE_C_COMPILER   $ENV{RPI_ROOT}/bin/arm-linux-gnueabihf-gcc)
SET(CMAKE_CXX_COMPILER $ENV{RPI_ROOT}/bin/arm-linux-gnueabihf-g++)
SET(CMAKE_AR           $ENV{RPI_ROOT}/bin/arm-linux-gnueabihf-ar)
SET(CMAKE_LINKER       $ENV{RPI_ROOT}/bin/arm-linux-gnueabihf-ld)
SET(CMAKE_NM           $ENV{RPI_ROOT}/bin/arm-linux-gnueabihf-nm)
SET(CMAKE_OBJCOPY      $ENV{RPI_ROOT}/bin/arm-linux-gnueabihf-objcopy)
SET(CMAKE_OBJDUMP      $ENV{RPI_ROOT}/bin/arm-linux-gnueabihf-objdump)
SET(CMAKE_STRIP        $ENV{RPI_ROOT}/bin/arm-linux-gnueabihf-strip)
SET(CMAKE_RANLIB       $ENV{RPI_ROOT}/bin/arm-linux-gnueabihf-tanlib)

# where is the target environment 
SET(CMAKE_FIND_ROOT_PATH  $ENV{RPI_ROOT}/arm-linux-gnueabihf)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

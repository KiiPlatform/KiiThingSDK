INCLUDE(CMakeForceCompiler)
ADD_DEFINITIONS("-Dinline=")
SET(CMAKE_SYSTEM_NAME Linux) # this one is important
SET(CMAKE_SYSTEM_VERSION 1)  # this one not so much

# where is the target environment 
SET(CMAKE_FIND_ROOT_PATH  /Volumes/CrossTool2NG/install/arm-unknown-linux-gnueabi) #change this with your cross compiling environment

SET(JANSSON_IMPORTED_LIB ${CMAKE_FIND_ROOT_PATH}/lib/libjansson.so) #define where is libjansson located

SET(CMAKE_C_COMPILER   ${CMAKE_FIND_ROOT_PATH}/bin/arm-linux-gnueabihf-gcc)
SET(CMAKE_CXX_COMPILER ${CMAKE_FIND_ROOT_PATH}/bin/arm-linux-gnueabihf-g++)
SET(CMAKE_AR           ${CMAKE_FIND_ROOT_PATH}/bin/arm-linux-gnueabihf-ar)
SET(CMAKE_LINKER       ${CMAKE_FIND_ROOT_PATH}/bin/arm-linux-gnueabihf-ld)
SET(CMAKE_NM           ${CMAKE_FIND_ROOT_PATH}/bin/arm-linux-gnueabihf-nm)
SET(CMAKE_OBJCOPY      ${CMAKE_FIND_ROOT_PATH}/bin/arm-linux-gnueabihf-objcopy)
SET(CMAKE_OBJDUMP      ${CMAKE_FIND_ROOT_PATH}/bin/arm-linux-gnueabihf-objdump)
SET(CMAKE_STRIP        ${CMAKE_FIND_ROOT_PATH}/bin/arm-linux-gnueabihf-strip)
SET(CMAKE_RANLIB       ${CMAKE_FIND_ROOT_PATH}/bin/arm-linux-gnueabihf-tanlib)


# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

#extra configuration for building jansson
SET(CONFIGURE_EXTRA_ARG CC=${CMAKE_C_COMPILER} CROSS_COMPILER=arm-linux-gnueabihf- --host=arm-unknown-linux-gnueabihf --prefix=${CMAKE_FIND_ROOT_PATH} CHOST=arm-linux-gnueabihf NM=${CMAKE_NM} )
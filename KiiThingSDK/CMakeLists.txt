PROJECT(KiiThingSDK)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
if (NOT DEFINED CMAKE_MACOSX_RPATH)
  set(CMAKE_MACOSX_RPATH 0)
endif()
FILE(GLOB KiiThingSDK_files *.c *.h)
SET(KiiThingSDK_src
    ${KiiThingSDK_files}
    )
    
ADD_DEFINITIONS("-D_POSIX_C_SOURCE=200809L") #fix warning related on strndup http://stackoverflow.com/questions/9168150/implicit-declaration-of-strndup

if (BUILD_JANSSON)
    ExternalProject_Add(project_jansson
      URL http://www.digip.org/jansson/releases/jansson-2.7.tar.gz
      PREFIX ${CMAKE_CURRENT_BINARY_DIR}/jansson-2.7
      BUILD_IN_SOURCE 1
      UPDATE_COMMAND  autoreconf -vi
      CONFIGURE_COMMAND ./configure ${CONFIGURE_EXTRA_ARG}
      BUILD_COMMAND make
    )
    ExternalProject_Get_Property(project_jansson install_dir)
    SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 

    SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

    if (NOT DEFINED JANSSON_IMPORTED_LIB)
      set(JANSSON_IMPORTED_LIB ${JANSSON_LIBRARIES})
    endif()
    add_library(Jansson SHARED IMPORTED)
    set_property(TARGET Jansson PROPERTY IMPORTED_LOCATION ${CMAKE_INSTALL_RPATH}/libjansson${CMAKE_SHARED_LIBRARY_SUFFIX})
    add_dependencies(Jansson project_jansson)

    ADD_LIBRARY(kii SHARED ${KiiThingSDK_src})

    TARGET_LINK_LIBRARIES(kii ${CURL_LIBRARIES} Jansson)
else()
    ADD_LIBRARY(kii SHARED ${KiiThingSDK_src})

    TARGET_LINK_LIBRARIES(kii ${CURL_LIBRARIES} ${JANSSON_LIBRARIES})
endif()


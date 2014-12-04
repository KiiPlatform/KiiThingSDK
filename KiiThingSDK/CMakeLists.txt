PROJECT(KiiThingSDK)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
if (NOT DEFINED CMAKE_MACOSX_RPATH)
  set(CMAKE_MACOSX_RPATH 0)
endif()
FILE(GLOB KiiThingSDK_files *.c *.h)
SET(KiiThingSDK_src
    ${KiiThingSDK_files}
    )
if(DEFINED NO_INLINE)
    ADD_DEFINITIONS("-Dinline=")
endif()

if (BUILD_JANSSON)
    ExternalProject_Add(project_jansson
      URL http://www.digip.org/jansson/releases/jansson-2.6.tar.gz
      PREFIX ${CMAKE_CURRENT_BINARY_DIR}/jansson-2.6
      BUILD_IN_SOURCE 1
      UPDATE_COMMAND  autoreconf -vi
      CONFIGURE_COMMAND ./configure ${CONFIGURE_EXTRA_ARG}
      BUILD_COMMAND make
    )
    ExternalProject_Get_Property(project_jansson install_dir)
    #include_directories(${CMAKE_CURRENT_SOURCE_DIR}/jansson)
    if (NOT DEFINED JANSSON_IMPORTED_LIB)
      set(JANSSON_IMPORTED_LIB ${JANSSON_LIBRARIES})
    endif()
    add_library(Jansson SHARED IMPORTED)
    set_property(TARGET Jansson PROPERTY IMPORTED_LOCATION ${JANSSON_IMPORTED_LIB})
    add_dependencies(Jansson project_jansson)


    ADD_LIBRARY(kii SHARED ${KiiThingSDK_src})

    TARGET_LINK_LIBRARIES(kii ${CURL_LIBRARIES} Jansson)
else()
    ADD_LIBRARY(kii SHARED ${KiiThingSDK_src})

    TARGET_LINK_LIBRARIES(kii ${CURL_LIBRARIES} ${JANSSON_LIBRARIES})
endif()


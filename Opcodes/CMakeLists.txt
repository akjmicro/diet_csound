option(BUILD_DSSI_OPCODES "Build the DSSI opcodes" OFF)
option(BUILD_OSC_OPCODES "Build the OSC Opcodes" OFF)
option(BUILD_PADSYNTH_OPCODES "Build the Padsynth opcode" OFF)
option(BUILD_SCANSYN_OPCODES "Build the scansyn opcodes" OFF)
option(BUILD_DEPRECATED_OPCODES "Build deprecated opcodes" OFF)

##########################################
## Plugin opcodes are in the following categories ##
# 1) opcodes in C++ with no external dependencies
# 2) opcodes with special licensing conditions
# 3) platform-specific opcodes
# 4) opcodes with external dependencies
# 5) deprecated opcodes
#
#  if an opcode is not in one of the categories above,
#  it should be added with BUILTIN linkage
#  (i.e. not as plugins)
#  See instructions in ../Top/csmodule.c
##########################################

message(STATUS "## Building Plugin Opcodes ##")
## CPP OPCODE LIBS WITH NO EXTERNAL DEPENDENCIES ##
make_plugin(fractalnoise tl/fractalnoise.cpp)

if(APPLE)
   #make_plugin(arrayops arrayops.cpp)
   #make_plugin(trigenvsegs trigEnvSegs.cpp)
elseif(LINUX)
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("-std=gnu++11" COMPILER_SUPPORTS_CXX11)
     if(COMPILER_SUPPORTS_CXX11)
       #make_plugin(arrayops arrayops.cpp)
       #set_target_properties(arrayops PROPERTIES COMPILE_FLAGS "-std=gnu++11")
       #make_plugin(trigenvsegs trigEnvSegs.cpp)
       #set_target_properties(trigenvsegs PROPERTIES COMPILE_FLAGS "-std=gnu++11")
     else()
        message(STATUS "Not building array operators as no C++11 support found.")
     endif()
else()
    #make_plugin(arrayops arrayops.cpp)
    #make_plugin(trigenvsegs trigEnvSegs.cpp)
    if(WIN32 AND NOT MSVC)
      #set_target_properties(arrayops PROPERTIES COMPILE_FLAGS "-std=gnu++11")
      #set_target_properties(trigenvsegs PROPERTIES COMPILE_FLAGS "-std=gnu++11")
      endif()
endif()


## OPCODES WITH EXTERNAL DEPENDENCIES ##
find_package(LIBLO)
if(BUILD_OSC_OPCODES AND LIBLO_FOUND)
    make_plugin(osc OSC.c)
    if(WIN32)
      target_link_libraries(osc ${LIBLO_LIBRARIES})
	  # FIXME how to build a static version of this?
      if(BUILD_STATIC_LIBRARY AND NOT MSVC)
        add_library(pthread_static STATIC IMPORTED)
        set_target_properties(pthread_static PROPERTIES IMPORTED_LOCATION ${PTHREAD_LIBRARY})
        target_link_libraries(osc pthread_static)
      elseif(NOT MSVC)
        target_link_libraries(osc ${PTHREAD_LIBRARY})
      endif()
      target_link_libraries(osc wsock32 ws2_32 iphlpapi)
    elseif(HAIKU)
      target_link_libraries(osc ${LIBLO_LIBRARIES})
    else()
      target_include_directories(osc PRIVATE ${LIBLO_INCLUDE_DIRS})
      target_link_libraries(osc ${LIBLO_LIBRARIES} pthread)
    endif()
endif()

## deprecated opcodes ##
if(BUILD_DEPRECATED_OPCODES)
    #make_plugin(deprecated deprecated.c)
endif()

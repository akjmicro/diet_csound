option(BUILD_CSBEATS "Build the beats score frontend" ON)

if (NOT MSVC)
    find_package(FLEX)
    find_package(BISON)
endif()

find_path(PD_HEADER_PATH m_pd.h)
find_library(PD_LIBRARY pd.dll)

## Csound Commandline Executable ##
set(CS_MAIN_SRCS csound/csound_main.c)
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    list(APPEND CS_MAIN_SRCS csound/sched.c)
    list(APPEND CSOUNDLIB -lpthread)
endif()

# We need a different name to avoid clashes with float libcsound
make_executable(csound-bin "${CS_MAIN_SRCS}" "${CSOUNDLIB};${MATH_LIBRARY}" csound)

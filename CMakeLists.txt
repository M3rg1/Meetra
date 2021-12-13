cmake_minimum_required(VERSION 3.20)
project(Meetra VERSION 0.1)
set(PROJECT_AUTHOR M3rg1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)
if (supported)
    message(STATUS "IPO / LTO enabled")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else ()
    message(STATUS "IPO / LTO not supported")
    message(DEBUG "<${error}>")
endif ()

configure_file(Env.h.in ${CMAKE_BINARY_DIR}/generated/Env.h)
set(INCLUDE_DIRS ${CMAKE_BINARY_DIR}/generated/)
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "src/*.cpp")

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

set(LINK_LIBS Threads::Threads)

add_executable(${PROJECT_NAME} ${SRC_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${LINK_LIBS})

if (MINGW)
    set(CMAKE_EXE_LINKER_FLAGS -static)
endif ()
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -fno-exceptions -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -pedantic -Wextra -Wshadow -Wno-unknown-pragmas")

message(STATUS "CMAKE_CXX_FLAGS_DEBUG are: ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CMAKE_CXX_FLAGS_RELEASE are: ${CMAKE_CXX_FLAGS_RELEASE}")

message(STATUS "Install to: ${CMAKE_INSTALL_PREFIX}")
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

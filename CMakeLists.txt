cmake_minimum_required(VERSION 3.00)
project(Meetra VERSION 1.0)
set(PROJECT_AUTHOR M3rg1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

configure_file(Env.h.in ${CMAKE_BINARY_DIR}/generated/Env.h)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS "src/*.h")

set(INCLUDE_DIRS ${CMAKE_BINARY_DIR}/generated/)
#[[foreach(HEADER_FILE ${HEADER_FILES})
    get_filename_component(DIR ${HEADER_FILE} PATH)
    list(APPEND INCLUDE_DIRS ${DIR})
endforeach()
list(REMOVE_DUPLICATES INCLUDE_DIRS)]]

add_executable(${PROJECT_NAME} ${SRC_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS})

#file(GLOB_RECURSE SRC_FILES src/*.cxx)
#add_executable(${PROJECT_NAME} ${SRC_FILES})

#target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/generated/)
# target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# https://stackoverflow.com/questions/35116327/when-g-static-link-pthread-cause-segmentation-fault-why/45271521
set(CMAKE_EXE_LINKER_FLAGS "-static -pthread -Wl,--whole-archive -lpthread -Wl,--no-whole-archive")

# -fno-inline-small-functions
# -funroll-loops
# -march=native
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-exceptions")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -pedantic -Wextra -Wno-unknown-pragmas")

message("CMAKE_CXX_FLAGS_DEBUG is ${CMAKE_CXX_FLAGS_DEBUG}")
message("CMAKE_CXX_FLAGS_RELEASE is ${CMAKE_CXX_FLAGS_RELEASE}")

install(TARGETS ${PROJECT_NAME} DESTINATION bin)

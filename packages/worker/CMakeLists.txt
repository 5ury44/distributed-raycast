# packages/worker/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(RaycastWorker)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Protobuf REQUIRED)
find_package(SDL2 REQUIRED)

# Find gRPC using pkg-config
pkg_check_modules(GRPC REQUIRED grpc++)

# Include directories
include_directories(include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../shared/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Source files
set(SOURCES
    src/worker.cpp
    src/raycast_engine.cpp
    ../shared/include/Player.cpp
    ../shared/include/Map.cpp
)

# Generated protobuf files
set(PROTO_FILES
    proto/worker_service.proto
)

# Generate protobuf and gRPC files
set(PROTO_SRCS)
set(PROTO_HDRS)
set(GRPC_SRCS)
set(GRPC_HDRS)

foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    get_filename_component(PROTO_PATH ${PROTO_FILE} PATH)
    
    list(APPEND PROTO_SRCS "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.cc")
    list(APPEND PROTO_HDRS "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.h")
    list(APPEND GRPC_SRCS "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.cc")
    list(APPEND GRPC_HDRS "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.h")
endforeach()

add_custom_command(
    OUTPUT ${PROTO_SRCS} ${PROTO_HDRS} ${GRPC_SRCS} ${GRPC_HDRS}
    COMMAND protobuf::protoc
    ARGS --proto_path="${CMAKE_CURRENT_SOURCE_DIR}/proto"
         --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
         --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
         --plugin=protoc-gen-grpc=/usr/bin/grpc_cpp_plugin
         "${CMAKE_CURRENT_SOURCE_DIR}/proto/worker_service.proto"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/proto/worker_service.proto"
)

# Create executable
add_executable(raycast_worker ${SOURCES} ${PROTO_SRCS} ${GRPC_SRCS})

# Link libraries
target_link_libraries(raycast_worker
    ${GRPC_LIBRARIES}
    protobuf::libprotobuf
    SDL2::SDL2
    pthread
)

# Add include directories from pkg-config
target_include_directories(raycast_worker PRIVATE ${GRPC_INCLUDE_DIRS})

# Compiler flags
target_compile_options(raycast_worker PRIVATE -O3 -march=native ${GRPC_CFLAGS_OTHER})
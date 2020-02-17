cmake_minimum_required(VERSION 3.5)

project(adsb)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
message("${CMAKE_SOURCE_DIR}/cmake/Modules/")


set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "-mavx -mavx2 -O3")

set(SOURCE_FILES
        main.cpp)

add_executable(adsb ${SOURCE_FILES})

find_package (UHD)
if    (UHD_FOUND)
    include_directories(${UHD_INCLUDE_DIRS})
    target_link_libraries (adsb ${UHD_LIBRARIES})
endif (UHD_FOUND)

find_package(Boost COMPONENTS system filesystem REQUIRED)

target_link_libraries(adsb
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        )


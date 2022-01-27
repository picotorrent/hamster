cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

cmake_policy(SET CMP0092 NEW) # don't add /W3 as default

set(VCPKG_OVERLAY_PORTS    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/vcpkg-overlays/ports)
set(VCPKG_OVERLAY_TRIPLETS ${CMAKE_CURRENT_SOURCE_DIR}/vendor/vcpkg-overlays/triplets)
set(CMAKE_TOOLCHAIN_FILE   ${CMAKE_CURRENT_SOURCE_DIR}/vendor/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")

project(hamster)

set(CMAKE_CXX_STANDARD 20)

find_package(Boost                      REQUIRED COMPONENTS log program_options system)
find_package(LibtorrentRasterbar CONFIG REQUIRED)
find_package(nlohmann_json       CONFIG REQUIRED)
find_package(unofficial-sqlite3  CONFIG REQUIRED)

add_executable(
    hamster
    src/indexer.cpp
    src/main.cpp
    src/migrator.cpp
    src/models/node.cpp
    src/models/sample.cpp
    src/models/torrent.cpp
    src/options.cpp
)

target_link_libraries(
    hamster
    PRIVATE
    Boost::boost
    Boost::log
    Boost::program_options
    nlohmann_json::nlohmann_json
    LibtorrentRasterbar::torrent-rasterbar
    unofficial::sqlite3::sqlite3
)

# MK Arcade Online: the emulation core (core/, FBNeo's Midway drivers, FB Neo license) and the app (app/, MIT).
cmake_minimum_required(VERSION 3.24)
project(MKArcadeOnline VERSION 0.1.1 LANGUAGES C CXX)

# Qt 6.12 (msvc2022_64). The MK3D project's copy is used when this one has none (docs/SETUP.md).
if(NOT CMAKE_PREFIX_PATH)
    foreach(qt "${CMAKE_CURRENT_SOURCE_DIR}/external/qt/6.12.0/msvc2022_64"
               "${CMAKE_CURRENT_SOURCE_DIR}/../mk3d-rollback/external/qt/6.12.0/msvc2022_64")
        if(EXISTS "${qt}/lib/cmake/Qt6")
            set(CMAKE_PREFIX_PATH "${qt}")
            break()
        endif()
    endforeach()
endif()

add_subdirectory(core)
add_subdirectory(app)

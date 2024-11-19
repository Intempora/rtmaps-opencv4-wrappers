##############################################################################
# 
#  Copyright 2014-2024 Intempora S.A.S.
#  
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#  
#      http://www.apache.org/licenses/LICENSE-2.0
#  
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
##############################################################################

cmake_minimum_required(VERSION 2.8.12)

set(PCK rtmaps_opencv4)
project("${PCK}.u")

set(CMAKE_CXX_STANDARD          11 )
set(CMAKE_CXX_STANDARD_REQUIRED ON )

if (NOT DEFINED RTMAPS_SDKDIR)  # If -D"RTMAPS_SDKDIR=<RTMaps Install Dir>" has NOT been passed to the "cmake" command
    if (DEFINED ENV{RTMAPS_SDKDIR})  # Try to use the default RTMaps installation
        set(RTMAPS_SDKDIR "$ENV{RTMAPS_SDKDIR}" CACHE PATH "Path the RTMaps installation directory")
    else()
        message(FATAL_ERROR "RTMAPS_SDKDIR not defined. Either pass -D\"RTMAPS_SDKDIR=...\" to CMake or define an RTMAPS_SDKDIR environment variable")
    endif()
endif()

include("${RTMAPS_SDKDIR}/templates.u/rtmaps.cmake")


    if(MSVC)
        find_package(OpenCV REQUIRED PATHS "./opencv/build/install")  # Specify the DIRECTORY of OpenCVConfig.cmake
    else()
        find_package(OpenCV REQUIRED PATHS "./opencv/build")  # Specify the DIRECTORY of OpenCVConfig.cmake
    endif()
    
add_rtmaps_package(${PCK} PCKINFO "${PCK}.pckinfo"

    "local_interfaces"                    # NB: if you add and/or remove files to this directory, you must re-run the CMake generation command
    "src"                                 # NB: if you add and/or remove files to this directory, you must re-run the CMake generation command


)

     target_include_directories(${PCK} PRIVATE
         ${OpenCV_INCLUDE_DIRS}
     )

     target_link_libraries(${PCK}
         ${OpenCV_LIBS}
		 rtmaps_input_reader
     )

if (MSVC)
    target_compile_options(${PCK} PRIVATE /W4)
else()
    target_compile_options(${PCK} PRIVATE -fdiagnostics-show-option -Wall -Wextra -pedantic)
endif()
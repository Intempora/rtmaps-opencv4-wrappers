# OpenCV4


## Requirements

* CMake >= 2.8.12
* Linux: GCC or Clang
* Windows: Visual C++ Compiler >= 2005
* OpenCV >= 4.0.0

## OpenCV

This RTMaps package uses the OpenCV library version 4. You can download pre-built binaries or download the sources and build them. Both binaries and sources are available from https://opencv.org/releases/ --please select a version 4.0.0 or greater.

### Pre-built binaries
Keep in mind that, as OpenCV 4 is a C++ library with a C++ interface, the compiler used to build OpenCV has to match the compiler you use to build your RTMaps component. "Windows" downloads on the OpenCV download page contain binaries built for versions of Microsoft Visual C++ (Visual Studio). For example, the string "vc14_vc15" in the file name means that it contains binaries for Visual Studio 2015 and 2017. It will not work with any other version.

In Ubuntu, installing OpenCV with the system's packet manager should also work.

### Build the OpenCV library
To build OpenCV from source, download them from https://opencv.org/releases/ and follow the included instructions. Use the same compiler as for your RTMaps component.

## Build the OpenCV Package
   
1. Firstly you need to open the `CMakeLists.txt` file and modify, with your OpenCV build path, the following code:

```

if(MSVC)
        find_package(OpenCV REQUIRED PATHS "./opencv/build/install")  # Specify the DIRECTORY of OpenCVConfig.cmake
    else()
        find_package(OpenCV REQUIRED PATHS "./opencv/build")  # Specify the DIRECTORY of OpenCVConfig.cmake
    endif()

```
2. Create a build directory and go to it.
   
### Linux

Generate the build system, for example you can do it by using the following command line:
```
    cmake -D"RTMAPS_SDKDIR=rtmaps_install_dir" -DCMAKE_BUILD_TYPE=Release ..
```
`Note`: 
1. You need to replace `rtmaps_install_dir` by your rtmaps installation directory.
   
2.  This will build will be of Release-type.
   
Then you can build the project with the command line:
```
    cmake --build . --config Release --target rtmaps_opencv4
```

### Windows

Generate a Visual Studio project using CMake:

```
    cmake -D"RTMAPS_SDKDIR=rtmaps_install_dir" -G"Visual Studio 14 2015 Win64" ..
```
You must modify the `Visual Studio 14 2015 Win64` part depending on your setup and replace `rtmaps_install_dir` by your rtmaps installation directory.

Finally you can open `rtmaps_opencv4.sln` and compile it.

`Note` that on Windows once compiled successfully, you must copy the bin/ folder of the openCV libraries next to the .pck, otherwise you will not be able to load the package into RTMaps. In that case, you will have the `DLL missing` message in the console, showing your dependencies problem.
The structure should be as following:
            
```
packages
└───debug/    
│   └───opencv4.pck
│   └───bin/
│       │   opencv_core401.dll
│       │   opencv_imgproc401.dll
│       │   [...]
```

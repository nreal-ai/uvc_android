# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)

# Declares the project name. The project name can be accessed via ${ PROJECT_NAME},
# Since this is the top level CMakeLists.txt, the project name is also accessible
# with ${CMAKE_PROJECT_NAME} (both CMake variables are in-sync within the top level
# build script scope).
project("uvctest")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
#
# In this top level CMakeLists.txt, ${CMAKE_PROJECT_NAME} is used to define
# the target library name; in the sub-module's CMakeLists.txt, ${PROJECT_NAME}
# is preferred for the same purpose.
#
# In order to load a library into your app from Java/Kotlin, you must call
# System.loadLibrary() and pass the name of the library defined here;
# for GameActivity/NativeActivity derived applications, the same library name must be
# used in the AndroidManifest.xml file.


add_executable(${CMAKE_PROJECT_NAME}
        # List C/C++ source files with relative paths to this CMakeLists.txt.
#        uvctest.cpp
        main.c
        v4l2.c
        xreal_metadata.c
        show_image.cpp
        show_image_c.cpp
        )

set(OpenCV_DIR /Users/lpzhang/proj/uvc_cam/opencv-4.5.4-android-sdk/OpenCV-android-sdk)
set(Jpeg_DIR /Users/lpzhang/proj/uvc_cam/libjpeg-turbo)
target_include_directories(${CMAKE_PROJECT_NAME}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/includes
        ${OpenCV_DIR}/sdk/native/jni/include
        ${Jpeg_DIR}/src
        ${Jpeg_DIR}/build)
# Specifies libraries CMake should link to your target library. You
# can link libraries from various origins, such as libraries defined in this
# build script, prebuilt third-party libraries, or Android system libraries.
set(OpenCV_LIB_PATH ${OpenCV_DIR}/sdk/native/libs/${ANDROID_ABI})

#link_directories(${OpenCV_LIB_PATH})
add_library( lib_opencv SHARED IMPORTED )
set_target_properties(lib_opencv PROPERTIES IMPORTED_LOCATION ${OpenCV_LIB_PATH}/libopencv_java4.so)


add_library(lib_jpeg SHARED IMPORTED)
set_target_properties(lib_jpeg PROPERTIES IMPORTED_LOCATION
        ${Jpeg_DIR}/build/libturbojpeg.so)

target_link_libraries(${CMAKE_PROJECT_NAME}
        # List libraries link to the target library
        lib_opencv
        lib_jpeg
        android
        log
        mediandk)
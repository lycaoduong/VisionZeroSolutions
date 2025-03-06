cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)


project(nx_plugin_clip)
message(STATUS "PROJECT_NAME = ${CMAKE_PROJECT_NAME}")
add_definitions(-DPROJECT_NAME="${PROJECT_NAME}" -DHAS_CPP17)


#Find NX SDK
set(metadataSdkDir "" CACHE PATH "Path to unpacked VMS Metadata SDK zip.")
if(metadataSdkDir STREQUAL "")
    set(metadataSdkDir ${CMAKE_CURRENT_LIST_DIR}) #< Assume building samples inside the SDK.
    if(NOT EXISTS ${metadataSdkDir}/nx_sdk/src/nx/sdk OR NOT EXISTS ${metadataSdkDir}/nx_kit/src/nx/kit)
        message(FATAL_ERROR "Define metadataSdkDir cache variable to point to the unzipped SDK.")
    endif()
endif()

if(UNIX)
    # In Linux, for the plugin .so library, set `rpath` to "$ORIGIN" and do not set `runpath`, thus
    # enabling the lookup of the dependencies in the plugin dir first.
    string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,--disable-new-dtags")
endif()
set(CMAKE_SKIP_BUILD_RPATH ON)
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
set(CMAKE_INSTALL_RPATH "$ORIGIN")

if(WIN32)
    # Use all CPU cores by MSVC.
    string(APPEND CMAKE_CXX_FLAGS " /MP")
    # Do not create separate .pdb files for object files (workaround for mspdbsrv.exe bug, needed
    # for using "/MP" flag).
    add_compile_options("/Z7")
endif()

if(WIN32)
    set(API_EXPORT_MACRO "__declspec(dllexport)")
else()
    set(API_EXPORT_MACRO "__attribute__((visibility(\"default\")))")
endif()

#--------------------------------------------------------------------------------------------------
# Define nx_kit lib, static.

set(nxKitLibraryType "STATIC" CACHE STRING "" FORCE)
set(nxKitWithTests "NO" CACHE STRING "" FORCE)

add_subdirectory(${metadataSdkDir}/nx_kit ${CMAKE_CURRENT_BINARY_DIR}/nx_kit)

#--------------------------------------------------------------------------------------------------
# Define nx_sdk lib, static, depends on nx_kit.

set(SDK_SRC_DIR ${metadataSdkDir}/nx_sdk/src)
file(GLOB_RECURSE SDK_SRC CONFIGURE_DEPENDS ${SDK_SRC_DIR}/*)

add_library(nx_sdk STATIC ${SDK_SRC})
target_include_directories(nx_sdk PUBLIC ${SDK_SRC_DIR})
target_link_libraries(nx_sdk PRIVATE nx_kit)

target_compile_definitions(nx_sdk PRIVATE NX_PLUGIN_API=${API_EXPORT_MACRO}) #< for nxLibContext()

#Include 3rd Lib
#----------------------------------------------------------------------------------------- OnnxRuntime
set(OnnxRuntime_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/lib/onnxruntime/include)
set(OnnxRuntime_LIB_DIRS ${CMAKE_SOURCE_DIR}/lib/onnxruntime/lib)
file(GLOB Onnx_LIB_FILES "${OnnxRuntime_LIB_DIRS}/*.lib")
message(STATUS "Found OnnxRuntime libs: ${Onnx_LIB_FILES}")

#----------------------------------------------------------------------------------------- OpenCV
set(OPENCV_VERSION "480")
set(OpenCV_LIB "opencv_world${OPENCV_VERSION}.lib")  # Construct the library name
set(OpenCV_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/lib/opencv/include)
set(OpenCV_LIB_DIRS ${CMAKE_SOURCE_DIR}/lib/opencv/lib/Release)

#----------------------------------------------------------------------------------------- RapidJson, Eigen
set(RapidJson_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/lib/rapidjson)
set(Eigen_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/lib/eigen)

#----------------------------------------------------------------------------------------- Curl
# Define CURL_STATICLIB for static linking
add_definitions(-DCURL_STATICLIB)
set(Curl_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/lib/curl-7.86.0/include)
set(Curl_LIB_DIRS ${CMAKE_SOURCE_DIR}/lib/curl-7.86.0/lib)
file(GLOB Curl_LIB_FILES "${Curl_LIB_DIRS}/*.lib")
message(STATUS "Found Curl libs: ${Curl_LIB_FILES}")

#----------------------------------------------------------------------------------------- Spdlog
set(Spdlog_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/lib/spdlog-1.x/include)
# set(Spdlog_LIB_DIRS ${CMAKE_SOURCE_DIR}/lib/spdlog-1.x/lib)
# file(GLOB Spdlog_LIB_FILES "${Spdlog_LIB_DIRS}/*.lib")
# message(STATUS "Found Spd libs: ${Spdlog_LIB_FILES}")

#----------------------------------------------------------------------------------------- Mysql
set(Mysql_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/lib/mysql-8.0.32/include)
set(Mysql_LIB_DIRS ${CMAKE_SOURCE_DIR}/lib/mysql-8.0.32/lib)
set(Mysql_LIB "libmysql.lib")  # Construct the library name

#----------------------------------------------------------------------------------------- ZeroVisionCore
set(Core_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/core/LVisLib/include)
set(Core_LIB_DIRS ${CMAKE_SOURCE_DIR}/core/LVisLib/lib)
set(Core_LIB "LVisLib.lib")  # Construct the library name

#Include 3rd libraries and our CORE
include_directories( ${OnnxRuntime_INCLUDE_DIRS} 
                    ${OpenCV_INCLUDE_DIRS} 
                    ${RapidJson_INCLUDE_DIRS}
                    ${Eigen_INCLUDE_DIRS}
                    ${Curl_INCLUDE_DIRS}
                    ${Spdlog_INCLUDE_DIRS}
                    ${Mysql_INCLUDE_DIRS}
                    ${Core_INCLUDE_DIRS}
                    )


# Define source directory for plugin
set(visionZero_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)

# Add all the source files (*.cpp files) from the plugin directory
file(GLOB_RECURSE visionZero_PLUGIN_SRC CONFIGURE_DEPENDS ${visionZero_SRC_DIR}/*.cpp)

# Create a shared library from the source files
add_library(${PROJECT_NAME} SHARED 
            ${visionZero_PLUGIN_SRC}
            ${CMAKE_SOURCE_DIR}/version.rc  # Include version.rc
            )

# Set version properties for the DLL
# Set project version
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Specify the include directories (headers for your plugin)
target_include_directories(${PROJECT_NAME} PRIVATE ${visionZero_SRC_DIR})

# Link the dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE 
    nx_kit
    nx_sdk
    ${Onnx_LIB_FILES}       # Ensure this is a list of `.lib` files
    ${Curl_LIB_FILES}       # Ensure this is a list of `.lib` files
    ${OpenCV_LIB_DIRS}/${OpenCV_LIB}   # Full path to the OpenCV library
    ${Mysql_LIB_DIRS}/${Mysql_LIB}    # Full path to the MySQL library
    ${Core_LIB_DIRS}/${Core_LIB}      # Full path to the Core library
)

# Ensure proper API export macro for plugin
target_compile_definitions(${PROJECT_NAME} PRIVATE NX_PLUGIN_API=${API_EXPORT_MACRO})
                       
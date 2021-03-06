# Path to project cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
    ${CMAKE_CURRENT_LIST_DIR}/cmake
    ${CMAKE_CURRENT_LIST_DIR}/cotire/CMake)
# Include utility and Python build functions
include(Util)
include(Python)
include(cotire)

# Release and Debug builds
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif ()
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-O2 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
if (NOT (CMAKE_BUILD_TYPE MATCHES Debug))
    set(NO_DEBUG ON)
endif ()

# Fix for finding the correct Python version on Apple
if (APPLE)
    unset(PYTHON_LIBRARY CACHE)
    unset(PYTHON_INCLUDE_DIR CACHE)
    unset(PYTHON_LIBRARIES CACHE)
endif ()

cmake_minimum_required(VERSION 3.1.0)

project(minotaur-cpp)

# Unit testing with GoogleTest framework
enable_testing()
set(GTEST_INCLUDE_DIR ${gtest_SOURCE_DIR}/include)
add_subdirectory(lib/googletest)
add_subdirectory(test)
add_test(NAME minotaur-cpp-test COMMAND tests)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS} -std=c++11")
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(PythonLibs 3.4 REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Multimedia REQUIRED)
find_package(Qt5MultimediaWidgets REQUIRED)
find_package(Threads REQUIRED)

if (Qt5_POSITION_INDEPENDENT_CODE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif ()

file(GLOB_RECURSE SOURCE_FILES
        ${PROJECT_SOURCE_DIR}/code/**/*.cpp)

file(GLOB_RECURSE HEADER_FILES
        ${PROJECT_SOURCE_DIR}/code/**/*.h)

file(GLOB_RECURSE FORM_FILES
        ${PROJECT_SOURCE_DIR}/code/**/*.ui)

set(MINOTAUR_EXECUTABLE_MAIN ${PROJECT_SOURCE_DIR}/code/main.cpp)

add_python_target(code/scripts/__init__.py)
add_python_target(code/scripts/robot.py)

# Include library directories
include_directories(${Qt5Widgets_INCLUDE_DIRS})
include_directories(${PYTHON_INCLUDE_DIRS})
include_directories(third-party)

# AutoMOC and AutoUIC for Qt compiler
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

get_target_property(QtCore_location Qt5::Core LOCATION)
add_library(minotaur-lib ${SOURCE_FILES} ${HEADER_FILES} ${FORM_FILES})
cotire(minotaur-lib)

# Qt5 modules
qt5_use_modules(minotaur-lib SerialPort)

# Supply cmake argument -D NO_DEBUG=ON to disable console debug output
option(NO_DEBUG "Build minotaur without debugging output" OFF)
if (NO_DEBUG)
    log("Building without debugging")
    target_compile_definitions(minotaur-lib PRIVATE QT_NO_DEBUG)
    target_compile_definitions(minotaur-lib PRIVATE NDEBUG)
else ()
    log("Building with debugging on")
endif ()
unset(NO_DEBUG)

# Find and link OpenCV 3 package
find_package(OpenCV 3 REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
target_link_libraries(minotaur-lib ${OpenCV_LIBS})

# Supply cmake argument -D NO_CONTRIB=ON to disable tracking module
option(NO_CONTRIB "Build minotaur without OpenCV contrib modules" OFF)
include(CheckIncludeFileCXX)
# Check for the OpenCV tracking contrib module
check_include_file_cxx(opencv2/tracking.hpp HAVE_OPENCV_TRACKER)
if (NO_CONTRIB OR NOT HAVE_OPENCV_TRACKER)
    target_compile_definitions(minotaur-lib PRIVATE TRACKER_OFF)
    log("Building without OpenCV tracking module")
endif ()

# Attempt to find GOTURN files
include(Goturn)
if (${GOTURN_FILES_FOUND})
    target_compile_definitions(minotaur-lib PRIVATE GOTURN_FOUND)
endif ()

# Link Qt, Python, and Thread libraries
target_link_libraries(minotaur-lib
        Qt5::Widgets
        Qt5::Core
        Qt5::Gui
        Qt5::Multimedia
        Qt5::MultimediaWidgets)
target_link_libraries(minotaur-lib ${PYTHON_LIBRARIES})
target_link_libraries(minotaur-lib "${CMAKE_THREAD_LIBS_INIT}")

# Include current directory to find Qt MOC files
target_include_directories(minotaur-lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Add definition for location of python scripts
target_compile_definitions(minotaur-lib PRIVATE PYTHON_SCRIPT_DIR="${PYTHON_SCRIPT_DIR}")

# Windows requires separate libraries
if (WIN32)
    target_link_libraries(minotaur-lib advapi32 user32 setupapi)
endif ()

# Create the Minotaur executable
add_executable(minotaur-cpp ${MINOTAUR_EXECUTABLE_MAIN})
cotire(minotaur-cpp)
target_link_libraries(minotaur-cpp minotaur-lib)

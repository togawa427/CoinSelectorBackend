# cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

# specify the C++ standard
# set(CMAKE_CXX_STANDARD 20)
# set(CMAKE_CXX_STANDARD_REQUIRED True)
set (CMAKE_CXX_STANDARD 11)
project(CoinSelectorBackend)

# set the project name and version
# project(httpSample CXX)

# find package
find_package(OpenCV 4.4.0 REQUIRED)
find_package(Threads REQUIRED)

# compile executable file
# add_executable(serverSample main.cpp)
include_directories(${OpenCV_INCLUDE_DIRS})
file(GLOB SOURCES *.cpp)

# module, library, target link
# target_link_libraries(main PUBLIC "-pthread")
add_executable (main ${SOURCES})
target_link_libraries (main
	PRIVATE ${OpenCV_LIBS}
)
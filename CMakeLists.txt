cmake_minimum_required(VERSION 3.9)
project(ca_traffic_simulation)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)

# Use MPI compiler (mpic++)
set(CMAKE_CXX_COMPILER mpic++)

# Set compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -DDEBUG -Wall -g")

# Specify output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../test)

# Add the executable
add_executable(cats src/main.cpp src/Road.cpp src/Road.h src/Lane.cpp src/Lane.h src/Vehicle.cpp src/Vehicle.h src/Simulation.cpp src/Simulation.h src/Inputs.cpp src/Inputs.h src/Statistic.cpp src/Statistic.h src/CDF.cpp src/CDF.h src/ProcessData.h)
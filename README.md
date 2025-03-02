<div align="center">
# PapiCPP: A C++ Wrapper for PAPI
</div>

PapiCPP is a C++ wrapper for the Performance API (PAPI), providing an easy and convenient interface to access hardware performance counters. This library allows you to use PAPI's event and counter functionalities in a C++-friendly way, abstracting away low-level details and offering features like event sets, counter tracking, and printing event statistics.

## Features
* Easy-to-use interface for working with PAPI performance counters.
* Template-based approach for event sets, allowing you to group and track multiple events at once.
* Automatic event code to name resolution.
* Stream output support for easy display of event data.
* Exception handling for PAPI library errors.

## Prerequisites
Before using PapiCPP, ensure that you have the PAPI library installed on your system. It is typically available through package managers on many Linux distributions or can be compiled from source.

* PAPI version 5.x or later is required.
* A C++17 compatible compiler.

## Building and Testing

1. First create a build folder in the base directory and cd into it
    * `mkdir build`
    * `cd ./build`

2. Run the cmake command in the build directory
    * `cmake ..`
    - Or alternatively specify the directory where the header file is and the library file you want to use
    * `cmake -DPAPI_INCLUDE_DIR=/path/to/papi/include -DPAPI_LIBRARIES=/path/to/papi/lib/libpapi.* ..`

3. Run the make command
    * `make`

4. Run the test
    * `./testing`


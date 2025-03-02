
# PapiCPP: A C++ Wrapper for PAPI

PapiCPP is a C++ wrapper for the Performance API (PAPI), providing an easy and convenient interface to access hardware performance counters. This library allows you to use PAPI's event and counter functionalities in a C++ friendly way, abstracting away low-level details and offering features like event sets, counter tracking, and printing event statistics.

### Note:

The PAPI library is only available for Linux OS and its available functionality is hardware dependent.

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

## Checking Available PAPI Preset Events

PAPI comes with preset events. Supported preset events will depend on your computers architecture.

To check available preset events, run the following:

`papi_avail -a`

## Using PapiCPP

PAPI_CPP makes use of templates to allow you to pass in the preset event codes from the PAPI C library. Use these event codes with the C++ `papi::event_set` wrapper class and it will handle all initialization, and de-initialization for the underlying PAPI code.

Some brief example code may look like the following:

```cpp
#include <iostream>
#include "papiCPP.h"

int main(int argc, char **argv) {
	
	// Count total instructions and cpu cycles
	papi::event_set<
		PAPI_TOT_INS,
		PAPI_TOT_CYC
	> events;

	events.start_counters(); // Start counting

	{
		// Code you want to profile
	}

	events.stop_counters(); // Stop counting

	std::cout << events << std::endl; // Print all events

	// Alternatively you can print specific events with
	// the .get or .at functions
	std::cout
		<< "Total IPC = "
		<< (double) events.get<PAPI_TOT_INS>().counter() /
		<< (double) events.get<PAPI_TOT_CYC>().counter()
		<< std::endl;

	return 0;
}
```

## Building and Testing

1. First clone the github project with

	* `git clone https://github.com/squee72564/PAPI_CPP.git`

2. cd into the base directory with

	* `cd ./PAPI_CPP`

3. Next create a build folder in the base directory and cd into it

	* `mkdir build`

	* `cd ./build`

4. Run the cmake command in the build directory if your system recognizes where the necessary PAPI header and library file is

	* `cmake ..`

- Or alternatively specify the directory where the header file is and the library file you want to use

	* `cmake -DPAPI_INCLUDE_DIR=/path/to/papi/includedir -DPAPI_LIBRARIES=/path/to/papi/lib/papiLibraryFile.* ..`

5. Run the make command

	* `make`

6. Run the test

	* `./testing`

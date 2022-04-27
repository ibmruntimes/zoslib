# ZOSLIB - A z/OS C/C++ Library

## Table of Contents

 * [Overview](#overview)
 * [System Requirements](#system-requirements)
 * [Build and Install](#build-and-install)
 * [Quick Start](#quick-start)
 * [API Documentation](#api-documentation)
 * [Legalities](#legalities)

## Overview

ZOSLIB is a z/OS C/C++ library. It is an extended implementation of the
z/OS LE C Runtime Library.

ZOSLIB implements the following:

- A subset of POSIX APIs that are not available in the LE C Runtime Library
- EBCDIC <-> ASCII conversion C APIs
- APIs for improved diagnostic reporting
- and more!

## System Requirements

ZOSLIB is supported on the following z/OS operating systems
with z/OS® UNIX System Services enabled:

- z/OS V2R3 with the following PTFs installed:
 - UI61308
 - UI61375
 - UI61747

- z/OS V2R4 with the following PTFs installed:
 - UI64830
 - UI64837
 - UI64839
 - UI64940
 - UI65567

ZOSLIB is supported on the following hardware:
- IBM z15
- IBM z14/z14 Model ZR1
- IBM z13/z13s
- IBM zEnterprise EC12/BC12

## Build and Install

### Build tool prerequisites
* CMake 3.5+
* GNU Make 4.1+
* IBM XL C/C++ V2.3.1 for z/OS V2.3 web deliverable (xlclang/xlcang++)
* Git
* Ninja (optional)

Clone the ZOSLIB source code using Git into a newly created zoslib directory:

``` bash
$ git clone git@github.com:ibmruntimes/zoslib.git zoslib
```

After obtaining the source, `cd` to the `zoslib` directory and follow one of the
following options to build zoslib and run its tests.

1. Use the included `build.sh` to build and optionally run the zoslib tests:

``` bash
$ ./build.sh -h
```

which displays flags that you can pass to `build.sh` to specify a Release build
(default is Debug) or Shared library (default is Static), and whether to build
and run the zoslib tests.

Example:
``` bash
$ ./build.sh -c -r -s -t
```

performs a Clean (-c) Release (-r) build that creates a Shared (-s) library
`libzoslib.so` and its sidedeck `libzoslib.x`, builds and runs the tests (-t).

`build.sh` creates directory `./build` to hold the build files, and then places
the target files under `./install` directory.

2. Use the steps below to build and optionally run the zoslib tests:

Create a build directory to hold your build files and `cd` to it:

``` bash
$ mkdir build && cd build
```

Next, we will configure our build with CMake.

Make sure to export the `CC` and `CXX` environment variables to
point to the supported C/C++ build compiler, or pass in the CMake
options -DCMAKE_C_COMPILER and -DCMAKE_CXX_COMPILER.

From the directory `build`, enter the following CMake command
(here, `..` refers to the ZOSLIB source directory)

``` bash
$ cmake ..
```

By default CMake will configure your build as a Debug build. You can configure
your build as a Release build with the `-DCMAKE_BUILD_TYPE=Release` option.

Also by default, CMake will configure your build to create a static library
`libzoslib.a`. To create a shared library, pass to CMake the option
`-DBUILD_SHARED_LIBS=ON`.

CMake will detect your development environment, perform a series of tests, and
generate the files required for building ZOSLIB.

By default, CMake will generate Makefiles. If you prefer to use Ninja, you can
specify -GNinja as an option to CMake.

After CMake has finished with the configuration, start the build from `build`
using CMake:

``` bash
$ cmake --build .
```

After ZOSLIB has finished building, install it from `build`:

``` bash
$ cmake --build . --target install
```

## Quick Start

Once we have ZOSLIB built and installed, let's attempt to build our first
ZOSLIB C++ application. The application will generate a series of random
numbers, leveraging the `getentropy` C API in ZOSLIB.

1. Create a file named `random.cc` containing the following contents:

```cpp
// random.cc

// Include zos.h ZOSLIB header
#include <zos.h>
#include <stdio.h>

// Initialize ZOSLIB class
__init_zoslib __nodezoslib;

int main(int argc, char** argv) {
  printf("ZOSLIB version: %s\n", __zoslib_version);
  if (argc < 1) {
    printf("An argument specifying the number of random "
           " numbers is required\n");
    return 1;
  }

  int num = atoi(argv[1]);
  if (num < 0) {
    printf("The argument should be positive (>0)\n");
    return 2;
  }
  printf("Generating %d random values\n", num);

  char buffer[10];
  for (int i = 0; i < num; i++) {
    printf("Random index: %d\n", i);
    // Call ZOSLIB getentropy C API
    if (!getentropy(buffer, 10)) {
      for (int j = 0; j < 10; j++)
        printf("%2X ", buffer[j]);
      printf("\n");
    }
  }
  return 0;
}
```

This example will first include the main ZOSLIB header file, `zos.h`, which
subsequently includes all of the ZOSLIB header files. Alternatively, we could
have just included `zos-base.h`, since the prototype for `getentropy` is defined
in `zos-base.h`.

2. In order to initialize ZOSLIB, we need to create a static instance of the
`__init_zoslib` class: `__init_zoslib zoslib_init;`. This initializes the
Enhanced ASCII runtime environment, among other things. If your application
is C only, you can make use of the `init_zoslib` function instead.

In the `main` function, we make use of two ZOSLIB definitions,
`__zoslib_version` to obtain the ZOSLIB version, and `getentropy` to generate
a list of random values.

3. To compile and link the application, enter the following command:

``` bash
xlclang++ -I path/to/zoslib/include -L path/to/build/lib -lzoslib random.cc -o random
```

4. To run the application, enter the following command:
``` bash
./random 2
```

You should get an output similar to the following:
```
ZOSLIB version: v2.1.0
Generating 2 random values
Random index: 0
BC DE CF DE  7 E3 58 3A 4F 22
Random index: 1
5B 30 5A 9C C4 70 94 A6 B6 E5
```

## API Documentation

The ZOSLIB API documentation is available [here](docs).

## Legalities

ZOSLIB is available under the Apache 2.0 license. See the [LICENSE file](LICENSE)
for details

### Copyright

```
Licensed Materials - Property of IBM
ZOSLIB
(C) Copyright IBM Corp. 2020. All Rights Reserved.
US Government Users Restricted Rights - Use, duplication
or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
```

# gint project

gint (pronounce “guin”) is an add-in unikernel for CASIO calculators of the
fx-9860G II and fx-CG 50 families. It provides a mostly free-standing runtime
and is used to develop add-ins under Linux, along with specialized GCC
toolchains and the [fxSDK](/Lephenixnoir/fxsdk).

When running in an add-in, gint takes control of the calculator's hardware
from the operating system, and manages it with its own drivers. It exposes a
new, richer API that takes advantage of the full capabilities of the machine.

This is free software: you may use it for any purpose, share it, modify it, and
share your changes. Credit is not required, but please let me know!

gint used to include third-party code that has now been moved to the FxLibc. If
you stumble upon it in the history, check the README file at that time for
license details.

## Programming interface

Because of its free-standing design, gint's API provides direct and efficient
access to low-level MPU features, which includes:

* Multi-key management with event systems suitable for games
* Hardware timers with sub-millisecond and sub-microsecond resolution
* Fast screen drivers with DMAC on fx-CG 50
* Efficient and user-extendable interrupt management
* Safe access to on-chip and DSP memory areas
* Hardware-driven memory primitives (DMA, DSP)

The library also offers powerful higher-level features:

* An enhanced version of the system's GetKey() and GetKeyWait()
* A gray engine that works by rapidly swapping monochrome images on fx-9860G II
* Blazingly fast rendering functions (image rendering is 10 times faster than
  MonochromeLib)
* Integrated font management

A couple of libraries extend these features, including:

* [libprof](/Lephenixnoir/libprof): Profiling and performance evaluation
* [libimg](/Lephenixnoir/libimg): Versatile image transformations
* [OpenLibm](/Lephenixnoir/OpenLibm): A port of the standard math library
  (actually needed by gint)
* [fxlibc](/Vhex-Kernel-Core/fxlibc/): A community standard library with
  dedicated SuperH optimizations (in progress; needed by gint unless you're
  trying out Newlib)
* Integration with [a Newlib port by Memallox](/PlaneteCasio/libc) (unstable)

## Installing with GiteaPC

gint can be installed automatically with [GiteaPC](/Lephenixnoir/GiteaPC).

```bash
% giteapc install Lephenixnoir/gint
```

Normally you don't use gint directly, instead the fxSDK provides project
templates that are set up to use gint. Please see the
[fxSDK README file](/Lephenixnoir/fxsdk) for details.

## Building and installing manually

gint is built using the [fxSDK](/Lephenixnoir/fxsdk), which provides a suitable
CMake environment for the calculator. gint is always installed in the
compiler's install path (as given by `sh-elf-gcc --print-search-dirs`) which is
detected automatically, so normally you don't need to set the install prefix.

gint depends on OpenLibm and fxlibc (linked above). Both can be installed
easily (essentially copy-paste the command from the respective READMEs). You
can technically use another libc but there be dragons.

**Building for fx-9860G II**

`fxsdk build-fx` will invoke CMake and make. If you have specific configuration
options, run once with `-c` to configure.

```
% fxsdk build-fx -c <OPTIONS...>
```

Run without `-c` to build. This configures automatically.

```
% fxsdk build-fx
% fxsdk build-fx install
```

The available options are:

* `-DGINT_STATIC_GRAY=1`: Put the gray engine's VRAMs in static RAM instead of
  using `malloc()`

**Building for fx-CG 50**

Same as fx-9860G II, except the command is `fxsdk build-cg` instead of `fxsdk
build-fx`.

The available options are:

* `-DGINT_USER_VRAM=1`: Store all VRAMs in the user stack (takes up 350k/512k)

```
% fxsdk build-cg
% fxsdk build-cg install
```

**"Cross-Building" a fx-9860G II project for fx-CG 50**

Programs written for fx-9860G can also be built to run on the fx-CG series,
provided no low-level function or hardware-specific behavior (like syscalls) is
used by the program.

```
% fxsdk build-fxg3a
% fxsdk build-fxg3a install
```

## Using in CMake-based add-ins

Find the `Gint` module and link against `Gint::Gint`. gint declares the include
and library paths needed to link with OpenLibm and the libc, and will
automatically link both.

```cmake
find_module(Gint 2.1 REQUIRED)
target_link_libraries(<target_name> Gint::Gint)
```

## Using in Makefile-based add-ins

Projects created with the fxSDK link with gint out-of-the-box. If you're not
using the fxSDK, you will need to:

* Build with `-ffreestanding -fstrict-volatile-bitfields` and either
  `-DFX9860G` or `-DFXGC50`;
* Link with `-T fx9860g.ld -lgint-fx -lopenlibm -lc` on fx-9860G;
* Link with `-T fxcg50.ld -lgint-cg -lopenlibm -lc` on fx-CG 50.

To manually build the fx-CG executable for an fx-9860G program, mix as follows:

* Use platform flags `-DFXCG50 -DFX9860G_G3A`;
* Link with `-T fxcg50.ld -lgint-fxg3a -lopenlibm -lc`.

If you don't have a standard library such as
[Memallox's port of newlib](/PlaneteCasio/libc), you also need `-nostdlib`. I
typically use `-m3 -mb` or `-m4-nofpu -mb` to specify the platform, but that
may not even be necessary.

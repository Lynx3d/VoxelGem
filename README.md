VoxelGem
========

A Voxel editor written in C++,  using Qt5 and OpenGL 3.3 (Core Profile)

Building
--------

Requirements:
- A C++11 compatible compiler
- Qt5 >= 5.5, though 5.10 is highly recommended for correct sRGB rendering

## Linux:

Waf is used as build system. That means you need to have Python installed
(which is part of the base install of most distros now anyway), while Waf
itself is part of the repository.

Before building you need to configure once:

    ./waf configure

Available configure options:
	--debug_gl    |  enables OpenGL debug context and prints the debug log to console

compiling:

    ./waf build

or simply:

    ./waf

The resulting binary is in ./build/voxelGem
No installation required/supported at this point, it is a self-containing executable.

## Windows:

No setup available yet...contributions welcome.

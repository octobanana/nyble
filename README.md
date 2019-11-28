# nyble
A snake game for the terminal.

[![nyble](https://raw.githubusercontent.com/octobanana/nyble/master/res/nyble.png)](https://octobanana.com/software/nyble/blob/res/nyble.mp4#file)

Click the above image to view a video of __nyble__ in action.

## Contents
* [About](#about)
* [Design](#design)
* [Usage](#usage)
* [Pre-Build](#pre-build)
  * [Environments](#environments)
  * [Compilers](#compilers)
  * [Dependencies](#dependencies)
  * [Linked Libraries](#linked-libraries)
  * [Included Libraries](#included-libraries)
  * [macOS](#macos)
* [Build](#build)
* [Install](#install)
* [License](#license)

## About
__nyble__ is a take on the classic snake game, designed for the terminal using a text-based user interface (TUI).

## Design
### The Rules
Control the snake by moving it around the playing grid, collecting eggs while avoiding walls and the body of the snake.

### The Grid
As the height of a single character is about two times its width, the playing grid uses a 2:1 ratio. This allows the grid to be square.

The grids width must be divisible by two. If the width of the terminal is odd, subtract one from the width to make it even.

The grids origin is placed at the bottom left corner.  
The coordinates of the bottom left grid position is `(0, 0)`.  
The coordinates of the bottom right grid position is `(grid_width - 1, 0)`.  
The coordinates of the top left grid position is `(0, grid_height - 1)`.  
The coordinates of the top right grid position is `(grid_width - 1, grid_height - 1)`.

### The Egg
The egg is initially placed at position `((grid_width - 1) / 2, (_grid_height - 1) / 2)`.

A new egg is spawned by picking a random coordinate `(random_range(0, grid_width - 1), random_range(0, grid_height - 1))`.  
The new coordinate is then checked to make sure it doesn't match a position occupied by the snake.  
Repeat this process until a valid coordinate is found.

### The Snake
The snakes initial direction is `up`.

The snakes head is initially placed at position `((grid_width - 1) / 2, 0`.

The snake uses three colours, one for its head and two for its body.  
The two body colours are used alternatively behind the head on each update.

When the snake collects an egg:  
  The tail becomes fixed while the head continues to move, extending the length of the snake for n cycles.  
  The speed of the snake is increased.  
  The score is increased.

Updating the snake consists of:  
  Drawing the new head.  
  Drawing a new body over the previous head position.  
  Erasing the tail if the snake is not currently growing.

### The Game Loop
While the snake is alive:  
  Determine next head position based on the snakes current direction.  
  Check if the snake has collided with a wall.  
  Check if the snake has collided with itself.  
  Temporarily slow the tick rate if the snake is about to collide into a wall.  
  Check if the snake has collected an egg.  
  Update the snake.

### The Asynchronous Event Loop
Wait on a timer.  
Wait on a signal.  
Wait on user input.

## Usage
View the usage and help output with the `-h|--help` flag,
or as a plain text file in `./doc/help.txt`.

## Pre-Build
This section describes what environments this program may run on,
any prior requirements or dependencies needed, and any third party libraries used.

> #### Important
> Any shell commands using relative paths are expected to be executed in the
> root directory of this repository.

### Environments
* __Linux__ (supported)
* __BSD__ (supported)
* __macOS__ (supported)

### Compilers
* __GCC__ >= 8.0.0 (supported)
* __Clang__ >= 7.0.0 (supported)
* __Apple Clang__ >= 11.0.0 (untested)

### Dependencies
* __CMake__ >= 3.8
* __Boost__ >= 1.68.0
* __ICU__ >= 62.1
* __PThread__

### Linked Libraries
* __icui18n__ (libicui18n) part of the ICU library
* __icuuc__ (libicuuc) part of the ICU library
* __pthread__ (libpthread) POSIX threads library

### Included Libraries
* [__Belle__](https://github.com/octobanana/belle):
  Asynchronous input, modified and included as `./src/ob/belle.hh`
* [__Parg__](https://github.com/octobanana/parg):
  for parsing CLI args, modified and included as `./src/ob/parg.hh`

### macOS
Using a new version of __GCC__ or __Clang__ is __required__, as the default
__Apple Clang compiler__ does __not__ support C++17 Standard Library features such as `std::filesystem`.

A new compiler can be installed through a third-party package manager such as __Brew__.
Assuming you have __Brew__ already installed, the following commands should install
the latest __GCC__.

```sh
brew install gcc
brew link gcc
```

The following CMake argument will then need to be appended to the end of the line when running the shell script.
Remember to replace the placeholder `<path-to-g++>` with the canonical path to the new __g++__ compiler binary.

```sh
./RUNME.sh build -- -DCMAKE_CXX_COMPILER='<path-to-g++>'
```

## Build
The included shell script will build the project in release mode using the `build` subcommand:

```sh
./RUNME.sh build
```

## Install
The included shell script will install the project in release mode using the `install` subcommand:

```sh
./RUNME.sh install
```

## License
This project is licensed under the MIT License.

Copyright (c) 2019 [Brett Robinson](https://octobanana.com/)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

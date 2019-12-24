# nyble
A snake game for the terminal.

[![nyble](https://raw.githubusercontent.com/octobanana/nyble/master/res/nyble.png)](https://octobanana.com/software/nyble/blob/res/nyble.mp4#file)

Click the above image to view a video of __nyble__ in action.

## Contents
* [About](#about)
  * [Getting Started](#getting-started)
  * [Snake Skills](#snake-skills)
  * [nyblisp](#nyblisp)
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

### Getting Started
Currently, when the game starts it drops you into playground mode. The game is started and paused with the `<space>` key. The snake has two control types, 2-key and 4-key. Using 2-key moves the snake either left or right relative to the current moving direction of the snakes head. 2-key is bound to the `, .` keys. Using 4-key moves the snake in the direction of the key, if the move is possible. 4-key is bound to the `<up> <down> <left> <right>`, `w a s d`, and `h j k l` keys. Quit the game by pressing `<ctrl> c`.

To make handling the snake easier, the playing grid is checkered, and the border contains guides for both the head of the snake and the egg.

Collecting a golden egg increases the length and speed of the snake.

The snake can travel through each of the 4 golden portals tied to the x:y position of the egg.

### Snake Skills
Some snake skills being experimented with are:  
* __coil__ - press `1` to coil the body of the snake to a size of 3, and then extend itself back to its previous size.
* __reverse__ - press `2` to reverse the snakes direction, swapping the head with the tail.
* __fixed__ - press `3` to toggle fixed movement of the snake. A direction key must be pressed or held to move the snake.
* __???????__ - press `??????????` to enable ??????? mode, allowing the snake to ?????? ??????? ?????? ??? ????? for a short period of time.

### nyblisp
An embedded language is used to change settings and script the gameplay. Expressions can be entered at the prompt with the `:` key. The line editor has some helpful features such as parenthesis, bracket, brace, and quote autopairing. It also has an autocomplete function that can be initiated with the `<tab>` key.

A quick intro to the language:

```
; a comment

; a number
2
-8
4.0
3/4

; a string
"nyble"

; a symbol
*
map
nyble

; a list
'(1 2 3 4)
'(1 "nyble" 2 "nyble")
'(* 2 4)

; a function
(* 2 2)
(- 8 4)

; create a mutable binding
(var x 4)
(var x 8)

; create an immutable binding
(let name "nyble")
(let name 8) ; error constant binding

; create a function
(fn [x] (* 2 x))

; create, bind, and call a function
(let double (fn [x] (* 2 x)))
(double 4) ; 8

; create and call an anonymous function
((fn [x] (* 2 x)) 4) ; 8

; prevent evaluation of expression
(quote (1 2 3)) ; (1 2 3)
'(1 2 3) ; (1 2 3)

; index into list or string
(1 "nyble") ; "y"
(2 '(1 2 3)) ; 3

; get all but the first element in list or string
(@ "nyble") ; "yble"
(@ '(1 2 3)) ; (2 3)
```
Game specific:

```
; set frames per second (fps)
(fps 20)
(fps 30)
(fps 60)

; bind a key to a function
(key "w" '(up))
(key "s" '(down))
(key "a" '(left))
(key "d" '(right))
(key "," '(left2))
(key "." '(right2))

; bind keys to handle snake u-turn
(key "<" '(pn (left2) (left2)))
(key ">" '(pn (right2) (right2)))

; bind keys to inc/dec snake speed
(key "[" '(snake-speed (- (snake-speed) 10)))
(key "]" '(snake-speed (+ (snake-speed) 10)))

; get the snake size
(snake-size)

; set the snake size
(snake-size 10)

; get the snake speed
(snake-speed)

; set the snake speed measured in milliseconds between each movement
(snake-speed 200)
```

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
* __Boost__ >= 1.71.0
* __ICU__ >= 62.1
* __GMP__
* __MPFR__
* __PThread__

### Linked Libraries
* __icui18n__ (libicui18n) part of the ICU library
* __icuuc__ (libicuuc) part of the ICU library
* __pthread__ (libpthread) POSIX threads library
* __gmp__ (libgmp) arbitrary precision arithmetic library
* __mpfr__ (libmpfr) multiple-precision floating-point arithmetic library
* __boost_coroutine__ (libboost_coroutine) Boost coroutine library

### Included Libraries
* [__Belle__](https://github.com/octobanana/belle):
  Asynchronous input, modified and included as `./src/ob/belle.hh`
* [__Lispp__](https://github.com/octobanana/lispp):
  embedded programming language, included as `./src/ob/lispp.hh` and `./src/ob/lispp.cc`
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

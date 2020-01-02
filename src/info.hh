/*
                                    88888888
                                  888888888888
                                 88888888888888
                                8888888888888888
                               888888888888888888
                              888888  8888  888888
                              88888    88    88888
                              888888  8888  888888
                              88888888888888888888
                              88888888888888888888
                             8888888888888888888888
                          8888888888888888888888888888
                        88888888888888888888888888888888
                              88888888888888888888
                            888888888888888888888888
                           888888  8888888888  888888
                           888     8888  8888     888
                                   888    888

                                   OCTOBANANA

Licensed under the MIT License

Copyright (c) 2019 Brett Robinson <https://octobanana.com/>

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
*/

#ifndef INFO_HH
#define INFO_HH

#include "ob/parg.hh"
#include "ob/term.hh"

#include <cstddef>

#include <string>
#include <string_view>
#include <iostream>

inline int program_info(OB::Parg& pg);
inline bool program_color(std::string_view color);
inline void program_init(OB::Parg& pg);

inline void program_init(OB::Parg& pg)
{
  pg.name("nyble").version("0.5.0 (01.01.2020)");
  pg.description("A snake game for the terminal.");

  pg.usage("");
  pg.usage("[--colour=<on|off|auto>] -h|--help");
  pg.usage("[--colour=<on|off|auto>] -v|--version");
  pg.usage("[--colour=<on|off|auto>] --license");

  pg.info({"Key Bindings", {
    {"<ctrl-c>", "quit the program"},
    {"<ctrl-z>", "suspend the program"},
    {"<ctrl-l>", "force screen redraw"},
    {";, :", "enter the command prompt"},
    {"<space>", "start/pause the game"},
    {"r", "restart the game"},
    {"R", "restart the game and fit game window to the screen"},
    {"1", "coil the body of the snake to a size of 3, and then extend itself back to its previous size"},
    {"2", "reverse the snakes direction, swapping the head with the tail"},
    {"3", "toggle fixed movement of the snake, a direction key must be pressed or held to move the snake"},
    {",", "move left, 2 key mode"},
    {".", "move right, 2 key mode"},
    {"<", "u-turn left, 2 key mode"},
    {">", "u-turn right, 2 key mode"},
    {"<up>, w, k", "move up"},
    {"<down>, s, j", "move down"},
    {"<left>, a, h", "move left"},
    {"<right>, d, l", "move right"},
  }});

  pg.info({"Command Prompt Bindings", {
    {"<esc>", "exit the prompt"},
    {"<enter>", "submit the input"},
    {"<tab>", "enter autocomplete mode"},
    {"<ctrl-u>", "clear the prompt"},
    {"<up>, <ctrl-p>", "previous history value based on current input"},
    {"<down>, <ctrl-n>", "next history value based on current input"},
    {"<left>, <ctrl-b>", "move cursor left"},
    {"<right>, <ctrl-f>", "move cursor right"},
    {"<home>, <ctrl-a>", "move cursor to the start of the input"},
    {"<end>, <ctrl-e>", "move cursor to the end of the input"},
    {"<delete>, <ctrl-d>", "delete character under the cursor or delete previous character if cursor is at the end of the input"},
    {"<backspace>, <ctrl-h>", "delete previous character"},
  }});

  pg.info({"Autocomplete Prompt Bindings", {
    {"<esc>", "exit autocomplete mode"},
    {"<enter>", "select value under cursor"},
    {"<up>, <ctrl-p>", "move cursor to start of previous section"},
    {"<down>, <ctrl-n>", "move cursor to start of next section"},
    {"<left>, <ctrl-b>", "move cursor left"},
    {"<right>, <ctrl-f>", "move cursor right"},
    {"<home>, <ctrl-a>", "move cursor to the start of first section"},
    {"<end>, <ctrl-e>", "move cursor to the start of last section"},
  }});

  pg.info({"Examples", {
    {"nyble",
      "run the program"},
    {"nyble --help --colour=off",
      "print the help output, without colour"},
    {"nyble --help",
      "print the help output"},
    {"nyble --version",
      "print the program version"},
    {"nyble --license",
      "print the program license"},
  }});

  pg.info({"Exit Codes", {
    {"0", "normal"},
    {"1", "error"},
  }});

  pg.info({"Meta", {
    {"", "The version format is 'major.minor.patch (day.month.year)'."},
  }});

  pg.info({"Repository", {
    {"", "https://github.com/octobanana/nyble.git"},
  }});

  pg.info({"Homepage", {
    {"", "https://octobanana.com/software/nyble"},
  }});

  pg.author("Brett Robinson (octobanana) <octobanana.dev@gmail.com>");

  // general flags
  pg.set("help,h", "Print the help output.");
  pg.set("version,v", "Print the program version.");
  pg.set("license", "Print the program license.");

  // options
  pg.set("colour", "auto", "on|off|auto", "Print the program output with colour either on, off, or auto based on if stdout is a tty, the default value is 'auto'.");

  // allow and capture positional arguments
  // pg.set_pos();
}

inline bool program_color(std::string_view color)
{
  if (color == "on")
  {
    // color on
    return true;
  }

  if (color == "off")
  {
    // color off
    return false;
  }

  // color auto
  return OB::Term::is_term(STDOUT_FILENO);
}

inline int program_info(OB::Parg& pg)
{
  // init info/options
  program_init(pg);

  // parse options
  auto const status {pg.parse()};

  // set output color choice
  pg.color(program_color(pg.get<std::string>("colour")));

  if (status < 0)
  {
    // an error occurred
    std::cerr
    << pg.usage()
    << "\n"
    << pg.error();

    return -1;
  }

  if (pg.get<bool>("help"))
  {
    // show help output
    std::cout << pg.help();

    return 1;
  }

  if (pg.get<bool>("version"))
  {
    // show version output
    std::cout << pg.version();

    return 1;
  }

  if (pg.get<bool>("license"))
  {
    // show license output
    std::cout << pg.license();

    return 1;
  }

  // success
  return 0;
}

#endif // INFO_HH

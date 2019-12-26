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

#ifndef GAME_WORLD_HH
#define GAME_WORLD_HH

#include "ob/parg.hh"
#include "ob/text.hh"
#include "ob/term.hh"
#include "ob/lispp.hh"
#include "ob/timer.hh"
#include "ob/color.hh"
#include "ob/readline.hh"
#include "ob/ordered_map.hh"
#include "ob/belle/io.hh"
#include "ob/belle/signal.hh"

#include <cstddef>
#include <cstdint>

#include <deque>
#include <regex>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <utility>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <functional>
#include <unordered_map>

// Root <- events
// scenes
//   menu
//     objects
//       UI
//   game
//     state
//       start
//       pause
//       over
//       play
//     objects
//       UI
//       Board
//       Snake
//       Egg

// on_winch
// on_input
// on_update
// on_render
//   full
//   update
//   patch

namespace Nyble {

using Read = OB::Belle::IO::Read;
using Key = OB::Belle::IO::Read::Key;
using Mouse = OB::Belle::IO::Read::Mouse;
using Tick = std::chrono::milliseconds;
using Clock = std::chrono::steady_clock;
using Readline = OB::Readline;
using Timer = OB::Belle::asio::steady_timer;
using error_code = OB::Belle::error_code;
using namespace std::chrono_literals;
using namespace std::string_literals;
namespace fs = std::filesystem;
namespace Text = OB::Text;
namespace Term = OB::Term;
namespace Belle = OB::Belle;
namespace iom = OB::Term::iomanip;
namespace aec = OB::Term::ANSI_Escape_Codes;

struct Pos {
  // TODO use signed integer
  std::size_t x {0};
  std::size_t y {0};
  Pos(std::size_t const x, std::size_t const y) : x {x}, y {y} {}
  Pos() = default;
  Pos(Pos&&) = default;
  Pos(Pos const&) = default;
  ~Pos() = default;
  Pos& operator=(Pos&&) = default;
  Pos& operator=(Pos const&) = default;
};

struct Size {
  std::size_t w {0};
  std::size_t h {0};
  Size(std::size_t const w, std::size_t const h) : w {w}, h {h} {}
  Size() = default;
  Size(Size&&) = default;
  Size(Size const&) = default;
  ~Size() = default;
  Size& operator=(Size&&) = default;
  Size& operator=(Size const&) = default;
};

struct Color {
  std::uint8_t r {0};
  std::uint8_t g {0};
  std::uint8_t b {0};
  Color(std::uint8_t const r, std::uint8_t const g, std::uint8_t const b) : r {r}, g {g}, b {b} {}
  Color() = default;
  Color(Color&&) = default;
  Color(Color const&) = default;
  ~Color() = default;
  Color& operator=(Color&&) = default;
  Color& operator=(Color const&) = default;
};

Color hex_to_rgb(std::string const& str);

struct Style {
  friend std::ostream& operator<<(std::ostream& os, Style const& obj);
  enum Type : std::uint8_t {
    Bit_2 = 0,
    Bit_4,
    Bit_8,
    Bit_24,
  };
  enum Attr : std::uint8_t {
    Null = 0,
    Bold = 1 << 0,
    Reverse = 1 << 1,
  };
  std::uint8_t type {Bit_24};
  std::uint8_t attr {Null};
  Color fg;
  Color bg;
};

struct Cell {
  Style style;
  std::string text;
};

class Buffer {
public:
  Buffer() = default;
  Buffer(Buffer&&) = default;
  Buffer(Buffer const&) = default;
  ~Buffer() = default;
  Buffer& operator=(Buffer&&) = default;
  Buffer& operator=(Buffer const&) = default;
  void operator()(Cell const& cell);
  Cell& at(Pos const& pos);
  std::vector<Cell>& row(std::size_t y);
  Cell& col(Pos const& pos);
  Pos cursor();
  void cursor(Pos const& pos);
  Size size();
  void size(Size const& size);
  bool empty();
  void clear();

private:
  Pos _pos;
  Size _size;
  std::vector<std::vector<Cell>> _value;
}; // class Buffer

class Game;

using Ctx = Game*;

class Scene {
public:
  Scene(Ctx ctx) : _ctx {ctx} {}
  Scene(Scene&&) = default;
  Scene(Scene const&) = default;
  virtual ~Scene() = default;
  Scene& operator=(Scene&&) = default;
  Scene& operator=(Scene const&) = default;
  virtual void on_winch(Size const& size) = 0;
  virtual bool on_input(Read::Ctx const& ctx) = 0;
  virtual bool on_update(Tick const delta) = 0;
  virtual bool on_render(Buffer& buf) = 0;

  Ctx _ctx;
  Pos _pos;
  Size _size;
}; // class Scene

using Scenes = OB::Ordered_map<std::string, std::shared_ptr<Scene>>;

class Background : public Scene {
public:
  Background(Ctx ctx);
  Background(Background&&) = default;
  Background(Background const&) = default;
  ~Background();
  Background& operator=(Background&&) = default;
  Background& operator=(Background const&) = default;
  void on_winch(Size const& size);
  bool on_input(Read::Ctx const& ctx);
  bool on_update(Tick const delta);
  bool on_render(Buffer& buf);

// private:
  Cell _cell {Style{Style::Bit_24, Style::Null, Color{}, hex_to_rgb("031323")}, " "};
}; // class Background

class Board : public Scene {
public:
  Board(Ctx ctx);
  Board(Board&&) = default;
  Board(Board const&) = default;
  ~Board();
  Board& operator=(Board&&) = default;
  Board& operator=(Board const&) = default;
  void on_winch(Size const& size);
  bool on_input(Read::Ctx const& ctx);
  bool on_update(Tick const delta);
  bool on_render(Buffer& buf);

// private:
  Style _style {Style::Bit_24, Style::Null, hex_to_rgb("0b253d"), hex_to_rgb("031323")};
  Style _block1 {Style::Bit_24, Style::Null, Color(), hex_to_rgb("031323")};
  Style _block2 {Style::Bit_24, Style::Null, Color(), hex_to_rgb("0b253d")};
  bool _init {true};
}; // class Board

class Snake : public Scene {
public:
  Snake(Ctx ctx);
  Snake(Snake&&) = default;
  Snake(Snake const&) = default;
  ~Snake();
  Snake& operator=(Snake&&) = default;
  Snake& operator=(Snake const&) = default;
  void on_winch(Size const& size);
  bool on_input(Read::Ctx const& ctx);
  bool on_update(Tick const delta);
  bool on_render(Buffer& buf);

// private:

  struct {
    std::size_t idx {0};
    std::vector<Style> body {
      Style{Style::Bit_24, Style::Null, Color(), hex_to_rgb("43c7c3")},
      Style{Style::Bit_24, Style::Null, Color(), hex_to_rgb("3aaca8")}
    };
    Style head {Style::Bit_24, Style::Bold, hex_to_rgb("031323"), hex_to_rgb("4feae7")};
  } _style;

  struct {
    std::vector<std::string> head {"․․", "˙˙", " ⁚", "⁚ "};
    std::string body {"  "};
  } _text;

  struct Block {
    Pos pos;
    Style* style;
    std::string_view value;
  };

  std::deque<Block> _sprite;

  bool _init {true};
  enum Dir {Up, Down, Left, Right};
  Dir _dir_prev {Up};
  std::deque<Dir> _dir;
  enum State {Stopped, Moving, Fixed};
  State _state {Stopped};
  std::size_t _ext {2};
  Tick _delta {0ms};
  Tick _interval {300ms};
  std::unordered_map<char32_t, Xpr> _input;

  bool _hit_wall {false};
  bool _hit_wall_egg {true};
  bool _hit_wall_portal {false};
  bool _hit_body {true};

  enum Special {Normal, Rainbow, Flicker};
  int _special {Special::Normal};
  Tick _special_time {0ms};
  Tick _special_delta {0ms};
  Tick _special_interval {_interval / 2};
  OB::Color _color;

  std::size_t _state_eyes_idx {0};
  std::vector<std::pair<std::function<void(Cell&)>, Tick>> _state_eyes {{[](Cell&) {}, 3000ms}, {[](Cell& cell) {cell.style.attr = Style::Null;}, 100ms}, {[](Cell& cell) {cell.text = " ";}, 50ms}, {[](Cell& cell) {cell.style.attr = Style::Null;}, 100ms}};
  Tick _blink_delta {0ms};
  Tick _blink_interval {3000ms};

  // TODO move duplicate code to separate function
  void state_stopped();
  void state_moving();
  void state_fixed();
}; // class Snake

class Egg : public Scene {
public:
  Egg(Ctx ctx);
  Egg(Egg&&) = default;
  Egg(Egg const&) = default;
  ~Egg();
  Egg& operator=(Egg&&) = default;
  Egg& operator=(Egg const&) = default;
  void on_winch(Size const& size);
  bool on_input(Read::Ctx const& ctx);
  bool on_update(Tick const delta);
  bool on_render(Buffer& buf);

// private:

  struct Styles {
    enum Dir {Up, Down};
    Dir dir {Up};
    std::size_t idx {0};
    std::vector<Style> body {
      Style{Style::Bit_24, Style::Null, Color(), hex_to_rgb("f7ff57")},
      Style{Style::Bit_24, Style::Null, Color(), hex_to_rgb("d8df4c")},
      Style{Style::Bit_24, Style::Null, Color(), hex_to_rgb("c6cc45")},
      Style{Style::Bit_24, Style::Null, Color(), hex_to_rgb("b3b93f")},
      Style{Style::Bit_24, Style::Null, Color(), hex_to_rgb("a2a739")},
      Style{Style::Bit_24, Style::Null, Color(), hex_to_rgb("909533")}
    };
  } _style;
  std::string _text {"  "};
  bool _init {true};
  Tick _delta {0ms};
  Tick _interval {150ms};
  // enum Type {Normal, Rainbow};
  // int _type {Type::Normal};

  void spawn();
}; // class Egg

class Prompt : public Scene {
public:
  Prompt(Ctx ctx);
  Prompt(Prompt&&) = default;
  Prompt(Prompt const&) = default;
  ~Prompt();
  Prompt& operator=(Prompt&&) = default;
  Prompt& operator=(Prompt const&) = default;
  void on_winch(Size const& size);
  bool on_input(Read::Ctx const& ctx);
  bool on_update(Tick const delta);
  bool on_render(Buffer& buf);

// private:
  Readline _readline;
  enum State {Clear, Typing, Display};
  State _state {Clear};
  struct {
    Style prompt {Style::Bit_24, Style::Null, hex_to_rgb("ff5500"), hex_to_rgb("031323")};
    Style text {Style::Bit_24, Style::Null, hex_to_rgb("f0f0f0"), hex_to_rgb("031323")};
    Style success {Style::Bit_24, Style::Null, hex_to_rgb("55ff00"), hex_to_rgb("031323")};
    Style error {Style::Bit_24, Style::Null, hex_to_rgb("ff0000"), hex_to_rgb("031323")};
  } _style;
  std::string _buf;
  bool _status {false};
  Tick _delta {0ms};
  Tick _interval {8000ms};

  void draw();
}; // class Prompt

class Status : public Scene {
public:
  Status(Ctx ctx);
  Status(Status&&) = default;
  Status(Status const&) = default;
  ~Status();
  Status& operator=(Status&&) = default;
  Status& operator=(Status const&) = default;
  void on_winch(Size const& size);
  bool on_input(Read::Ctx const& ctx);
  bool on_update(Tick const delta);
  bool on_render(Buffer& buf);

// private:
  struct {
    Style line {Style::Bit_24, Style::Null, Color(), hex_to_rgb("0b253d")};
    Style name {Style::Bit_24, Style::Bold, hex_to_rgb("031323"), hex_to_rgb("6735a4")};
    Style key {Style::Bit_24, Style::Bold, hex_to_rgb("8242cf"), hex_to_rgb("0b253d")};
    Style val {Style::Bit_24, Style::Null, hex_to_rgb("b140a2"), hex_to_rgb("0b253d")};
  } _style;
  struct {
    std::string line {" "};
    std::string name {" NYBLE 0.4.0 "};
    std::string fps {" FPS "};
    std::string fpsv;
    std::string frames;
    std::string time;
    std::string dir;
  } _text;
  struct {
    std::string up {"↑"};
    std::string down {"↓"};
    std::string left {"←"};
    std::string right {"→"};
  } _sym;

  void widget_fps();
  void widget_dir();
}; // class Status

class Main : public Scene {
public:
  Main(Ctx ctx);
  Main(Main&&) = default;
  Main(Main const&) = default;
  ~Main();
  Main& operator=(Main&&) = default;
  Main& operator=(Main const&) = default;
  void on_winch(Size const& size);
  bool on_input(Read::Ctx const& ctx);
  bool on_update(Tick const delta);
  bool on_render(Buffer& buf);

  bool on_read(Read::Null const& ctx);
  bool on_read(Read::Mouse const& ctx);
  bool on_read(Read::Key const& ctx);

// private:
  Buffer _buf;
  Scenes _scenes;
  bool _dirty {true};
  std::string _focus;
  std::unordered_map<char32_t, Xpr> _input;

  std::vector<std::pair<char32_t, Tick>> _code {{0, 0ms}, {0, 0ms}, {0, 0ms}, {0, 0ms}, {0, 0ms}, {0, 0ms}, {0, 0ms}, {0, 0ms}, {0, 0ms}, {0, 0ms}};
  std::chrono::time_point<Clock> _code_begin {(Clock::time_point::min)()};
}; // class Main

class Game final {
public:
  Game(OB::Parg& pg);
  Game(Game&&) = delete;
  Game(Game const&) = delete;
  ~Game() = default;
  Game& operator=(Game&&) = delete;
  Game& operator=(Game const&) = delete;
  void run();

  std::shared_ptr<Scene> _main;
  Tick _time {0ms};
  std::size_t _frames {0};
  int _fps_actual {0};
  int _fps_dropped {0};
  std::shared_ptr<Env> _env {std::make_shared<Env>()};

// private:
  void start();
  void stop();
  void winch();
  void screen_init();
  void screen_deinit();
  void lang_init();
  void await_signal();
  void await_read();
  void await_tick();
  void on_tick(error_code const& ec, Tick const delta);
  bool on_read(Read::Null const& ctx);
  bool on_read(Read::Mouse const& ctx);
  bool on_read(Read::Key const& ctx);
  void write();

  OB::Parg& _pg;
  Belle::asio::io_context _io {1};
  Belle::Signal _sig {_io};
  Read _read {_io};
  Term::Mode _term_mode;
  Size _size;
  OB::Timer<std::chrono::steady_clock> _tick_timer;
  std::chrono::time_point<Clock> _tick_begin {(Clock::time_point::min)()};
  std::chrono::time_point<Clock> _tick_end {(Clock::time_point::min)()};
  int _fps {30};
  Tick _tick {static_cast<Tick>(1000 / _fps)};
  Timer _timer {_io};
  std::unordered_map<char32_t, Xpr> _input;

  // std::vector<std::vector<>> _collide;

  std::size_t _bsize {0};
  Buffer _buf;
  Buffer _buf_prev;
  Style _style;
  std::string _line;
}; // class Game

}; // namespace Nyble

#endif

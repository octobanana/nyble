#ifndef GAME_WORLD_HH
#define GAME_WORLD_HH

#include "lisp.hh"
#include "ob/parg.hh"
#include "ob/text.hh"
#include "ob/term.hh"
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
using Clock = std::chrono::high_resolution_clock;
using Readline = OB::Readline;
using Timer = OB::Belle::asio::high_resolution_timer;
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

class Game;

using Ctx = Game&;

class Scene {
public:
  Scene(Ctx ctx) : _ctx {ctx} {}
  Scene(Scene&&) = default;
  Scene(Scene const&) = default;
  virtual ~Scene() = default;
  Scene& operator=(Scene&&) = default;
  Scene& operator=(Scene const&) = default;
  virtual void on_winch() = 0;
  virtual bool on_input(Read::Ctx const& ctx) = 0;
  virtual bool on_update(Tick const delta) = 0;
  virtual bool on_render(std::string& buf) = 0;

  Ctx _ctx;
  Pos _pos;
  Size _size;
  std::vector<Pos> _patch;
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
  void on_winch();
  bool on_input(Read::Ctx const& ctx);
  bool on_update(Tick const delta);
  bool on_render(std::string& buf);

// private:
  struct Cell {
    std::string const* style;
    std::string value;
  };
  struct {
    std::string primary {aec::bg_true("#1b1e24")};
  } _color;
  std::vector<std::vector<Cell>> _sprite;
  bool _dirty {true};
  bool _draw {true};

  void draw();
}; // class Background

class Border : public Scene {
public:
  Border(Ctx ctx);
  Border(Border&&) = default;
  Border(Border const&) = default;
  ~Border();
  Border& operator=(Border&&) = default;
  Border& operator=(Border const&) = default;
  void on_winch();
  bool on_input(Read::Ctx const& ctx);
  bool on_update(Tick const delta);
  bool on_render(std::string& buf);

// private:
  struct Cell {
    std::string const* style {nullptr};
    std::string value;
  };
  struct {
    std::string primary {aec::fg_true("#f158dc") + aec::bg_true("#1b1e24")};
  } _color;
  std::vector<std::vector<Cell>> _sprite;
  bool _draw {true};
  bool _dirty {true};

  void draw();
}; // class Border

class Board : public Scene {
public:
  Board(Ctx ctx);
  Board(Board&&) = default;
  Board(Board const&) = default;
  ~Board();
  Board& operator=(Board&&) = default;
  Board& operator=(Board const&) = default;
  void on_winch();
  bool on_input(Read::Ctx const& ctx);
  bool on_update(Tick const delta);
  bool on_render(std::string& buf);

// private:
  struct Cell {
    std::string const* style {nullptr};
    std::string value;
  };
  struct {
    std::string primary {aec::bg_true("#1b1e24")};
    std::string secondary {aec::bg_true("#2c323c")};
  } _color;
  std::vector<std::vector<Cell>> _sprite;
  bool _dirty {true};
  bool _draw {true};

  void draw();
}; // class Board

class Snake : public Scene {
public:
  Snake(Ctx ctx);
  Snake(Snake&&) = default;
  Snake(Snake const&) = default;
  ~Snake();
  Snake& operator=(Snake&&) = default;
  Snake& operator=(Snake const&) = default;
  void on_winch();
  bool on_input(Read::Ctx const& ctx);
  bool on_update(Tick const delta);
  bool on_render(std::string& buf);

// private:
  struct Cell {
    std::string const* style {nullptr};
    std::string value;
    Pos pos;
  };
  struct {
    std::size_t idx {0};
    std::string head {aec::bg_true("#a7d3e5")};
    std::vector<std::string> body {aec::bg_true("#8bb1bf"), aec::bg_true("#7595a1")};
  } _color;
  enum Dir {Up, Down, Left, Right};
  Dir _dir_prev {Up};
  enum State {Stopped, Moving};
  State _state {Stopped};
  std::deque<Dir> _dir;
  std::size_t _ext {2};
  bool _update {false};
  bool _dirty {true};
  Tick _delta {0ms};
  Tick _interval {300ms};
  std::deque<Cell> _sprite;
  std::unordered_map<char32_t, Xpr> _input;

  void state_stopped();
  void state_moving();
}; // class Snake

class Egg : public Scene {
public:
  Egg(Ctx ctx);
  Egg(Egg&&) = default;
  Egg(Egg const&) = default;
  ~Egg();
  Egg& operator=(Egg&&) = default;
  Egg& operator=(Egg const&) = default;
  void on_winch();
  bool on_input(Read::Ctx const& ctx);
  bool on_update(Tick const delta);
  bool on_render(std::string& buf);

// private:
  struct Color {
    enum Dir {Up, Down};
    Dir dir {Up};
    std::size_t idx {0};
    std::vector<std::string> body {aec::bg_true("#f7ff57"), aec::bg_true("#d8df4c"), aec::bg_true("#c6cc45"), aec::bg_true("#b3b93f"), aec::bg_true("#a2a739"), aec::bg_true("#909533")};
  } _color;
  bool _dirty {true};
  Tick _delta {0ms};
  Tick _interval {150ms};

  void spawn();
}; // class Egg

class Hud : public Scene {
public:
  Hud(Ctx ctx);
  Hud(Hud&&) = default;
  Hud(Hud const&) = default;
  ~Hud();
  Hud& operator=(Hud&&) = default;
  Hud& operator=(Hud const&) = default;
  void on_winch();
  bool on_input(Read::Ctx const& ctx);
  bool on_update(Tick const delta);
  bool on_render(std::string& buf);

// private:
  struct {
    std::string text {aec::fg_true("#ff5500") + aec::bg_true("#1b1e24")};
  } _color;
  bool _dirty {true};
  int _score {0};

  void score(int const val);
}; // class Hud

class Prompt : public Scene {
public:
  Prompt(Ctx ctx);
  Prompt(Prompt&&) = default;
  Prompt(Prompt const&) = default;
  ~Prompt();
  Prompt& operator=(Prompt&&) = default;
  Prompt& operator=(Prompt const&) = default;
  void on_winch();
  bool on_input(Read::Ctx const& ctx);
  bool on_update(Tick const delta);
  bool on_render(std::string& buf);

// private:
  Readline _readline;
  enum State {Clear, Typing, Display};
  State _state {Clear};
  bool _dirty {false};
  struct {
    std::string prompt {aec::fg_true("#ff5500") + aec::bg_true("#1b1e24")};
    std::string text {aec::fg_true("#f0f0f0") + aec::bg_true("#1b1e24")};
    std::string success {aec::fg_true("#55ff00") + aec::bg_true("#1b1e24")};
    std::string error {aec::fg_true("#ff0000") + aec::bg_true("#1b1e24")};
  } _color;
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
  void on_winch();
  bool on_input(Read::Ctx const& ctx);
  bool on_update(Tick const delta);
  bool on_render(std::string& buf);

// private:
  struct {
    std::string text {aec::fg_true("#ff5500") + aec::bg_true("#1b1e24")};
  } _color;
}; // class Status

class Game final {
public:
  Game(OB::Parg& pg);
  Game(Game&&) = delete;
  Game(Game const&) = delete;
  ~Game() = default;
  Game& operator=(Game&&) = delete;
  Game& operator=(Game const&) = delete;
  void run();

  Size _size;
  Scenes _scenes;
  std::shared_ptr<Env> _env {std::make_shared<Env>()};
  std::string _focus;
  int _fps_actual {0};

private:
  void start();
  void stop();
  void winch();
  void screen_init();
  void screen_deinit();
  void lang_init();
  void await_signal();
  void await_read();
  void await_tick();
  void on_tick(error_code const& ec);
  bool on_read(Read::Null const& ctx);
  bool on_read(Read::Mouse const& ctx);
  bool on_read(Read::Key const& ctx);
  void buf_clear();
  void buf_flush();

  OB::Parg& _pg;
  Belle::asio::io_context _io {1};
  Belle::Signal _sig {_io};
  Read _read {_io};
  Term::Mode _term_mode;
  std::chrono::time_point<Clock> _tick_begin {(Clock::time_point::min)()};
  std::chrono::time_point<Clock> _tick_end {(Clock::time_point::min)()};
  int _fps {30};
  Tick _tick {static_cast<Tick>(1000 / _fps)};
  Timer _timer {_io};
  std::unordered_map<char32_t, Xpr> _input;
  std::string _buf;
}; // class Game

}; // namespace Nyble








// std::visit([&](auto const& e) {input(e);}, ctx);
// auto const key = std::get_if<Key>(&ctx);
// if (!key) {return;}
// if (auto const v = _input.find(key->ch); v != _input.end()) {
//   eval(v->second, _env);
// }









// struct Egg {
//   Point pos;
//   std::string sprite {"  "};
//   std::string color {aec::bg_magenta_bright};
//   Egg() = default;
//   Egg(Egg&&) = default;
//   Egg(Egg const&) = default;
//   ~Egg() = default;
//   Egg& operator=(Egg&&) = default;
//   Egg& operator=(Egg const&) = default;
// };

// struct Snake {
//   enum Dir {up, down, left, right};
//   std::deque<Dir> dir;
//   Dir dir_prev {up};
//   std::deque<Point> pos;
//   std::size_t ext {0};
//   std::string sprite {"  "};
//   struct {
//     std::size_t idx {0};
//     std::string head {aec::bg_cyan};
//     std::vector<std::string> body {aec::bg_blue, aec::bg_blue_bright};
//   } color;
//   Snake() = default;
//   Snake(Snake&&) = default;
//   Snake(Snake const&) = default;
//   ~Snake() = default;
//   Snake& operator=(Snake&&) = default;
//   Snake& operator=(Snake const&) = default;
// };

// class World final {
// public:
//   World(OB::Parg& pg) : _pg {pg} {}
//   World(World&&) = default;
//   World(World const&) = delete;
//   ~World() = default;
//   World& operator=(World&&) = default;
//   World& operator=(World const&) = delete;
//   void run();

// private:
//   OB::Parg& _pg;
//   Readline _readline;
//   Belle::asio::io_context _io {1};
//   Belle::Signal _sig {_io};
//   Belle::IO::Read _read {_io};
//   Term::Mode _term_mode;

//   std::chrono::time_point<std::chrono::high_resolution_clock> _time_render_begin {(std::chrono::high_resolution_clock::time_point::min)()};
//   std::chrono::time_point<std::chrono::high_resolution_clock> _time_render_end {(std::chrono::high_resolution_clock::time_point::min)()};
//   std::chrono::milliseconds _tick_min {20ms};
//   std::chrono::milliseconds _tick_max {10ms};
//   std::chrono::milliseconds _tick_default {16ms};
//   Tick<std::chrono::milliseconds> _tick {_tick_min, _tick_max, _tick_default};
//   Belle::asio::high_resolution_timer _timer {_io, _tick.val()};

//   std::stringstream _buf;
//   std::size_t _win_width {0};
//   std::size_t _win_height {0};
//   std::size_t _grid_width {0};
//   std::size_t _grid_height {0};
//   std::shared_ptr<Env> _env {std::make_shared<Env>()};

//   Egg _egg;
//   Snake _snake;
//   std::size_t _score {0};

//   struct {
//     std::string border {aec::fg_cyan};
//     std::string score_text {aec::fg_cyan};
//     std::string score_value {aec::fg_magenta};
//     std::string grid_primary {aec::bg_true("#1b1e24")};
//     std::string grid_secondary {aec::bg_true("#2c323c")};
//   } _color;

//   enum class State {menu, start, game, end, prompt};
//   State _state {State::menu};
//   std::stack<State> _state_stk;

//   void buf_clear();
//   void buf_print();
//   void win_size();
//   std::string win_clear();
//   std::string win_cursor(Point const& obj);
//   std::string grid_cursor(Point const& obj);

//   void draw_border(Point const& pos, Size const& size, std::function<void(Point const&)> const& fx, std::function<void(Point const&)> const& fy);
//   void draw_rect(Point const& pos, Size const& size, std::function<void(Point const&)> const& fx, std::function<void(Point const&)> const& fy);
//   void draw_grid(Point const& pos, std::size_t const w, std::size_t const h);
//   void draw_score();
//   void update_score();
//   void spawn_egg();
//   void game_redraw();
//   void game_menu();
//   void game_init();
//   void game_play();
//   void game_over();

//   void signals();
//   void lang();

//   void do_timer();
//   void on_timer(Belle::error_code const& ec);

//   std::unordered_map<char32_t, Xpr> _input_key;
//   void input_play(Belle::IO::Read::Ctx const& ctx);
//   void input_prompt(Belle::IO::Read::Ctx const& ctx);
//   void input();
// };

// template<typename T>
// class Tick {
// public:
//   Tick(T const t, T const min, T const max) : _min {min}, _max {max} {val(t);}
//   Tick(Tick&&) = default;
//   Tick(Tick const&) = default;
//   ~Tick() = default;
//   Tick& operator=(Tick&&) = default;
//   Tick& operator=(Tick const&) = default;
//   Tick& slow() {
//     _mode = Mode::slow;
//     return *this;
//   }
//   Tick& norm() {
//     _mode = Mode::norm;
//     return *this;
//   }
//   T const& min() {
//     return _min;
//   }
//   T const& max() {
//     return _max;
//   }
//   T const& val() {
//     return _mode == Mode::norm ? _val : _max;
//   }
//   Tick& val(T const& t) {
//     if (t > _max) {_val = _max;}
//     else if (t < _min) {_val = _min;}
//     else {_val = t;}
//     return *this;
//   }
//   Tick& inc(T const& t) {
//     if (_val + t > _max) {_val = _max;}
//     else {_val += t;}
//     return *this;
//   }
//   Tick& dec(T const& t) {
//     if (_val - t < _min) {_val = _min;}
//     else {_val -= t;}
//     return *this;
//   }
// private:
//   T _min;
//   T _max;
//   T _val;
//   enum class Mode {slow, norm};
//   Mode _mode {Mode::norm};
// };

#endif

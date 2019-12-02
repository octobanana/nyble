#ifndef GAME_WORLD_HH
#define GAME_WORLD_HH

#include "lisp.hh"
#include "ob/parg.hh"
#include "ob/text.hh"
#include "ob/term.hh"
#include "ob/readline.hh"
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

namespace Nyble {

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

struct Cell {
  std::string const* style {nullptr};
  std::string value;
  Cell(std::string const* style, std::string const& value) : style {style}, value {value} {};
  Cell() = default;
  Cell(Cell&&) = default;
  Cell(Cell const&) = default;
  ~Cell() = default;
  Cell& operator=(Cell&&) = default;
  Cell& operator=(Cell const&) = default;
};

struct Ctx {
  Size size;
  Ctx() = default;
  Ctx(Ctx&&) = default;
  Ctx(Ctx const&) = default;
  ~Ctx() = default;
  Ctx& operator=(Ctx&&) = default;
  Ctx& operator=(Ctx const&) = default;
};

class Scene {
public:
  Scene(Ctx const* ctx) : _ctx {ctx} {}
  Scene(Scene&&) = default;
  Scene(Scene const&) = default;
  virtual ~Scene() = default;
  Scene& operator=(Scene&&) = default;
  Scene& operator=(Scene const&) = default;
  virtual bool render(std::ostream& buf) = 0;
  virtual void update() = 0;
  virtual void input() = 0;
  virtual void winch() = 0;

protected:
  Ctx const* _ctx;
  Pos _pos;
  Size _size;
}; // class Scene

class Border : public Scene {
public:
  Border(Ctx const* ctx);
  Border(Border&&) = default;
  Border(Border const&) = default;
  ~Border();
  Border& operator=(Border&&) = default;
  Border& operator=(Border const&) = default;
  bool render(std::ostream& buf);
  void update();
  void input();
  void winch();

private:
  // struct {
  //   std::string _line_top {"─"};
  //   std::string _line_bottom {"─"};
  //   std::string _line_left {"│"};
  //   std::string _line_right {"│"};
  //   std::string _corner_top_left {"┌"};
  //   std::string _corner_top_right {"┐"};
  //   std::string _corner_bottom_left {"└"};
  //   std::string _corner_bottom_right {"┘"};
  // } _ch;
  struct {
    std::string primary {aec::fg_true("#93a1a1")};
  } _color;
  std::vector<std::vector<Cell>> _sprite;
  bool _draw {true};
  bool _dirty {true};

  void draw();
}; // class Border

class Board : public Scene {
public:
  Board(Ctx const* ctx);
  Board(Board&&) = default;
  Board(Board const&) = default;
  ~Board();
  Board& operator=(Board&&) = default;
  Board& operator=(Board const&) = default;
  bool render(std::ostream& buf);
  void update();
  void input();
  void winch();

private:
  struct {
    std::string primary {aec::bg_true("#1b1e24")};
    std::string secondary {aec::bg_true("#2c323c")};
  } _color;
  std::vector<std::vector<Cell>> _sprite;
  bool _dirty {true};
  bool _draw {true};

  void draw();
}; // class Board

class Game final {
public:
  Game(OB::Parg& pg) : _pg {pg} {}
  Game(Game&&) = delete;
  Game(Game const&) = delete;
  ~Game() = default;
  Game& operator=(Game&&) = delete;
  Game& operator=(Game const&) = delete;
  void run();

private:
  void start();
  void stop();
  void winch();
  void screen_init();
  void screen_deinit();
  void lang_init();
  void readline_init();
  void await_signal();
  void await_read();
  void await_tick();
  void on_tick(error_code const& ec);
  bool on_read(Belle::IO::Read::Null const& ctx);
  bool on_read(Belle::IO::Read::Mouse const& ctx);
  bool on_read(Belle::IO::Read::Key const& ctx);
  void buf_clear();
  void buf_flush();

  OB::Parg& _pg;
  Readline _readline;
  Belle::asio::io_context _io {1};
  Belle::Signal _sig {_io};
  Belle::IO::Read _read {_io};
  Term::Mode _term_mode;
  std::chrono::time_point<Clock> _tick_begin {(Clock::time_point::min)()};
  std::chrono::time_point<Clock> _tick_end {(Clock::time_point::min)()};
  Tick _tick {16ms};
  Timer _timer {_io};
  std::shared_ptr<Env> _env {std::make_shared<Env>()};
  std::unordered_map<char32_t, Xpr> _input;
  std::stringstream _buf;
  Ctx _ctx;
  std::vector<std::shared_ptr<Scene>> _scenes;
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

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

using Readline = OB::Readline;
using namespace std::chrono_literals;
using namespace std::string_literals;
namespace fs = std::filesystem;
namespace Text = OB::Text;
namespace Term = OB::Term;
namespace Belle = OB::Belle;
namespace iom = OB::Term::iomanip;
namespace aec = OB::Term::ANSI_Escape_Codes;

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

class Scene {
public:
  Scene() = default;
  Scene(Scene&&) = default;
  Scene(Scene const&) = default;
  virtual ~Scene() = 0;
  Scene& operator=(Scene&&) = default;
  Scene& operator=(Scene const&) = default;
  virtual void render() = 0;
  virtual void update() = 0;
  virtual void input() = 0;
private:
};

class Root : Scene {
public:
  Root() = default;
  Root(Root&&) = default;
  Root(Root const&) = default;
  ~Root();
  Root& operator=(Root&&) = default;
  Root& operator=(Root const&) = default;
  void render();
  void update();
  void input();
private:
  // std::vector<Scene> _scenes{};
};

class World final {
public:
  World(OB::Parg& pg) : _pg {pg} {}
  World(World&&) = default;
  World(World const&) = delete;
  ~World() = default;
  World& operator=(World&&) = default;
  World& operator=(World const&) = delete;
  void run();

private:
  OB::Parg& _pg;
  Readline _readline;
  Belle::asio::io_context _io {1};
  Belle::Signal _sig {_io};
  Belle::IO::Read _read {_io};
  Term::Mode _term_mode;

  template<typename T>
  class Tick {
  public:
    Tick(T const t, T const min, T const max) : _min {min}, _max {max} {val(t);}
    Tick(Tick&&) = default;
    Tick(Tick const&) = default;
    ~Tick() = default;
    Tick& operator=(Tick&&) = default;
    Tick& operator=(Tick const&) = default;
    Tick& slow() {
      _mode = Mode::slow;
      return *this;
    }
    Tick& norm() {
      _mode = Mode::norm;
      return *this;
    }
    T const& min() {
      return _min;
    }
    T const& max() {
      return _max;
    }
    T const& val() {
      return _mode == Mode::norm ? _val : _max;
    }
    Tick& val(T const& t) {
      if (t > _max) {_val = _max;}
      else if (t < _min) {_val = _min;}
      else {_val = t;}
      return *this;
    }
    Tick& inc(T const& t) {
      if (_val + t > _max) {_val = _max;}
      else {_val += t;}
      return *this;
    }
    Tick& dec(T const& t) {
      if (_val - t < _min) {_val = _min;}
      else {_val -= t;}
      return *this;
    }
  private:
    T _min;
    T _max;
    T _val;
    enum class Mode {slow, norm};
    Mode _mode {Mode::norm};
  };
  std::chrono::time_point<std::chrono::high_resolution_clock> _time_render_begin {(std::chrono::high_resolution_clock::time_point::min)()};
  std::chrono::time_point<std::chrono::high_resolution_clock> _time_render_end {(std::chrono::high_resolution_clock::time_point::min)()};
  std::chrono::milliseconds _tick_min {500ms};
  std::chrono::milliseconds _tick_max {20ms};
  std::chrono::milliseconds _tick_default {300ms};
  Tick<std::chrono::milliseconds> _tick {_tick_min, _tick_max, _tick_default};
  Belle::asio::high_resolution_timer _timer {_io, _tick.val()};

  struct Point {
    std::size_t x {0};
    std::size_t y {0};
    Point(std::size_t const x, std::size_t const y) : x {x}, y {y} {}
    Point() = default;
    Point(Point&&) = default;
    Point(Point const&) = default;
    ~Point() = default;
    Point& operator=(Point&&) = default;
    Point& operator=(Point const&) = default;
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

  struct Egg {
    Point pos;
    std::string sprite {"  "};
    std::string color {aec::bg_magenta_bright};
    Egg() = default;
    Egg(Egg&&) = default;
    Egg(Egg const&) = default;
    ~Egg() = default;
    Egg& operator=(Egg&&) = default;
    Egg& operator=(Egg const&) = default;
  };

  struct Snake {
    enum Dir {up, down, left, right};
    std::deque<Dir> dir;
    Dir dir_prev {up};
    std::deque<Point> pos;
    std::size_t ext {0};
    std::string sprite {"  "};
    struct {
      std::size_t idx {0};
      std::string head {aec::bg_cyan};
      std::vector<std::string> body {aec::bg_blue, aec::bg_blue_bright};
    } color;
    Snake() = default;
    Snake(Snake&&) = default;
    Snake(Snake const&) = default;
    ~Snake() = default;
    Snake& operator=(Snake&&) = default;
    Snake& operator=(Snake const&) = default;
  };

  Egg _egg;
  Snake _snake;

  std::size_t _score {0};

  std::stringstream _buf;

  struct {
    std::string border {aec::fg_cyan};
    std::string score_text {aec::fg_cyan};
    std::string score_value {aec::fg_magenta};
    std::string grid_primary {aec::bg_true("#1b1e24")};
    std::string grid_secondary {aec::bg_true("#2c323c")};
  } _color;

  std::size_t _win_width {0};
  std::size_t _win_height {0};
  std::size_t _grid_width {0};
  std::size_t _grid_height {0};

  enum class State {menu, start, game, end, prompt};
  State _state {State::menu};
  std::stack<State> _state_stk;

  std::shared_ptr<Env> _env {std::make_shared<Env>()};

  void buf_clear();
  void buf_print();

  void win_size();
  std::string win_clear();
  std::string win_cursor(Point const& obj);

  std::string grid_cursor(Point const& obj);

  void draw_border(Point const& pos, Size const& size, std::function<void(Point const&)> const& fx, std::function<void(Point const&)> const& fy);
  void draw_rect(Point const& pos, Size const& size, std::function<void(Point const&)> const& fx, std::function<void(Point const&)> const& fy);
  void draw_grid(Point const& pos, std::size_t const w, std::size_t const h);

  void draw_score();
  void update_score();
  void spawn_egg();

  void game_redraw();
  void game_menu();
  void game_init();
  void game_play();
  void game_over();

  void signals();
  void lang();

  void do_timer();
  void on_timer(Belle::error_code const& ec);

  using Fn = std::function<void()>;
  std::unordered_map<std::string, Fn> _fn;
  std::unordered_map<char32_t, Fn> _input_play;
  void fn_init();
  void input_play_init();
  void input_play(Belle::IO::Read::Ctx const& ctx);
  void input_prompt(Belle::IO::Read::Ctx const& ctx);
  void input();
};

#endif

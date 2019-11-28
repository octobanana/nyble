#include "game/world.hh"
#include "ob/rect.hh"

#include <unistd.h>

#include <ctime>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <random>
#include <variant>
#include <iostream>

using Rect = OB::Rect;
using Key = OB::Belle::IO::Read::Key;
using Mouse = OB::Belle::IO::Read::Mouse;

std::size_t random_range(std::size_t l, std::size_t u) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<std::size_t> distr(l, u);
  return distr(gen);
}

void World::buf_clear() {
  _buf.clear();
  _buf.str(std::string());
}

void World::buf_print() {
  // _term_mode.set_cooked();
  // std::cout << _buf.rdbuf() << std::flush;
  // _term_mode.set_raw();
  // return;

  std::string const buf {_buf.str()};
  int num {0};
  char const* ptr {buf.data()};
  std::size_t size {buf.size()};
  // std::cerr << "DBG> sz: " << buf.size() << "\n";
  while (size > 0 && (num = write(STDIN_FILENO, ptr, size)) != size) {
    if (num < 0) {
      if (errno == EINTR || errno == EAGAIN) {continue;}
      // std::cerr << "DBG> er: " << errno << " -> " << strerror(errno) << "\n";
      throw std::runtime_error("write failed");
    }
    size -= num;
    ptr += num;
    // std::cerr << "DBG> num: " << num << "\n";
  }
}

void World::draw_border(Point const& pos, Size const& size, std::function<void(Point const&)> const& fx, std::function<void(Point const&)> const& fy) {
  for (std::size_t y = 0; y < size.h; ++y) {
    fy({pos.x, pos.y + y});
    if (y == 0) {
      // bottom
      for (std::size_t x = 0; x < size.w; ++x) {
        if (x == 0 || x == size.w - 1) {
          // outer
          _buf << "  ";
        }
        else {
          // inner
          _buf << "  ";
        }
      }
    }
    else if (y == size.h - 1) {
      // top
      for (std::size_t x = 0; x < size.w; ++x) {
        if (x == 0 || x == size.w - 1) {
          // outer
          _buf << "  ";
        }
        else {
          // inner
          _buf << "  ";
        }
      }
    }
    else {
      // middle
      for (std::size_t x = 0; x < size.w; ++x) {
        if (x == 0 || x == size.w - 1) {
          // outer
          _buf << "  ";
        }
        else {
          // inner
          _buf << aec::cursor_right(2);
        }
      }
    }
  }
}

void World::draw_rect(Point const& pos, Size const& size, std::function<void(Point const&)> const& fx, std::function<void(Point const&)> const& fy) {
  for (std::size_t y = 0; y < size.h; ++y) {
    fy({pos.x, pos.y + y});
    for (std::size_t x = 0; x < size.w; ++x) {
      fx({pos.x + x, pos.y + y});
    }
  }
}

void World::draw_grid(Point const& pos, std::size_t const w, std::size_t const h) {
  for (std::size_t y = 0; y < h; ++y) {
    _buf << grid_cursor({pos.x, y + pos.y});
    for (std::size_t x = 0; x < w; ++x) {
      if ((x + pos.x) % 2) {
        if ((y + pos.y) % 2) {_buf << _color.grid_primary << "  ";}
        else {_buf << _color.grid_secondary << "  ";}
      }
      else {
        if ((y + pos.y) % 2) {_buf << _color.grid_secondary << "  ";}
        else {_buf << _color.grid_primary << "  ";}
      }
    }
  }
  _buf << aec::clear;
}

void World::draw_score() {
  _buf << win_cursor({2, _win_height - 1});
  _buf << _color.score_text << "SCORE: " << _color.score_value << _score << aec::clear;
}

void World::update_score() {
  _buf << win_cursor({9, _win_height - 1});
  _buf << _color.score_value << _score << aec::clear;
}

void World::win_size() {
  Term::size(_win_width, _win_height);
  _readline.size(_win_width, _win_height);
  _grid_width = _win_width - 4;
  if (_grid_width % 2 != 0) {_grid_width -= 1;}
  _grid_width /= 2;
  _grid_height = _win_height - 4;
}

std::string World::win_clear() {
  return aec::screen_clear;
}

std::string World::win_cursor(Point const& obj) {
  return aec::cursor_set(1 + obj.x, _win_height - obj.y);
}

std::string World::grid_cursor(Point const& obj) {
  return aec::cursor_set(3 + (obj.x * 2), _win_height - 2 - obj.y);
}

void World::game_redraw() {
  buf_clear();
  _buf << win_clear();

  draw_grid({0, 0}, _grid_width, _grid_height);

  _buf << grid_cursor(_snake.pos.front());
  _buf << _snake.color.head << _snake.sprite << aec::clear;
  for (std::size_t i = 1; i < _snake.pos.size(); ++i) {
    if (_snake.pos.at(i).x < 0 || _snake.pos.at(i).y < 0) {break;}
    _buf << grid_cursor(_snake.pos.at(i));
    _buf << _snake.color.body.at(_snake.color.idx) << _snake.sprite << aec::clear;
    if (++_snake.color.idx >= _snake.color.body.size()) {_snake.color.idx = 0;}
  }

  _buf << grid_cursor(_egg.pos);
  _buf << _egg.color << _egg.sprite << aec::clear;

  Rect r1;
  r1.xy_max((_grid_width * 2) + 3, _grid_height + 3);
  r1.xy(1, 1);
  r1.wh((_grid_width * 2) + 2, _grid_height + 2);
  r1.border(1, 1, 1, 1);
  r1.border_fg(_color.border);
  _buf << r1 << aec::clear;

  draw_score();

  buf_print();
  buf_clear();
}

void World::game_init() {
  _state = State::menu;
  _tick = {_tick_min, _tick_max, _tick_default};
  _egg = {};
  _snake = {};
  _score = 0;
  _state = State::game;
  _timer.expires_at((std::chrono::high_resolution_clock::time_point::min)());
  _timer.cancel();

  buf_clear();
  _buf << win_clear();

  _snake.dir.clear();
  _snake.dir_prev = Snake::up;
  _snake.ext = 2;
  _snake.pos.emplace_back((_grid_width - 1) / 2, 0);

  _egg.pos = {(_grid_width - 1) / 2, (_grid_height - 1) / 2};

  game_redraw();
}

void World::game_play() {
  // determine next snake head position
  Point head {_snake.pos.front()};
  Snake::Dir dir;
  if (_snake.dir.size()) {
    dir = _snake.dir.front();
    _snake.dir.pop_front();
  }
  else {
    dir = _snake.dir_prev;
  }
  switch (_snake.dir_prev) {
    case Snake::up: {
      if (dir == Snake::down) {dir = Snake::up;}
      switch (dir) {
        case Snake::up: {head.y += 1; break;}
        case Snake::left: {head.x -= 1; break;}
        case Snake::right: {head.x += 1; break;}
        break;
      }
      break;
    }
    case Snake::down: {
      if (dir == Snake::up) {dir = Snake::down;}
      switch (dir) {
        case Snake::down: {head.y -= 1; break;}
        case Snake::left: {head.x -= 1; break;}
        case Snake::right: {head.x += 1; break;}
        break;
      }
      break;
    }
    case Snake::left: {
      if (dir == Snake::right) {dir = Snake::left;}
      switch (dir) {
        case Snake::left: {head.x -= 1; break;}
        case Snake::up: {head.y += 1; break;}
        case Snake::down: {head.y -= 1; break;}
        break;
      }
      break;
    }
    case Snake::right: {
      if (dir == Snake::left) {dir = Snake::right;}
      switch (dir) {
        case Snake::right: {head.x += 1; break;}
        case Snake::up: {head.y += 1; break;}
        case Snake::down: {head.y -= 1; break;}
        break;
      }
      break;
    }
  }

  buf_clear();

  // check if snake has collided with a wall
  bool hit_wall {false};
  if (head.x < 0 || head.x > _grid_width - 1 ||
      head.y < 0 || head.y > _grid_height - 1) {
    hit_wall = true;
  }
  if (hit_wall) {
    draw_grid(_egg.pos, 1, 1);
    buf_print();
    _state = State::end;
    _tick.norm().val(_tick.min());
    do_timer();
    return;
  }

  // check if snake has collided with itself
  bool hit_snake {false};
  for (auto const& [x, y] : _snake.pos) {
    if (head.x == x && head.y == y) {
      hit_snake = true;
      break;
    }
  }
  if (hit_snake) {
    draw_grid(_egg.pos, 1, 1);
    buf_print();
    _state = State::end;
    _tick.norm().val(_tick.min());
    do_timer();
    return;
  }

  // slow down tick rate if about to collide into wall
  if ((head.x == 0 && dir == Snake::left) ||
      (head.x == _grid_width - 1 && dir == Snake::right) ||
      (head.y == 0 && dir == Snake::up) ||
      (head.y == _grid_height - 1 && dir == Snake::down)) {
    _tick.slow();
  }
  else {_tick.norm();}

  // snake has hit egg, update and draw egg
  bool new_egg {false};
  if (head.x == _egg.pos.x && head.y == _egg.pos.y) {
    new_egg = true;
    _tick.dec(3ms);
    _score += 10;
    update_score();
  }

  _snake.dir_prev = dir;
  _snake.pos.emplace_front(head);

  _buf << grid_cursor(_snake.pos.front());
  _buf << _snake.color.head << _snake.sprite << aec::clear;

  _buf << grid_cursor(_snake.pos.at(1));
  _buf << _snake.color.body.at(_snake.color.idx) << _snake.sprite << aec::clear;
  if (++_snake.color.idx >= _snake.color.body.size()) {_snake.color.idx = 0;}

  if (_snake.ext) {_snake.ext -= 1;}
  else {
    draw_grid(_snake.pos.back(), 1, 1);
    _snake.pos.pop_back();
  }

  if (new_egg) {
    _snake.ext += 3;
    spawn_egg();
    _buf << grid_cursor(_egg.pos);
    _buf << _egg.color << _egg.sprite << aec::clear;
  }

  buf_print();

  do_timer();
}

void World::game_over() {
  if (_snake.pos.empty()) {
    _state = State::end;
    buf_clear();

  // {
  //   Point pos {1, (_grid_height / 2) - (_grid_height / 4)};
  //   Size size {_grid_width - 2, _grid_height / 2};
  //   _buf << aec::bg_true("#cb4b16");
  //   draw_border(pos, size,
  //     [&](auto const& p) {},
  //     [&](auto const& p) {_buf << grid_cursor(p);});
  //   _buf << aec::clear;
  // }

    // {
    //   Size size {12, 5};
    //   Point pos {(_grid_width / 2) - (size.w / 2), (_grid_height / 2) - (size.w / 2)};
    //   // _buf << aec::bg_true("#1b1e24");
    //   draw_rect(pos, size,
    //       [&](auto const& p) {_buf << "  ";},
    //       [&](auto const& p) {_buf << grid_cursor(p);});
    //   // _buf << aec::clear;
    // }

    Rect r;
    r.xy_max(_win_width + 1, _win_height + 1);
    r.xy(0, (_win_height / 2) - 1);
    r.wh(_win_width, 3);
    r.color_fg("cyan"s);
    // r.color_bg("#1b1e24"s);
    r.text("GAME OVER\n\nPress [R] to Restart");
    r.align(Rect::Align::center, Rect::Align::center);
    _buf << r;
    buf_print();
    return;
  }

  buf_clear();

  draw_grid(_snake.pos.back(), 1, 1);
  _snake.pos.pop_back();

  buf_print();

  do_timer();
}

void World::spawn_egg() {
  do {
    _egg.pos = {random_range(0, _grid_width - 1), random_range(0, _grid_height - 1)};
  } while ([&]() {
    for (auto const& [x, y] : _snake.pos) {
      if (_egg.pos.x == x && _egg.pos.y == y) {return true;}
    }
    return false;
  }());
}

void World::game_menu() {
}

void World::do_timer() {
  if (_time_render_begin != (std::chrono::high_resolution_clock::time_point::min)()) {
    _time_render_end = std::chrono::high_resolution_clock::now();
  }
  _timer.expires_at(std::chrono::high_resolution_clock::now() - (_time_render_end - _time_render_begin) + _tick.val());
  _timer.async_wait([&](auto ec) {
    _time_render_begin = std::chrono::high_resolution_clock::now();
    on_timer(ec);
  });
}

void World::on_timer(Belle::error_code const& ec) {
  if (ec) {
    _time_render_begin = (std::chrono::high_resolution_clock::time_point::min)();
    _time_render_end = (std::chrono::high_resolution_clock::time_point::min)();
    return;
  }
  switch (_state) {
    case State::menu: {
      game_menu();
      break;
    }
    case State::start: {
      game_init();
      break;
    }
    case State::game: {
      game_play();
      break;
    }
    case State::end: {
      game_over();
      break;
    }
  }
}

void World::signals() {
  _sig.on_signal({SIGINT, SIGTERM}, [&](auto const& ec, auto sig) {
    // std::cerr << "\nEvent: " << Belle::Signal::str(sig) << "\n";
    _io.stop();
  });

  _sig.on_signal(SIGWINCH, [&](auto const& ec, auto sig) {
    win_size();
    _sig.wait();
  });

  _sig.wait();
}

void World::lang() {
  // TODO add variable get/set functions to help debug and test
  env_init(_env, 0, nullptr);
  (*_env)["snake-len?"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
    return num_xpr(static_cast<Int>(_snake.pos.size()));
  }}, _env, Val::evaled};
  (*_env)["snake-ext?"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
    return num_xpr(static_cast<Int>(_snake.ext));
  }}, _env, Val::evaled};
  (*_env)["snake-ext"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
    auto x = eval(sym_xpr("a"), e);
    if (auto const v = xpr_int(&x)) {
      _snake.ext += static_cast<std::size_t>(*v);
      return x;
    }
    throw std::runtime_error("expected number");
  }}, _env, Val::evaled};
  (*_env)["tick"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
    auto x = eval(sym_xpr("a"), e);
    if (auto const v = xpr_int(&x)) {
      _tick.val(static_cast<std::chrono::milliseconds>(static_cast<std::size_t>(*v)));
      return x;
    }
    throw std::runtime_error("expected number");
  }}, _env, Val::evaled};
  (*_env)["egg-pos"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
    return Xpr{Lst{num_xpr(static_cast<Int>(_egg.pos.x)), num_xpr(static_cast<Int>(_egg.pos.y))}};
  }}, _env, Val::evaled};
}

void World::input_play(Belle::IO::Read::Ctx const& ctx) {
  auto const pkey = std::get_if<Key>(&ctx);
  if (!pkey) {return;}
  // OB::Text::Char32 key {pkey->ch, pkey->str};
  switch (pkey->ch) {
    case Term::ctrl_key('c'): {
      std::cout
      << aec::erase_down
      << std::flush;
      kill(getpid(), SIGINT);
      break;
    }
    case Term::ctrl_key('l'): {
      game_redraw();
      break;
    }
    case ':': {
      // pause
      _timer.expires_at((std::chrono::high_resolution_clock::time_point::min)());
      _timer.cancel();
      // init prompt
      _state_stk.push(_state);
      _state = State::prompt;
      buf_clear();
      _buf << win_cursor({0, 0});
      _buf << _readline.clear().refresh().render();
      buf_print();
      break;
    }
    case 'q': case 'Q': {
      _io.stop();
      break;
    }
    case 'r': {
      _timer.expires_at((std::chrono::high_resolution_clock::time_point::min)());
      _timer.cancel();
      game_init();
      break;
    }
    case 'z': {
      _snake.ext += _snake.pos.size() - 1;
      buf_clear();
      while (_snake.pos.size() > 1) {
        draw_grid(_snake.pos.back(), 1, 1);
        _snake.pos.pop_back();
      }
      buf_print();
      break;
    }
    case '?': {
      _timer.expires_at((std::chrono::high_resolution_clock::time_point::min)());
      _timer.cancel();

      std::cout
      << aec::mouse_disable
      << aec::nl
      << aec::screen_pop
      << aec::cursor_show
      << std::flush;
      _term_mode.set_cooked();

      std::system(("$(which less) -ir <<'EOF'\n" + _pg.help() + "EOF").c_str());

      _term_mode.set_raw();
      std::cout
      << aec::cursor_hide
      << aec::screen_push
      << aec::cursor_hide
      << aec::mouse_enable
      << aec::screen_clear
      << aec::cursor_home
      << std::flush;

      game_redraw();
      break;
    }
    case Key::Space: case 'p': {
      if (_timer.expiry() == (std::chrono::high_resolution_clock::time_point::min)()) {
        buf_clear();
        _buf << win_cursor({0, 0});
        _buf << aec::erase_line;
        buf_print();
        do_timer();
      }
      else {
        _timer.expires_at((std::chrono::high_resolution_clock::time_point::min)());
        _timer.cancel();
      }
      break;
    }
    case ',': {
      Snake::Dir dir;
      if (_snake.dir.size()) {dir = _snake.dir.back();}
      else {dir = _snake.dir_prev;}
      switch (dir) {
        case Snake::up: {
          _snake.dir.emplace_back(Snake::left);
          break;
        }
        case Snake::down: {
          _snake.dir.emplace_back(Snake::right);
          break;
        }
        case Snake::left: {
          _snake.dir.emplace_back(Snake::down);
          break;
        }
        case Snake::right: {
          _snake.dir.emplace_back(Snake::up);
          break;
        }
      }
      break;
    }
    case '.': {
      Snake::Dir dir;
      if (_snake.dir.size()) {dir = _snake.dir.back();}
      else {dir = _snake.dir_prev;}
      switch (dir) {
        case Snake::up: {
          _snake.dir.emplace_back(Snake::right);
          break;
        }
        case Snake::down: {
          _snake.dir.emplace_back(Snake::left);
          break;
        }
        case Snake::left: {
          _snake.dir.emplace_back(Snake::up);
          break;
        }
        case Snake::right: {
          _snake.dir.emplace_back(Snake::down);
          break;
        }
      }
      break;
    }
    case Key::Up: case 'w': case 'k': {
      if ((_snake.dir.empty() && _snake.dir_prev != Snake::up) || (_snake.dir.size() && _snake.dir.back() != Snake::up)) {
        _snake.dir.emplace_back(Snake::up);
      }
      break;
    }
    case Key::Down: case 's': case 'j': {
      if ((_snake.dir.empty() && _snake.dir_prev != Snake::down) || (_snake.dir.size() && _snake.dir.back() != Snake::down)) {
        _snake.dir.emplace_back(Snake::down);
      }
      break;
    }
    case Key::Left: case 'a': case 'h': {
      if ((_snake.dir.empty() && _snake.dir_prev != Snake::left) || (_snake.dir.size() && _snake.dir.back() != Snake::left)) {
        _snake.dir.emplace_back(Snake::left);
      }
      break;
    }
    case Key::Right: case 'd': case 'l': {
      if ((_snake.dir.empty() && _snake.dir_prev != Snake::right) || (_snake.dir.size() && _snake.dir.back() != Snake::right)) {
        _snake.dir.emplace_back(Snake::right);
      }
      break;
    }
  }
}

void World::input_prompt(Belle::IO::Read::Ctx const& ctx) {
  auto const pkey = std::get_if<Key>(&ctx);
  if (!pkey) {return;}
  OB::Text::Char32 key {pkey->ch, pkey->str};
  buf_clear();
  _buf << win_cursor({0, 0});
  _buf << aec::erase_line;
  if (! _readline(key)) {
    _buf << _readline.render();
    buf_print();
    return;
  }
  else {
    auto input = _readline.get();
    if (input.size()) {
      try {
        if (auto x = read(input)) {
          auto v = eval(*x, _env);
          _buf << aec::fg_green_bright << ">" << aec::clear << cprint(v);
        }
      }
      catch (std::exception const& e) {
        _buf << aec::fg_red << ">" << aec::clear << cprint(*read("(err \""s + e.what() + "\")"s));
      }
      // deinit prompt
      _buf << aec::cursor_hide;
      _state = _state_stk.top();
      _state_stk.pop();
    }
    else {
      // deinit prompt
      _buf << aec::cursor_hide << aec::erase_line;
      _state = _state_stk.top();
      _state_stk.pop();
    }
  }
  buf_print();
}

void World::input() {
  // debug
  // _readline.hist_load("./history.txt");
  _readline.style("");
  _readline.prompt(">", aec::fg_magenta_bright);
  _readline.refresh();
  _readline.boundaries(" `',@()[]{}");
  _readline.autocomplete([&]() {
    std::vector<std::string> values;
    for (auto const& [k, v] : _env->inner) {
      values.emplace_back(k);
    }
    return values;
  });

  _read.on_read([&](auto const& ctx) {
    switch (_state) {
      case State::prompt: {
        input_prompt(ctx);
        break;
      }
      case State::menu: {
        input_play(ctx);
        break;
      }
      case State::start: {
        input_play(ctx);
        break;
      }
      case State::game: {
        input_play(ctx);
        break;
      }
      case State::end: {
        input_play(ctx);
        break;
      }
    }
  });
  _read.run();
}

void World::run() {
  signals();
  win_size();
  lang();
  input();

  std::cout
  << aec::cursor_hide
  << aec::screen_push
  << aec::cursor_hide
  << aec::mouse_enable
  << aec::screen_clear
  << aec::cursor_home
  << std::flush;
  _term_mode.set_raw();

  game_init();
  _io.run();

  std::cout
  << aec::mouse_disable
  << aec::nl
  << aec::screen_pop
  << aec::cursor_show
  << std::flush;
  _term_mode.set_cooked();
}

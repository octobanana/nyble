#include "game/world.hh"

#include <unistd.h>

#include <ctime>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <random>
#include <variant>
#include <iostream>

namespace Nyble {

  // Utils ----------------------------------------------------------------------------

std::size_t random_range(std::size_t l, std::size_t u) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<std::size_t> distr(l, u);
  return distr(gen);
}

// Background -----------------------------------------------------------------------

Background::Background(Ctx ctx) : Scene(ctx) {
}

Background::~Background() {
}

void Background::on_winch() {
  _draw = true;
}

bool Background::on_input(Read::Ctx const& ctx) {
  return false;
}

bool Background::on_update(Tick const delta) {
  return false;
}

bool Background::on_render(std::string& buf) {
  if (_draw) {
    draw();
  }
  if (_dirty) {
    _dirty = false;
    _patch.clear();
    std::size_t y {0};
    for (auto const& row : _sprite) {
      buf += aec::cursor_set(_pos.x + 1, _size.h - (_pos.y + y++));
      for (auto const& col : row) {
        if (col.style) {buf += *col.style;}
        buf += col.value;
      }
    }
    buf += aec::clear;
    return true;
  }
  if (_patch.size()) {
    for (auto const& pos : _patch) {
      if (pos.x >= _size.w || pos.y >= _size.h) {continue;}
      buf += aec::cursor_set(_pos.x + pos.x + 1, _size.h - (_pos.y + pos.y));
      auto const& cell = _sprite.at(pos.y).at(pos.x);
      if (cell.style) {buf += *cell.style;}
      buf += cell.value;
    }
    buf += aec::clear;
    _patch.clear();
    return true;
  }
  return false;
}

void Background::draw() {
  _draw = false;
  _dirty = true;
  _sprite.clear();
  _size = Size(_ctx._size);
  for (std::size_t h = 0; h < _size.h; ++h) {
    _sprite.emplace_back(std::vector<Cell>());
    auto& row = _sprite.back();
    for (std::size_t w = 0; w < _size.w; ++w) {
      row.emplace_back(Cell{&_color.primary, " "});
    }
  }
}

// Border -----------------------------------------------------------------------

Border::Border(Ctx ctx) : Scene(ctx) {
}

Border::~Border() {
}

void Border::on_winch() {
  _dirty = true;
}

bool Border::on_input(Read::Ctx const& ctx) {
  return false;
}

bool Border::on_update(Tick const delta) {
  return false;
}

bool Border::on_render(std::string& buf) {
  if (_draw) {
    draw();
  }
  if (_dirty) {
    _dirty = false;
    _pos = Pos((_ctx._size.w / 4) - (_size.w / 2), (_ctx._size.h / 2) - (_size.h / 2));
    if (_pos.y % 2) {++_pos.y;}
    std::size_t y {0};
    for (auto const& row : _sprite) {
      buf += aec::cursor_set((_pos.x * 2) + 1, _ctx._size.h - (_pos.y + y++));
      for (auto const& col : row) {
        if (col.style) {buf += *col.style;}
        buf += col.value;
      }
    }
    buf += aec::clear;
    return true;
  }
  return false;
}

void Border::draw() {
  _draw = false;
  _sprite.clear();
  _size = Size([&]{if (auto const w = _ctx._size.w - 4; w % 2) {return (w - 1) / 2;} else {return w / 2;}}(), _ctx._size.h - 3);
  for (std::size_t y = 0; y < _size.h; ++y) {
    _sprite.emplace_back(std::vector<Cell>());
    auto& row = _sprite.back();
      if (y == 0) {
        // bottom
        for (std::size_t x = 0; x < _size.w; ++x) {
          if (x == 0) {
            // outer left
            row.emplace_back(Cell{&_color.primary, " └"});
          }
          else if (x == _size.w - 1) {
            // outer right
            row.emplace_back(Cell{&_color.primary, "┘ "});
          }
          else {
            // inner
            row.emplace_back(Cell{&_color.primary, "──"});
          }
        }
      }
      else if (y == _size.h - 1) {
        // top
        for (std::size_t x = 0; x < _size.w; ++x) {
          if (x == 0) {
            // outer left
            row.emplace_back(Cell{&_color.primary, " ┌"});
          }
          else if (x == _size.w - 1) {
            // outer right
            row.emplace_back(Cell{&_color.primary, "┐ "});
          }
          else {
            // inner
            row.emplace_back(Cell{&_color.primary, "──"});
          }
        }
      }
      else {
        // middle
        for (std::size_t x = 0; x < _size.w; ++x) {
          if (x == 0) {
            // outer left
            row.emplace_back(Cell{&_color.primary, " │"});
          }
          else if (x == _size.w - 1) {
            // outer right
            row.emplace_back(Cell{&_color.primary, "│ "});
          }
          else {
            // inner
            row.emplace_back(Cell{&_color.primary, "  "});
          }
        }
      }
  }
}

// Board -----------------------------------------------------------------------

Board::Board(Ctx ctx) : Scene(ctx) {
  _size = Size([&]{if (auto const w = _ctx._size.w - 8; w % 2) {return (w - 1) / 2;} else {return w / 2;}}(), _ctx._size.h - 5);
  _pos = Pos((_ctx._size.w / 4) - (_size.w / 2), (_ctx._size.h / 2) - (_size.h / 2));
  if (_pos.y % 2 == 0) {++_pos.y;}
}

Board::~Board() {
}

void Board::on_winch() {
  _dirty = true;
}

bool Board::on_input(Read::Ctx const& ctx) {
  return false;
}

bool Board::on_update(Tick const delta) {
  return false;
}

bool Board::on_render(std::string& buf) {
  if (_draw) {
    draw();
  }
  if (_dirty) {
    _dirty = false;
    _patch.clear();
    _pos = Pos((_ctx._size.w / 4) - (_size.w / 2), (_ctx._size.h / 2) - (_size.h / 2));
    if (_pos.y % 2 == 0) {++_pos.y;}
    std::size_t y {0};
    for (auto const& row : _sprite) {
      buf += aec::cursor_set((_pos.x * 2) + 1, _ctx._size.h - (_pos.y + y++));
      for (auto const& col : row) {
        if (col.style) {buf += *col.style;}
        buf += col.value;
      }
    }
    buf += aec::clear;
    return true;
  }
  if (_patch.size()) {
    for (auto const& pos : _patch) {
      if (pos.x >= _size.w || pos.y >= _size.h) {continue;}
      buf += aec::cursor_set((_pos.x * 2) + (pos.x * 2) + 1, _ctx._size.h - (_pos.y + pos.y));
      auto const& cell = _sprite.at(pos.y).at(pos.x);
      if (cell.style) {buf += *cell.style;}
      buf += cell.value;
    }
    buf += aec::clear;
    _patch.clear();
    return true;
  }
  return false;
}

void Board::draw() {
  _draw = false;
  _sprite.clear();
  bool swap {false};
  for (std::size_t h = 0; h < _size.h; ++h) {
    _sprite.emplace_back(std::vector<Cell>());
    auto& row = _sprite.back();
    for (std::size_t w = 0; w < _size.w; ++w) {
      if (swap) {
        row.emplace_back(Cell{&_color.primary, "  "});
      }
      else {
        row.emplace_back(Cell{&_color.secondary, "  "});
      }
      swap = !swap;
    }
    if (_size.w % 2 == 0) {swap = !swap;}
  }
}

// Snake -----------------------------------------------------------------------

Snake::Snake(Ctx ctx) : Scene(ctx) {
  auto const& _env = _ctx._env;

  (*_env)["pause"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
    switch (_state) {
      case Stopped: {
        _state = Moving;
        break;
      }
      case Moving: {
        _state = Stopped;
        break;
      }
    }
    return sym_xpr("T");
  }}, _env, Val::evaled};

  (*_env)["left2"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
    if (_state == Stopped || _dir.size() >= 8) return sym_xpr("F");
    Dir dir;
    if (_dir.size()) {dir = _dir.back();}
    else {dir = _dir_prev;}
    switch (dir) {
      case Up: {
        _dir.emplace_back(Left);
        break;
      }
      case Down: {
        _dir.emplace_back(Right);
        break;
      }
      case Left: {
        _dir.emplace_back(Down);
        break;
      }
      case Right: {
        _dir.emplace_back(Up);
        break;
      }
    }
    return sym_xpr("T");
  }}, _env, Val::evaled};

  (*_env)["right2"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
    if (_state == Stopped || _dir.size() >= 8) return sym_xpr("F");
    Dir dir;
    if (_dir.size()) {dir = _dir.back();}
    else {dir = _dir_prev;}
    switch (dir) {
      case Up: {
        _dir.emplace_back(Right);
        break;
      }
      case Down: {
        _dir.emplace_back(Left);
        break;
      }
      case Left: {
        _dir.emplace_back(Up);
        break;
      }
      case Right: {
        _dir.emplace_back(Down);
        break;
      }
    }
    return sym_xpr("T");
  }}, _env, Val::evaled};

  _input[Key::Space] = *read("(pause)");
  _input[','] = *read("(left2)");
  _input['.'] = *read("(right2)");
  // _input[Key::Up] = *read("(up)");
  // _input['w'] = *read("(up)");
  // _input['k'] = *read("(up)");
  // _input[Key::Down] = *read("(down)");
  // _input['s'] = *read("(down)");
  // _input['j'] = *read("(down)");
  // _input[Key::Left] = *read("(left)");
  // _input['a'] = *read("(left)");
  // _input['h'] = *read("(left)");
  // _input[Key::Right] = *read("(right)");
  // _input['d'] = *read("(right)");
  // _input['l'] = *read("(right)");
  // _input['z'] = *read("(coil)");

  auto const& board = _ctx._scenes.at("board");
  _sprite.emplace_back(Cell{&_color.head, "  ", Pos{board->_size.w / 2, 0}});
}

Snake::~Snake() {
}

void Snake::on_winch() {
  _dirty = true;
}

bool Snake::on_input(Read::Ctx const& ctx) {
  if (auto const v = std::get_if<Key>(&ctx)) {
    if (auto const x = _input.find(v->ch); x != _input.end()) {
      eval(x->second, _ctx._env);
      return true;
    }
  }
  return false;
}

bool Snake::on_update(Tick const delta) {
  switch (_state) {
    case Stopped: {
      state_stopped();
      return false;
    }
    case Moving: {
      _delta += delta;
      if (_delta >= _interval) {
        _update = true;
        while (_delta > delta) {
          _delta -= _interval;
          state_moving();
        }
        return true;
      }
    }
  }
  return false;
}

bool Snake::on_render(std::string& buf) {
  if (_dirty) {
    _dirty = false;
    _update = false;
    auto const& board = _ctx._scenes.at("board");
    for (auto it = _sprite.rbegin(); it != _sprite.rend(); ++it) {
      auto const& cell = *it;
      buf += aec::cursor_set(((cell.pos.x + board->_pos.x) * 2) + 1, _ctx._size.h - (cell.pos.y) - board->_pos.y);
      if (cell.style) {buf += *cell.style;}
      buf += cell.value;
    }
    buf += aec::clear;
    return true;
  }
  if (_update) {
    _update = false;
    auto const& board = _ctx._scenes.at("board");
    auto const update = [&](auto const& cell) {
      buf += aec::cursor_set(((cell.pos.x + board->_pos.x) * 2) + 1, _ctx._size.h - (cell.pos.y) - board->_pos.y);
      if (cell.style) {buf += *cell.style;}
      buf += cell.value;
    };
    update(_sprite.front());
    update(_sprite.at(1));
    return true;
  }
  return false;
}

void Snake::state_stopped() {
}

void Snake::state_moving() {
  Pos head {_sprite.front().pos};
  Dir dir;
  if (_dir.size()) {
    dir = _dir.front();
    _dir.pop_front();
  }
  else {
    dir = _dir_prev;
  }
  switch (_dir_prev) {
    case Up: {
      if (dir == Down) {dir = Up;}
      switch (dir) {
        case Up: {head.y += 1; break;}
        case Left: {head.x -= 1; break;}
        case Right: {head.x += 1; break;}
        break;
      }
      break;
    }
    case Down: {
      if (dir == Up) {dir = Down;}
      switch (dir) {
        case Down: {head.y -= 1; break;}
        case Left: {head.x -= 1; break;}
        case Right: {head.x += 1; break;}
        break;
      }
      break;
    }
    case Left: {
      if (dir == Right) {dir = Left;}
      switch (dir) {
        case Left: {head.x -= 1; break;}
        case Up: {head.y += 1; break;}
        case Down: {head.y -= 1; break;}
        break;
      }
      break;
    }
    case Right: {
      if (dir == Left) {dir = Right;}
      switch (dir) {
        case Right: {head.x += 1; break;}
        case Up: {head.y += 1; break;}
        case Down: {head.y -= 1; break;}
        break;
      }
      break;
    }
  }

  // hit wall
  {
    bool hit_wall {false};
    auto const& board = _ctx._scenes.at("board");
    if (head.x < 0 || head.x >= board->_size.w ||
        head.y < 0 || head.y >= board->_size.h) {
      hit_wall = true;
    }
    if (hit_wall) {
      std::cerr << "collide> " << "snake -> wall" << "\n";
      _dir.clear();
      return;
    }
  }

  // hit snake
  {
    bool hit_snake {false};
    for (auto const& cell : _sprite) {
      if (head.x == cell.pos.x && head.y == cell.pos.y) {
        hit_snake = true;
        break;
      }
    }
    if (hit_snake) {
      std::cerr << "collide> " << "snake -> snake" << "\n";
      _dir.clear();
      return;
    }
  }

  // hit egg
  bool hit_egg {false};
  auto& egg = *std::dynamic_pointer_cast<Egg>(_ctx._scenes.at("egg"));
  if (head.x == egg._pos.x && head.y == egg._pos.y) {
    hit_egg = true;
  }

  if (_ext) {
    _ext -= 1;
  }
  else {
    auto const& board = _ctx._scenes.at("board");
    board->_patch.emplace_back(_sprite.back().pos);
    _sprite.pop_back();
  }

  _dir_prev = dir;
  _sprite.emplace_front(Cell{&_color.head, "  ", head});
  _sprite.at(1).style = &_color.body.at(_color.idx);
  if (++_color.idx >= _color.body.size()) {_color.idx = 0;}

  if (hit_egg) {
    std::cerr << "collide> " << "snake -> egg" << "\n";
    _ext += 2;
    _interval -= 5ms;
    if (_interval < 20ms) {_interval = 20ms;}
    egg.spawn();
    auto& hud = *std::dynamic_pointer_cast<Hud>(_ctx._scenes.at("hud"));
    hud.score(10);
  }

  // slow down tick rate if about to collide into wall
  {
    // if ((head.x == 0 && dir == Snake::left) ||
    //     (head.x == _grid_width - 1 && dir == Snake::right) ||
    //     (head.y == 0 && dir == Snake::up) ||
    //     (head.y == _grid_height - 1 && dir == Snake::down)) {
    //   _tick.slow();
    // }
    // else {_tick.norm();}
  }

}

// Egg -----------------------------------------------------------------------

Egg::Egg(Ctx ctx) : Scene(ctx) {
  on_winch();
  auto const& board = _ctx._scenes.at("board")->_size;
  _pos = Pos(board.w / 2, board.h / 2);
}

Egg::~Egg() {
}

void Egg::on_winch() {
  _dirty = true;
}

bool Egg::on_input(Read::Ctx const& ctx) {
  return false;
}

bool Egg::on_update(Tick const delta) {
  _delta += delta;
  if (_delta >= _interval) {
    while (_delta > delta) {
      _delta -= _interval;
      _dirty = true;
      // TODO update state here
      if (_color.dir == Color::Up) {
        if (++_color.idx == _color.body.size()) {
          _color.dir = Color::Down;
          _color.idx -= 2;
        }
      }
      else {
        if (--_color.idx == 0) {
          _color.dir = Color::Up;
        }
      }
    }
    return true;
  }
  return false;
}

bool Egg::on_render(std::string& buf) {
  if (_dirty) {
    _dirty = false;
    auto const& board = _ctx._scenes.at("board");
    buf += aec::cursor_set(((_pos.x + board->_pos.x) * 2) + 1, _ctx._size.h - (_pos.y) - board->_pos.y);
    buf += _color.body.at(_color.idx);
    buf += "  ";
    buf += aec::clear;
    return true;
  }
  return false;
}

void Egg::spawn() {
  _dirty = true;
  auto const& board = _ctx._scenes.at("board")->_size;
  auto const& snake = std::dynamic_pointer_cast<Snake>(_ctx._scenes.at("snake"))->_sprite;
  auto const is_valid = [&]() {
    for (auto const& e : snake) {
      if (_pos.x == e.pos.x && _pos.y == e.pos.y) {return false;}
    }
    return true;
  };
  do {_pos = {random_range(0, board.w - 1), random_range(0, board.h - 1)};}
  while (!is_valid());
}

// Hud -----------------------------------------------------------------------

Hud::Hud(Ctx ctx) : Scene(ctx) {
  on_winch();
}

Hud::~Hud() {
}

void Hud::on_winch() {
  _dirty = true;
  _size = Size(_ctx._size.w, 1);
  _pos = Pos(0, _ctx._size.h - 1);
}

bool Hud::on_input(Read::Ctx const& ctx) {
  return false;
}

bool Hud::on_update(Tick const delta) {
  return false;
}

bool Hud::on_render(std::string& buf) {
  if (_dirty) {
    _dirty = false;
    std::size_t size {0};
    buf += aec::cursor_set(_pos.x + 1, _ctx._size.h - _pos.y);
    buf += aec::fg_true("#23262c");
    buf += aec::bg_true("#93a1a1");
    buf += " SCORE ";
    size += sizeof(" SCORE ");
    buf += aec::bold;
    buf += std::to_string(_score);
    buf += aec::nbold;
    size += std::to_string(_score).size();
    for (size; size <= _size.w; ++size) {buf += " ";}
    buf += aec::clear;
    return true;
  }
  return false;
}

void Hud::score(int const val) {
  _dirty = true;
  _score += val;
}

// Prompt -----------------------------------------------------------------------

Prompt::Prompt(Ctx ctx) : Scene(ctx) {
  on_winch();
  auto const& _env = _ctx._env;
  _readline.hist_load("./history.txt"); // debug
  _readline.style(_color.text);
  _readline.prompt(">", _color.prompt);
  _readline.refresh();
  _readline.boundaries(" `',@()[]{}");
  _readline.autocomplete([&]() {
    std::vector<std::string> values;
    for (auto const& [k, v] : _env->inner) {
      values.emplace_back(k);
    }
    return values;
  });
}

Prompt::~Prompt() {
}

void Prompt::on_winch() {
  if (_state != Clear) {_dirty = true;}
  _size = _ctx._size;
  _readline.size(_size.w, _size.h);
}

bool Prompt::on_input(Read::Ctx const& ctx) {
  if (auto const v = std::get_if<Key>(&ctx)) {
    _dirty = true;
    OB::Text::Char32 key {v->ch, v->str};
    if (! _readline(key)) {
      _state = Typing;
      return true;
    }
    else {
      auto input = _readline.get();
      _readline.clear();
      if (input.size()) {
        _buf.clear();
        try {
          if (auto x = read(input)) {
            auto v = eval(*x, _ctx._env);
            _buf = print(v);
            _status = true;
          }
        }
        catch (std::exception const& e) {
          _buf = print(*read("(err \""s + e.what() + "\")"s));
          _status = false;
        }
        _delta = 0ms;
        _dirty = true;
        _state = Display;
        _ctx._focus = "snake";
      }
      else {
        _delta = 0ms;
        _dirty = true;
        _state = Clear;
        _ctx._focus = "snake";
      }
    }
    return true;
  }
  return false;
}

bool Prompt::on_update(Tick const delta) {
  if (_state == Display) {
    _delta += delta;
    if (_delta >= _interval) {
      _delta = 0ms;
      _dirty = true;
      _state = Clear;
      return true;
    }
  }
  return false;
}

bool Prompt::on_render(std::string& buf) {
  if (_dirty) {
    _dirty = false;
    switch (_state) {
      case Clear: {
        auto const& background = _ctx._scenes.at("background");
        for (auto x = _pos.x, w = _pos.x + _size.w; x < w; ++x) {
          background->_patch.emplace_back(Pos{x, _pos.y});
        }
        return true;
      }
      case Typing: {
        buf += aec::cursor_set((_pos.x) + 1, _size.h - (_pos.y));
        buf += _readline.refresh().render();
        return true;
      }
      case Display: {
        Text::View vres {_buf};
        auto const res = vres.colstr(0, _size.w - 1);
        buf += aec::cursor_set((_pos.x) + 1, _size.h - (_pos.y));
        buf += _color.text;
        for (std::size_t i = 0; i < _size.w; ++i) {buf += " ";}
        buf += aec::cr;
        buf += (_status ? _color.success : _color.error);
        buf += ">";
        buf += aec::clear;
        buf += _color.text;
        buf += res;
        buf += std::string(vres.size(), ' ');
        buf += aec::clear;
        _readline.clear();
        return true;
      }
    }
    return true;
  }
  return false;
}

// Status -----------------------------------------------------------------------

Status::Status(Ctx ctx) : Scene(ctx) {
  on_winch();
}

Status::~Status() {
}

void Status::on_winch() {
  _size = _ctx._size;
}

bool Status::on_input(Read::Ctx const& ctx) {
  return false;
}

bool Status::on_update(Tick const delta) {
  return false;
}

bool Status::on_render(std::string& buf) {
  std::size_t size {0};
  buf += aec::cursor_set(1, _size.h - 1);
  buf += aec::fg_true("#23262c");
  buf += aec::bold;
  buf += aec::bg_cyan;
  buf += " NYBLE ";
  size += sizeof(" NYBLE ");
  buf += aec::nbold;
  buf += aec::bg_true("#93a1a1");
  buf += " FPS ";
  size += sizeof(" FPS ");
  buf += aec::bold;
  buf += std::to_string(_ctx._fps_actual);
  buf += aec::nbold;
  size += std::to_string(_ctx._fps_actual).size();
  for (--size; size <= _size.w; ++size) {buf += " ";}
  buf += aec::clear;
  return false;
}

// Game ------------------------------------------------------------------------

Game::Game(OB::Parg& pg) : _pg {pg} {
  _buf.reserve(1000000);
  // _input['q'] = *read("(quit)");
  // _input[Term::ctrl_key('c')] = *read("(sigint)");
  // _input[Term::ctrl_key('l')] = *read("(refresh)");
  // _input[':'] = *read("(prompt)");
  // _input['?'] = *read("(help)");

  // _input[Key::Space] = *read("(pause)");
  // _input['r'] = *read("(restart)");
}

void Game::start() {
  _io.run();
}

void Game::stop() {
  _io.stop();
}

void Game::winch() {
  // TODO check min x,y window size
  // TODO allow fixed x,y window size
  Term::size(_size.w, _size.h);
  for (auto const& e : _scenes) {
    e->second->on_winch();
  }
  _buf += aec::screen_clear;
}

void Game::screen_init() {
  std::cout
  << aec::cursor_hide
  << aec::screen_push
  << aec::cursor_hide
  << aec::mouse_enable
  << aec::screen_clear
  << aec::cursor_home
  << std::flush;
  _term_mode.set_raw();
}

void Game::screen_deinit() {
  std::cout
  << aec::mouse_disable
  << aec::nl
  << aec::screen_pop
  << aec::cursor_show
  << std::flush;
  _term_mode.set_cooked();
}

void Game::lang_init() {
  env_init(_env, 0, nullptr);

  // TODO add bindings

  (*_env)["fps"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
    auto x = eval(sym_xpr("a"), e);
    if (auto const v = xpr_int(&x)) {
      _fps = static_cast<int>(*v);
      _tick = static_cast<Tick>(1000 / _fps);
      return x;
    }
    throw std::runtime_error("expected 'Int'");
  }}, _env, Val::evaled};
}

void Game::await_signal() {
  _sig.on_signal({SIGINT, SIGTERM}, [&](auto const& ec, auto sig) {
    // std::cerr << "\nEvent: " << Belle::Signal::str(sig) << "\n";
    stop();
  });

  _sig.on_signal(SIGWINCH, [&](auto const& ec, auto sig) {
    // std::cerr << "\nEvent: " << Belle::Signal::str(sig) << "\n";
    _sig.wait();
    winch();
  });

  _sig.on_signal(SIGTSTP, [&](auto const& ec, auto sig) {
    // std::cerr << "\nEvent: " << Belle::Signal::str(sig) << "\n";
    _sig.wait();
    _timer.cancel();
    screen_deinit();
    kill(getpid(), SIGSTOP);
  });

  _sig.on_signal(SIGCONT, [&](auto const& ec, auto sig) {
    // std::cerr << "\nEvent: " << Belle::Signal::str(sig) << "\n";
    _sig.wait();
    winch();
    screen_init();
    await_tick();
  });

  _sig.wait();
}

void Game::await_read() {
  _read.on_read([&](auto const& ctx) {
    bool found {false};
    if (_focus != "prompt") {
      std::visit([&](auto const& e) {found = on_read(e);}, ctx);
    }
    if (!found) {
      // TODO call input for active scene
      if (auto const v = _scenes.find(_focus); v != _scenes.map_end()) {
        v->second->on_input(ctx);
      }
    }
  });

  _read.run();
}

void Game::await_tick() {
  _tick_end = std::chrono::high_resolution_clock::now();
  _fps_actual = std::round(1000000000 / static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(_tick_end - _tick_begin + std::chrono::nanoseconds(_tick)).count()));
  _timer.expires_at(std::chrono::high_resolution_clock::now() - (_tick_end - _tick_begin) + _tick);
  _timer.async_wait([&](auto ec) {
    _tick_begin = std::chrono::high_resolution_clock::now();
    on_tick(ec);
  });
}

void Game::on_tick(Belle::error_code const& ec) {
  if (ec) {
    _tick_begin = (std::chrono::high_resolution_clock::time_point::min)();
    _tick_end = (std::chrono::high_resolution_clock::time_point::min)();
    return;
  }

  // TODO main update/render here
  // auto const tm = std::chrono::time_point_cast<std::chrono::nanoseconds>(Clock::now()).time_since_epoch().count();
  // std::cerr << "tick> " << tm << "\n";

  for (auto const& e : _scenes) {
    if (e->second->on_update(_tick)) {
      std::cerr << "update> " << e->first << "\n";
    }
  }

  for (auto const& e : _scenes) {
    if (e->second->on_render(_buf)) {
      std::cerr << "render> " << e->first << "\n";
    }
  }

  buf_flush();

  await_tick();
}

bool Game::on_read(Read::Null const& ctx) {
  // TODO log error
  return true;
}

bool Game::on_read(Read::Mouse const& ctx) {
  if (auto const x = _input.find(ctx.ch); x != _input.end()) {
    eval(x->second, _env);
    return true;
  }
  return false;
}

bool Game::on_read(Read::Key const& ctx) {
  switch (ctx.ch) {
    case 'q':
    case OB::Term::ctrl_key('c'): {
      kill(getpid(), SIGINT);
      return true;
    }
    case OB::Term::ctrl_key('z'): {
      kill(getpid(), SIGTSTP);
      return true;
    }
    case OB::Term::ctrl_key('l'): {
      winch();
      return true;
    }
    case ':': {
      _focus = "prompt";
      auto const snake = std::dynamic_pointer_cast<Snake>(_scenes.at("snake"));
      snake->_state = Snake::State::Stopped;
      auto const prompt = std::dynamic_pointer_cast<Prompt>(_scenes.at("prompt"));
      prompt->_state = Prompt::State::Typing;
      prompt->_dirty = true;
      return true;
    }
  }
  if (auto const x = _input.find(ctx.ch); x != _input.end()) {
    eval(x->second, _env);
    return true;
  }
  return false;
}

void Game::buf_clear() {
  _buf.clear();
}

void Game::buf_flush() {
  int num {0};
  char const* ptr {_buf.data()};
  std::size_t size {_buf.size()};
  // std::cerr << "write> " << size << "\n";
  while (size > 0 && (num = write(STDIN_FILENO, ptr, size)) != size) {
    if (num < 0) {
      if (errno == EINTR || errno == EAGAIN) {continue;}
      throw std::runtime_error("write failed");
    }
    size -= num;
    ptr += num;
    // std::cerr << "write> " << size << "\n";
  }
  buf_clear();
}

void Game::run() {
  winch();
  screen_init();
  lang_init();
  await_signal();
  await_read();
  await_tick();

  // TODO init scenes here

  // Scene Game
  _scenes("background", std::make_shared<Background>(*this));
  _scenes("prompt", std::make_shared<Prompt>(*this));
  _scenes("status", std::make_shared<Status>(*this));
  _scenes("border", std::make_shared<Border>(*this));
  _scenes("board", std::make_shared<Board>(*this));
  _scenes("snake", std::make_shared<Snake>(*this));
  _scenes("egg", std::make_shared<Egg>(*this));
  _scenes("hud", std::make_shared<Hud>(*this));

  _focus = "snake";

  start();
  screen_deinit();
}

}; // namespace Nyble

// std::size_t random_range(std::size_t l, std::size_t u) {
//   std::random_device rd;
//   std::mt19937 gen(rd());
//   std::uniform_int_distribution<std::size_t> distr(l, u);
//   return distr(gen);
// }

// void World::buf_clear() {
//   _buf.clear();
//   _buf.str(std::string());
// }

// void World::buf_print() {
//   // _term_mode.set_cooked();
//   // std::cout << _buf.rdbuf() << std::flush;
//   // _term_mode.set_raw();
//   // return;

//   std::string const buf {_buf.str()};
//   int num {0};
//   char const* ptr {buf.data()};
//   std::size_t size {buf.size()};
//   // std::cerr << "DBG> sz: " << buf.size() << "\n";
//   while (size > 0 && (num = write(STDIN_FILENO, ptr, size)) != size) {
//     if (num < 0) {
//       if (errno == EINTR || errno == EAGAIN) {continue;}
//       // std::cerr << "DBG> er: " << errno << " -> " << strerror(errno) << "\n";
//       throw std::runtime_error("write failed");
//     }
//     size -= num;
//     ptr += num;
//     // std::cerr << "DBG> num: " << num << "\n";
//   }
// }

// void World::draw_border(Point const& pos, Size const& size, std::function<void(Point const&)> const& fx, std::function<void(Point const&)> const& fy) {
//   for (std::size_t y = 0; y < size.h; ++y) {
//     fy({pos.x, pos.y + y});
//     if (y == 0) {
//       // bottom
//       for (std::size_t x = 0; x < size.w; ++x) {
//         if (x == 0 || x == size.w - 1) {
//           // outer
//           _buf << "  ";
//         }
//         else {
//           // inner
//           _buf << "  ";
//         }
//       }
//     }
//     else if (y == size.h - 1) {
//       // top
//       for (std::size_t x = 0; x < size.w; ++x) {
//         if (x == 0 || x == size.w - 1) {
//           // outer
//           _buf << "  ";
//         }
//         else {
//           // inner
//           _buf << "  ";
//         }
//       }
//     }
//     else {
//       // middle
//       for (std::size_t x = 0; x < size.w; ++x) {
//         if (x == 0 || x == size.w - 1) {
//           // outer
//           _buf << "  ";
//         }
//         else {
//           // inner
//           _buf << aec::cursor_right(2);
//         }
//       }
//     }
//   }
// }

// void World::draw_rect(Point const& pos, Size const& size, std::function<void(Point const&)> const& fx, std::function<void(Point const&)> const& fy) {
//   for (std::size_t y = 0; y < size.h; ++y) {
//     fy({pos.x, pos.y + y});
//     for (std::size_t x = 0; x < size.w; ++x) {
//       fx({pos.x + x, pos.y + y});
//     }
//   }
// }

// void World::draw_grid(Point const& pos, std::size_t const w, std::size_t const h) {
//   for (std::size_t y = 0; y < h; ++y) {
//     _buf << grid_cursor({pos.x, y + pos.y});
//     for (std::size_t x = 0; x < w; ++x) {
//       if ((x + pos.x) % 2) {
//         if ((y + pos.y) % 2) {_buf << _color.grid_primary << "  ";}
//         else {_buf << _color.grid_secondary << "  ";}
//       }
//       else {
//         if ((y + pos.y) % 2) {_buf << _color.grid_secondary << "  ";}
//         else {_buf << _color.grid_primary << "  ";}
//       }
//     }
//   }
//   _buf << aec::clear;
// }

// void World::draw_score() {
//   _buf << win_cursor({2, _win_height - 1});
//   _buf << _color.score_text << "SCORE: " << _color.score_value << _score << aec::clear;
// }

// void World::update_score() {
//   _buf << win_cursor({9, _win_height - 1});
//   _buf << _color.score_value << _score << aec::clear;
// }

// void World::win_size() {
//   Term::size(_win_width, _win_height);
//   _readline.size(_win_width, _win_height);
//   _grid_width = _win_width - 4;
//   if (_grid_width % 2 != 0) {_grid_width -= 1;}
//   _grid_width /= 2;
//   _grid_height = _win_height - 4;
// }

// std::string World::win_clear() {
//   return aec::screen_clear;
// }

// std::string World::win_cursor(Point const& obj) {
//   return aec::cursor_set(1 + obj.x, _win_height - obj.y);
// }

// std::string World::grid_cursor(Point const& obj) {
//   return aec::cursor_set(3 + (obj.x * 2), _win_height - 2 - obj.y);
// }

// void World::game_redraw() {
//   buf_clear();
//   _buf << win_clear();

//   draw_grid({0, 0}, _grid_width, _grid_height);

//   _buf << grid_cursor(_snake.pos.front());
//   _buf << _snake.color.head << _snake.sprite << aec::clear;
//   for (std::size_t i = 1; i < _snake.pos.size(); ++i) {
//     if (_snake.pos.at(i).x < 0 || _snake.pos.at(i).y < 0) {break;}
//     _buf << grid_cursor(_snake.pos.at(i));
//     _buf << _snake.color.body.at(_snake.color.idx) << _snake.sprite << aec::clear;
//     if (++_snake.color.idx >= _snake.color.body.size()) {_snake.color.idx = 0;}
//   }

//   _buf << grid_cursor(_egg.pos);
//   _buf << _egg.color << _egg.sprite << aec::clear;

//   Rect r1;
//   r1.xy_max((_grid_width * 2) + 3, _grid_height + 3);
//   r1.xy(1, 1);
//   r1.wh((_grid_width * 2) + 2, _grid_height + 2);
//   r1.border(1, 1, 1, 1);
//   r1.border_fg(_color.border);
//   _buf << r1 << aec::clear;

//   draw_score();

//   buf_print();
//   buf_clear();
// }

// void World::game_init() {
//   _state = State::menu;
//   _tick = {_tick_min, _tick_max, _tick_default};
//   _egg = {};
//   _snake = {};
//   _score = 0;
//   _state = State::game;
//   _timer.expires_at((std::chrono::high_resolution_clock::time_point::min)());
//   _timer.cancel();

//   buf_clear();
//   _buf << win_clear();

//   _snake.dir.clear();
//   _snake.dir_prev = Snake::up;
//   _snake.ext = 2;
//   _snake.pos.emplace_back((_grid_width - 1) / 2, 0);

//   _egg.pos = {(_grid_width - 1) / 2, (_grid_height - 1) / 2};

//   game_redraw();
// }

// void World::game_play() {
//   // determine next snake head position
//   Point head {_snake.pos.front()};
//   Snake::Dir dir;
//   if (_snake.dir.size()) {
//     dir = _snake.dir.front();
//     _snake.dir.pop_front();
//   }
//   else {
//     dir = _snake.dir_prev;
//   }
//   switch (_snake.dir_prev) {
//     case Snake::up: {
//       if (dir == Snake::down) {dir = Snake::up;}
//       switch (dir) {
//         case Snake::up: {head.y += 1; break;}
//         case Snake::left: {head.x -= 1; break;}
//         case Snake::right: {head.x += 1; break;}
//         break;
//       }
//       break;
//     }
//     case Snake::down: {
//       if (dir == Snake::up) {dir = Snake::down;}
//       switch (dir) {
//         case Snake::down: {head.y -= 1; break;}
//         case Snake::left: {head.x -= 1; break;}
//         case Snake::right: {head.x += 1; break;}
//         break;
//       }
//       break;
//     }
//     case Snake::left: {
//       if (dir == Snake::right) {dir = Snake::left;}
//       switch (dir) {
//         case Snake::left: {head.x -= 1; break;}
//         case Snake::up: {head.y += 1; break;}
//         case Snake::down: {head.y -= 1; break;}
//         break;
//       }
//       break;
//     }
//     case Snake::right: {
//       if (dir == Snake::left) {dir = Snake::right;}
//       switch (dir) {
//         case Snake::right: {head.x += 1; break;}
//         case Snake::up: {head.y += 1; break;}
//         case Snake::down: {head.y -= 1; break;}
//         break;
//       }
//       break;
//     }
//   }

//   buf_clear();

//   // check if snake has collided with a wall
//   bool hit_wall {false};
//   if (head.x < 0 || head.x > _grid_width - 1 ||
//       head.y < 0 || head.y > _grid_height - 1) {
//     hit_wall = true;
//   }
//   if (hit_wall) {
//     draw_grid(_egg.pos, 1, 1);
//     buf_print();
//     _state = State::end;
//     _tick.norm().val(_tick.min());
//     do_timer();
//     return;
//   }

//   // check if snake has collided with itself
//   bool hit_snake {false};
//   for (auto const& [x, y] : _snake.pos) {
//     if (head.x == x && head.y == y) {
//       hit_snake = true;
//       break;
//     }
//   }
//   if (hit_snake) {
//     draw_grid(_egg.pos, 1, 1);
//     buf_print();
//     _state = State::end;
//     _tick.norm().val(_tick.min());
//     do_timer();
//     return;
//   }

//   // slow down tick rate if about to collide into wall
//   if ((head.x == 0 && dir == Snake::left) ||
//       (head.x == _grid_width - 1 && dir == Snake::right) ||
//       (head.y == 0 && dir == Snake::up) ||
//       (head.y == _grid_height - 1 && dir == Snake::down)) {
//     _tick.slow();
//   }
//   else {_tick.norm();}

//   // snake has hit egg, update and draw egg
//   bool new_egg {false};
//   if (head.x == _egg.pos.x && head.y == _egg.pos.y) {
//     new_egg = true;
//     _tick.dec(3ms);
//     _score += 10;
//     update_score();
//   }

//   _snake.dir_prev = dir;
//   _snake.pos.emplace_front(head);

//   _buf << grid_cursor(_snake.pos.front());
//   _buf << _snake.color.head << _snake.sprite << aec::clear;

//   _buf << grid_cursor(_snake.pos.at(1));
//   _buf << _snake.color.body.at(_snake.color.idx) << _snake.sprite << aec::clear;
//   if (++_snake.color.idx >= _snake.color.body.size()) {_snake.color.idx = 0;}

//   if (_snake.ext) {_snake.ext -= 1;}
//   else {
//     draw_grid(_snake.pos.back(), 1, 1);
//     _snake.pos.pop_back();
//   }

//   if (new_egg) {
//     _snake.ext += 3;
//     spawn_egg();
//     _buf << grid_cursor(_egg.pos);
//     _buf << _egg.color << _egg.sprite << aec::clear;
//   }

//   buf_print();

//   do_timer();
// }

// void World::game_over() {
//   if (_snake.pos.empty()) {
//     _state = State::end;
//     buf_clear();

//   // {
//   //   Point pos {1, (_grid_height / 2) - (_grid_height / 4)};
//   //   Size size {_grid_width - 2, _grid_height / 2};
//   //   _buf << aec::bg_true("#cb4b16");
//   //   draw_border(pos, size,
//   //     [&](auto const& p) {},
//   //     [&](auto const& p) {_buf << grid_cursor(p);});
//   //   _buf << aec::clear;
//   // }

//     // {
//     //   Size size {12, 5};
//     //   Point pos {(_grid_width / 2) - (size.w / 2), (_grid_height / 2) - (size.w / 2)};
//     //   // _buf << aec::bg_true("#1b1e24");
//     //   draw_rect(pos, size,
//     //       [&](auto const& p) {_buf << "  ";},
//     //       [&](auto const& p) {_buf << grid_cursor(p);});
//     //   // _buf << aec::clear;
//     // }

//     Rect r;
//     r.xy_max(_win_width + 1, _win_height + 1);
//     r.xy(0, (_win_height / 2) - 1);
//     r.wh(_win_width, 3);
//     r.color_fg("cyan"s);
//     // r.color_bg("#1b1e24"s);
//     r.text("GAME OVER\n\nPress [R] to Restart");
//     r.align(Rect::Align::center, Rect::Align::center);
//     _buf << r;
//     buf_print();
//     return;
//   }

//   buf_clear();

//   draw_grid(_snake.pos.back(), 1, 1);
//   _snake.pos.pop_back();

//   buf_print();

//   do_timer();
// }

// void World::spawn_egg() {
//   do {
//     _egg.pos = {random_range(0, _grid_width - 1), random_range(0, _grid_height - 1)};
//   } while ([&]() {
//     for (auto const& [x, y] : _snake.pos) {
//       if (_egg.pos.x == x && _egg.pos.y == y) {return true;}
//     }
//     return false;
//   }());
// }

// void World::game_menu() {
// }

// void World::input_prompt(Belle::IO::Read::Ctx const& ctx) {
//   auto const pkey = std::get_if<Key>(&ctx);
//   if (!pkey) {return;}
//   OB::Text::Char32 key {pkey->ch, pkey->str};
//   buf_clear();
//   _buf << win_cursor({0, 0}) << aec::erase_line;
//   if (! _readline(key)) {
//     _buf << _readline.render();
//     buf_print();
//     return;
//   }
//   else {
//     auto input = _readline.get();
//     if (input.size()) {
//       try {
//         if (auto x = read(input)) {
//           auto v = eval(*x, _env);
//           auto const res = print(v);
//           Text::View vres {res};
//           _buf << win_cursor({0, 0}) << aec::erase_line << aec::fg_green_bright << ">" << aec::clear << vres.colstr(0, _win_width - 1);
//         }
//       }
//       catch (std::exception const& e) {
//         auto const res = print(*read("(err \""s + e.what() + "\")"s));
//         Text::View vres {res};
//         _buf << win_cursor({0, 0}) << aec::erase_line << aec::fg_red << ">" << aec::clear << vres.colstr(0, _win_width - 1);
//       }
//       // deinit prompt
//       _buf << aec::cursor_hide;
//       _state = _state_stk.top();
//       _state_stk.pop();
//     }
//     else {
//       // deinit prompt
//       _buf << aec::cursor_hide << aec::erase_line;
//       _state = _state_stk.top();
//       _state_stk.pop();
//     }
//   }
//   buf_print();
// }

// void World::input_play(Belle::IO::Read::Ctx const& ctx) {
//   auto const pkey = std::get_if<Key>(&ctx);
//   if (!pkey) {return;}
//   if (auto const v = _input_key.find(pkey->ch); v != _input_key.end()) {
//     eval(v->second, _env);
//   }
// }

// void World::input() {
//   _readline.hist_load("./history.txt"); // debug
//   _readline.style("");
//   _readline.prompt(">", aec::fg_magenta_bright);
//   _readline.refresh();
//   _readline.boundaries(" `',@()[]{}");
//   _readline.autocomplete([&]() {
//     std::vector<std::string> values;
//     for (auto const& [k, v] : _env->inner) {
//       values.emplace_back(k);
//     }
//     return values;
//   });

//   _input_key['q'] = *read("(quit)");
//   _input_key[Term::ctrl_key('c')] = *read("(sigint)");
//   _input_key[Term::ctrl_key('l')] = *read("(refresh)");
//   _input_key[':'] = *read("(prompt)");
//   _input_key['r'] = *read("(restart)");
//   _input_key['z'] = *read("(coil)");
//   _input_key['?'] = *read("(help)");
//   _input_key[Key::Space] = *read("(pause)");
//   _input_key[','] = *read("(left2)");
//   _input_key['.'] = *read("(right2)");
//   _input_key[Key::Up] = *read("(up)");
//   _input_key['w'] = *read("(up)");
//   _input_key['k'] = *read("(up)");
//   _input_key[Key::Down] = *read("(down)");
//   _input_key['s'] = *read("(down)");
//   _input_key['j'] = *read("(down)");
//   _input_key[Key::Left] = *read("(left)");
//   _input_key['a'] = *read("(left)");
//   _input_key['h'] = *read("(left)");
//   _input_key[Key::Right] = *read("(right)");
//   _input_key['d'] = *read("(right)");
//   _input_key['l'] = *read("(right)");

//   _read.on_read([&](auto const& ctx) {
//     switch (_state) {
//       case State::prompt: {
//         input_prompt(ctx);
//         break;
//       }
//       case State::menu: {
//         input_play(ctx);
//         break;
//       }
//       case State::start: {
//         input_play(ctx);
//         break;
//       }
//       case State::game: {
//         input_play(ctx);
//         break;
//       }
//       case State::end: {
//         input_play(ctx);
//         break;
//       }
//     }
//   });

//   _read.run();
// }

// void World::lang() {
//   env_init(_env, 0, nullptr);

//   (*_env)["snake-len?"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
//     return num_xpr(static_cast<Int>(_snake.pos.size()));
//   }}, _env, Val::evaled};

//   (*_env)["snake-ext?"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
//     return num_xpr(static_cast<Int>(_snake.ext));
//   }}, _env, Val::evaled};

//   (*_env)["snake-ext"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
//     auto x = eval(sym_xpr("a"), e);
//     if (auto const v = xpr_int(&x)) {
//       _snake.ext += static_cast<std::size_t>(*v);
//       return x;
//     }
//     throw std::runtime_error("expected number");
//   }}, _env, Val::evaled};

//   (*_env)["tick"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
//     auto x = eval(sym_xpr("a"), e);
//     if (auto const v = xpr_int(&x)) {
//       _tick.val(static_cast<std::chrono::milliseconds>(static_cast<std::size_t>(*v)));
//       return x;
//     }
//     throw std::runtime_error("expected number");
//   }}, _env, Val::evaled};

//   (*_env)["egg-pos"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
//     return Xpr{Lst{num_xpr(static_cast<Int>(_egg.pos.x)), num_xpr(static_cast<Int>(_egg.pos.y))}};
//   }}, _env, Val::evaled};

//   (*_env)["key"] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
//     auto a = eval(sym_xpr("a"), e);
//     if (auto const s = xpr_str(&a)) {
//       if (s->size() == 1) {
//         auto key = OB::Term::utf8_to_char32(s->front());
//         _input_key[key] = eval(sym_xpr("b"), e);
//         return a;
//       }
//       else if (s->size() > 1) {
//         if (auto const p = Belle::IO::Read::Key::map.find(s->str()); p != Belle::IO::Read::Key::map.end()) {
//           _input_key[p->second] = eval(sym_xpr("b"), e);
//           return a;
//         }
//       }
//       throw std::runtime_error("invalid key '" + s->str() + "'");
//     }
//     throw std::runtime_error("invalid type '" + typ_str.at(type(a)) + "'");
//   }}, _env, Val::evaled};

//   (*_env)["quit"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
//     _io.stop();
//     return sym_xpr("T");
//   }}, _env, Val::evaled};

//   (*_env)["sigint"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
//     std::cout << aec::erase_down << std::flush;
//     kill(getpid(), SIGINT);
//     return sym_xpr("T");
//   }}, _env, Val::evaled};

//   (*_env)["refresh"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
//     game_redraw();
//     return sym_xpr("T");
//   }}, _env, Val::evaled};

//   (*_env)["prompt"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
//     // pause
//     _timer.expires_at((std::chrono::high_resolution_clock::time_point::min)());
//     _timer.cancel();
//     // init prompt
//     _state_stk.push(_state);
//     _state = State::prompt;
//     buf_clear();
//     _buf << win_cursor({0, 0});
//     _buf << _readline.clear().refresh().render();
//     buf_print();
//     return sym_xpr("T");
//   }}, _env, Val::evaled};

//   (*_env)["restart"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
//     _timer.expires_at((std::chrono::high_resolution_clock::time_point::min)());
//     _timer.cancel();
//     game_init();
//     return sym_xpr("T");
//   }}, _env, Val::evaled};

//   (*_env)["coil"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
//     _snake.ext += _snake.pos.size() - 1;
//     buf_clear();
//     while (_snake.pos.size() > 1) {
//       draw_grid(_snake.pos.back(), 1, 1);
//       _snake.pos.pop_back();
//     }
//     buf_print();
//     return sym_xpr("T");
//   }}, _env, Val::evaled};

//   (*_env)["help"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
//     _timer.expires_at((std::chrono::high_resolution_clock::time_point::min)());
//     _timer.cancel();
//     std::cout << aec::mouse_disable << aec::nl << aec::screen_pop << aec::cursor_show << std::flush;
//     _term_mode.set_cooked();
//     std::system(("$(which less) -ir <<'EOF'\n" + _pg.help() + "EOF").c_str());
//     _term_mode.set_raw();
//     std::cout << aec::cursor_hide << aec::screen_push << aec::cursor_hide << aec::mouse_enable << aec::screen_clear << aec::cursor_home << std::flush;
//     game_redraw();
//     return sym_xpr("T");
//   }}, _env, Val::evaled};

//   (*_env)["pause"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
//     if (_timer.expiry() == (std::chrono::high_resolution_clock::time_point::min)()) {
//       buf_clear();
//       _buf << win_cursor({0, 0});
//       _buf << aec::erase_line;
//       buf_print();
//       do_timer();
//     }
//     else {
//       _timer.expires_at((std::chrono::high_resolution_clock::time_point::min)());
//       _timer.cancel();
//     }
//     return sym_xpr("T");
//   }}, _env, Val::evaled};

//   (*_env)["left2"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
//     Snake::Dir dir;
//     if (_snake.dir.size()) {dir = _snake.dir.back();}
//     else {dir = _snake.dir_prev;}
//     switch (dir) {
//       case Snake::up: {
//         _snake.dir.emplace_back(Snake::left);
//         break;
//       }
//       case Snake::down: {
//         _snake.dir.emplace_back(Snake::right);
//         break;
//       }
//       case Snake::left: {
//         _snake.dir.emplace_back(Snake::down);
//         break;
//       }
//       case Snake::right: {
//         _snake.dir.emplace_back(Snake::up);
//         break;
//       }
//     }
//     return sym_xpr("T");
//   }}, _env, Val::evaled};

//   (*_env)["right2"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
//     Snake::Dir dir;
//     if (_snake.dir.size()) {dir = _snake.dir.back();}
//     else {dir = _snake.dir_prev;}
//     switch (dir) {
//       case Snake::up: {
//         _snake.dir.emplace_back(Snake::right);
//         break;
//       }
//       case Snake::down: {
//         _snake.dir.emplace_back(Snake::left);
//         break;
//       }
//       case Snake::left: {
//         _snake.dir.emplace_back(Snake::up);
//         break;
//       }
//       case Snake::right: {
//         _snake.dir.emplace_back(Snake::down);
//         break;
//       }
//     }
//     return sym_xpr("T");
//   }}, _env, Val::evaled};

//   (*_env)["up"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
//     if ((_snake.dir.empty() && _snake.dir_prev != Snake::up) || (_snake.dir.size() && _snake.dir.back() != Snake::up)) {
//       _snake.dir.emplace_back(Snake::up);
//     }
//     return sym_xpr("T");
//   }}, _env, Val::evaled};

//   (*_env)["down"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
//     if ((_snake.dir.empty() && _snake.dir_prev != Snake::down) || (_snake.dir.size() && _snake.dir.back() != Snake::down)) {
//       _snake.dir.emplace_back(Snake::down);
//     }
//     return sym_xpr("T");
//   }}, _env, Val::evaled};

//   (*_env)["left"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
//     if ((_snake.dir.empty() && _snake.dir_prev != Snake::left) || (_snake.dir.size() && _snake.dir.back() != Snake::left)) {
//       _snake.dir.emplace_back(Snake::left);
//     }
//     return sym_xpr("T");
//   }}, _env, Val::evaled};

//   (*_env)["right"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
//     if ((_snake.dir.empty() && _snake.dir_prev != Snake::right) || (_snake.dir.size() && _snake.dir.back() != Snake::right)) {
//       _snake.dir.emplace_back(Snake::right);
//     }
//     return sym_xpr("T");
//   }}, _env, Val::evaled};
// }

// void World::signals() {
//   _sig.on_signal({SIGINT, SIGTERM}, [&](auto const& ec, auto sig) {
//     // std::cerr << "\nEvent: " << Belle::Signal::str(sig) << "\n";
//     _io.stop();
//   });

//   _sig.on_signal(SIGWINCH, [&](auto const& ec, auto sig) {
//     win_size();
//     _sig.wait();
//   });

//   _sig.wait();
// }

// void World::do_timer() {
//   _time_render_end = std::chrono::high_resolution_clock::now();
//   _timer.expires_at(std::chrono::high_resolution_clock::now() - (_time_render_end - _time_render_begin) + _tick.val());
//   _timer.async_wait([&](auto ec) {
//     _time_render_begin = std::chrono::high_resolution_clock::now();
//     on_timer(ec);
//   });
// }

// void World::on_timer(Belle::error_code const& ec) {
//   if (ec) {
//     _time_render_begin = (std::chrono::high_resolution_clock::time_point::min)();
//     _time_render_end = (std::chrono::high_resolution_clock::time_point::min)();
//     return;
//   }
// }

// void World::run() {
//   signals();
//   win_size();
//   lang();
//   input();

//   std::cout
//   << aec::cursor_hide
//   << aec::screen_push
//   << aec::cursor_hide
//   << aec::mouse_enable
//   << aec::screen_clear
//   << aec::cursor_home
//   << std::flush;
//   _term_mode.set_raw();

//   // game_init();
//   _io.run();

//   std::cout
//   << aec::mouse_disable
//   << aec::nl
//   << aec::screen_pop
//   << aec::cursor_show
//   << std::flush;
//   _term_mode.set_cooked();
// }

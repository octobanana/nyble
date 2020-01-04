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
#include <algorithm>

namespace Nyble {

std::ostream& operator<<(std::ostream& os, Style const& obj) {
  os
  << static_cast<int>(obj.attr) << " "
  << static_cast<int>(obj.type) << " "
  << static_cast<int>(obj.fg.r) << "." << static_cast<int>(obj.fg.g) << "." << static_cast<int>(obj.fg.b) << " "
  << static_cast<int>(obj.bg.r) << "." << static_cast<int>(obj.bg.g) << "." << static_cast<int>(obj.bg.b);
  return os;
}

// Utils ----------------------------------------------------------------------------

std::size_t random_range(std::size_t l, std::size_t u) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<std::size_t> distr(l, u);
  return distr(gen);
}

std::string hex_encode(std::string_view const str) {
  char buf[3];
  std::string res;
  for (auto const& ch : str) {
    if (ch & 0x80) {
      std::snprintf(&buf[0], 3, "%02X", static_cast<unsigned int>(ch & 0xff));
    }
    else {
      std::snprintf(&buf[0], 3, "%02X", static_cast<unsigned int>(ch));
    }
    res += buf;
  }
  return res;
}


std::uint8_t hex_decode(std::string const& s) {
  unsigned int n;
  std::sscanf(s.data(), "%x", &n);
  return static_cast<std::uint8_t>(n);
}

Color hex_to_rgb(std::string const& str) {
  return Color(hex_decode(str.substr(0, 2)), hex_decode(str.substr(2, 2)), hex_decode(str.substr(4, 2)));
}

// Background -----------------------------------------------------------------------

Background::Background(Ctx ctx) : Scene(ctx) {
}

Background::~Background() {
}

void Background::on_winch(Size const& size) {
  _size = size;
}

bool Background::on_input(Read::Ctx const& ctx) {
  return false;
}

bool Background::on_update(Tick const delta) {
  return false;
}

bool Background::on_render(Buffer& buf) {
  buf.cursor(_pos);
  for (std::size_t h = 0; h < _size.h; ++h) {
    for (std::size_t w = 0; w < _size.w; ++w) {
      buf(_cell);
    }
  }
  return false;
}

// Root -----------------------------------------------------------------------

Root::Root(Ctx ctx) : Scene(ctx) {
  _scenes("background", std::make_shared<Background>(ctx));
  _scenes("status", std::make_shared<Status>(ctx));
  _scenes("prompt", std::make_shared<Prompt>(ctx));
  _scenes("board", std::make_shared<Board>(ctx));
  _scenes("snake", std::make_shared<Snake>(ctx));
  _scenes("egg", std::make_shared<Egg>(ctx));

  _focus = "snake";

  auto& _env = _ctx->_env;

  (*_env)["key"] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
    auto a = eval(sym_xpr("a"), e);
    if (auto const s = xpr_str(&a)) {
      if (s->size() == 1) {
        auto key = OB::Term::utf8_to_char32(s->front());
        _input[key] = eval(sym_xpr("b"), e);
        return a;
      }
      else if (s->size() > 1) {
        if (auto const p = Belle::IO::Read::Key::map.find(s->str()); p != Belle::IO::Read::Key::map.end()) {
          _input[p->second] = eval(sym_xpr("b"), e);
          return a;
        }
      }
      throw std::runtime_error("invalid key '" + s->str() + "'");
    }
    throw std::runtime_error("invalid type '" + typ_str.at(type(a)) + "'");
  }}, _env, Val::evaled};
}

Root::~Root() {
}

void Root::on_winch(Size const& size) {
  // TODO check min x,y window size
  // TODO allow fixed x,y window size
  _size = size;
  for (auto const& e : _scenes) {
    e->second->on_winch(_size);
  }
  _buf.size(_size);
  _scenes.at("background")->on_render(_buf);
  _scenes.at("board")->on_render(_buf);
}

bool Root::on_input(Read::Ctx const& ctx) {
  if (_focus != "prompt") {
    bool found {false};
    std::visit([&](auto const& e) {found = on_read(e);}, ctx);
    if (found) {return true;}
  }
  if (auto const v = _scenes.find(_focus); v != _scenes.map_end()) {
    v->second->on_input(ctx);
    return true;
  }
  return false;
}

bool Root::on_read(Read::Null const& ctx) {
  // TODO log error
  return true;
}

bool Root::on_read(Read::Mouse const& ctx) {
  if (auto const x = _input.find(ctx.ch); x != _input.end()) {
    eval(x->second, _ctx->_env);
    return true;
  }
  return false;
}

bool Root::on_read(Read::Key const& ctx) {
  {
    _code.erase(_code.begin());
    _code.emplace_back(std::make_pair(ctx.ch, _ctx->_time));
    if (_code.back().second - _code.front().second < 3000ms) {
      if (
          _code.at(0).first == Key::Up &&
          _code.at(1).first == Key::Up &&
          _code.at(2).first == Key::Down &&
          _code.at(3).first == Key::Down &&
          _code.at(4).first == Key::Left &&
          _code.at(5).first == Key::Right &&
          _code.at(6).first == Key::Left &&
          _code.at(7).first == Key::Right &&
          _code.at(8).first == 'b' &&
          _code.at(9).first == 'a'
          ) {
        auto const snake = std::dynamic_pointer_cast<Snake>(_scenes.at("snake"));
        snake->rainbow(true);
      }
    }
  }

  switch (ctx.ch) {
    case Key::Escape: {
      auto const prompt = std::dynamic_pointer_cast<Prompt>(_scenes.at("prompt"));
      prompt->_state = Prompt::State::Clear;
      return true;
    }
    case ':': case ';': {
      _focus = "prompt";
      auto const snake = std::dynamic_pointer_cast<Snake>(_scenes.at("snake"));
      snake->_state = Snake::State::Stopped;
      auto const prompt = std::dynamic_pointer_cast<Prompt>(_scenes.at("prompt"));
      prompt->_state = Prompt::State::Typing;
      return true;
    }
    case 'r': {
      _scenes.erase("snake");
      _scenes.erase("egg");
      _scenes("snake", std::make_shared<Snake>(_ctx));
      _scenes("egg", std::make_shared<Egg>(_ctx));
      on_winch(_size);
      return true;
    }
    case 'R': {
      _scenes.erase("board");
      _scenes.erase("snake");
      _scenes.erase("egg");
      _scenes("board", std::make_shared<Board>(_ctx));
      _scenes("snake", std::make_shared<Snake>(_ctx));
      _scenes("egg", std::make_shared<Egg>(_ctx));
      on_winch(_size);
      return true;
    }
  }
  if (auto const x = _input.find(ctx.ch); x != _input.end()) {
    eval(x->second, _ctx->_env);
    return true;
  }
  return false;
}

bool Root::on_update(Tick const delta) {
  for (auto const& e : _scenes) {
    if (e->second->on_update(delta)) {
      // std::cerr << "update> " << e->first << "\n";
    }
  }
  return true;
}

bool Root::on_render(Buffer& buf) {
  // for (auto const& e : _scenes) {
  //   if (e->second->on_render(buf)) {
  //     // std::cerr << "render> " << e->first << "\n";
  //   }
  // }

  buf = _buf;
  _scenes.at("status")->on_render(buf);
  _scenes.at("prompt")->on_render(buf);
  _scenes.at("snake")->on_render(buf);
  _scenes.at("egg")->on_render(buf);

  return true;
}

// Board -----------------------------------------------------------------------

Board::Board(Ctx ctx) : Scene(ctx) {
}

Board::~Board() {
}

void Board::on_winch(Size const& size) {
  if (_init) {
    _init = false;
    _size = Size(size.w, size.h - 2);
    if (_size.w % 2) {--_size.w;}
  }
  _pos = Pos((size.w / 2) - (_size.w / 2), (size.h / 2) - (_size.h / 2));
  if (_pos.x % 2) {--_pos.x;}
  if (_pos.y % 2) {++_pos.y;}
}

bool Board::on_input(Read::Ctx const& ctx) {
  return false;
}

bool Board::on_update(Tick const delta) {
  return false;
}

bool Board::on_render(Buffer& buf) {
  bool swap {false};
  for (std::size_t y = 0; y < _size.h; ++y) {
    if (y == 0) {
      // bottom
      for (std::size_t x = 0; x < _size.w; x += 2) {
        buf.cursor(Pos(_pos.x + x, _pos.y + y));
        if (x == 0) {
          // outer left
          buf(Cell{this, 0, _style, " └"});
        }
        else if (x == _size.w - 2) {
          // outer right
          buf(Cell{this, 0, _style, "┘ "});
        }
        else {
          // inner
          buf(Cell{this, 0, _style, "──"});
        }
      }
    }
    else if (y == _size.h - 1) {
      // top
      for (std::size_t x = 0; x < _size.w; x += 2) {
        buf.cursor(Pos(_pos.x + x, _pos.y + y));
        if (x == 0) {
          // outer left
          buf(Cell{this, 0, _style, " ┌"});
        }
        else if (x == _size.w - 2) {
          // outer right
          buf(Cell{this, 0, _style, "┐ "});
        }
        else {
          // inner
          buf(Cell{this, 0, _style, "──"});
        }
      }
    }
    else {
      // middle
      for (std::size_t x = 0; x < _size.w; x += 2) {
        buf.cursor(Pos(_pos.x + x, _pos.y + y));
        if (x == 0) {
          // outer left
          buf(Cell{this, 0, _style, " │"});
        }
        else if (x == _size.w - 2) {
          // outer right
          buf(Cell{this, 0, _style, "│ "});
        }
        else {
          // inner
          if (swap) {
            buf(Cell{this, 0, _block1, "  "});
          }
          else {
            buf(Cell{this, 0, _block2, "  "});
          }
          swap = !swap;
        }
      }
    }
    if ((_size.w / 2) % 2 == 0) {swap = !swap;}
  }
  return false;
}

// Snake -----------------------------------------------------------------------

Snake::Snake(Ctx ctx) : Scene(ctx) {
  auto const& _env = _ctx->_env;

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

  (*_env)["fixed"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
    if (_state == Moving) {
      _state = Fixed;
    }
    else if (_state == Fixed) {
      _state = Moving;
    }
    return sym_xpr("T");
  }}, _env, Val::evaled};

  (*_env)["straight"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
    if (_state == Stopped || _dir.size() >= 8) {return sym_xpr("F");}
    if (_dir.size()) {_dir.emplace_back(_dir.back());}
    else {_dir.emplace_back(_dir_prev);}
    return sym_xpr("T");
  }}, _env, Val::evaled};

  (*_env)["up"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
    if (_state == Stopped || _dir.size() >= 8) {return sym_xpr("F");}
    if (_state == Moving) {
      Dir dir;
      if (_dir.size()) {dir = _dir.back();}
      else {dir = _dir_prev;}
      if (dir == Snake::Up || dir == Snake::Down) {return sym_xpr("F");}
      _dir.emplace_back(Snake::Up);
      return sym_xpr("T");
    }
    if (_state == Fixed) {
      if (_dir.size() && _dir.back() == Snake::Up) {return sym_xpr("F");}
      _dir.emplace_back(Snake::Up);
      return sym_xpr("T");
    }
    return sym_xpr("F");
  }}, _env, Val::evaled};

  (*_env)["down"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
    if (_state == Stopped || _dir.size() >= 8) {return sym_xpr("F");}
    if (_state == Moving) {
      Dir dir;
      if (_dir.size()) {dir = _dir.back();}
      else {dir = _dir_prev;}
      if (dir == Snake::Down || dir == Snake::Up) {return sym_xpr("F");}
      _dir.emplace_back(Snake::Down);
      return sym_xpr("T");
    }
    if (_state == Fixed) {
      if (_dir.size() && _dir.back() == Snake::Down) {return sym_xpr("F");}
      _dir.emplace_back(Snake::Down);
      return sym_xpr("T");
    }
    return sym_xpr("F");
  }}, _env, Val::evaled};

  (*_env)["left"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
    if (_state == Stopped || _dir.size() >= 8) {return sym_xpr("F");}
    if (_state == Moving) {
      Dir dir;
      if (_dir.size()) {dir = _dir.back();}
      else {dir = _dir_prev;}
      if (dir == Snake::Left || dir == Snake::Right) {return sym_xpr("F");}
      _dir.emplace_back(Snake::Left);
      return sym_xpr("T");
    }
    if (_state == Fixed) {
      if (_dir.size() && _dir.back() == Snake::Left) {return sym_xpr("F");}
      _dir.emplace_back(Snake::Left);
      return sym_xpr("T");
    }
    return sym_xpr("F");
  }}, _env, Val::evaled};

  (*_env)["right"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
    if (_state == Stopped || _dir.size() >= 8) {return sym_xpr("F");}
    if (_state == Moving) {
      Dir dir;
      if (_dir.size()) {dir = _dir.back();}
      else {dir = _dir_prev;}
      if (dir == Snake::Right || dir == Snake::Left) {return sym_xpr("F");}
      _dir.emplace_back(Snake::Right);
      return sym_xpr("T");
    }
    if (_state == Fixed) {
      if (_dir.size() && _dir.back() == Snake::Right) {return sym_xpr("F");}
      _dir.emplace_back(Snake::Right);
      return sym_xpr("T");
    }
    return sym_xpr("F");
  }}, _env, Val::evaled};

  (*_env)["left2"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
    if (_state == Stopped || _dir.size() >= 8) {return sym_xpr("F");}
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
    if (_state == Stopped || _dir.size() >= 8) {return sym_xpr("F");}
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

  (*_env)["snake-coil"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
    if (_sprite.size() > 3) {
      _ext += _sprite.size() - 3;
      _sprite.erase(std::next(_sprite.begin(), 3), _sprite.end());
    }
    return sym_xpr("T");
  }}, _env, Val::evaled};

  (*_env)["snake-reverse"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
    _dir.clear();
    std::reverse(_sprite.begin(), _sprite.end());
    // TODO set direction after reversal
    return sym_xpr("T");
  }}, _env, Val::evaled};

  (*_env)["snake-speed"] = Val{Fun{str_lst("(@)"), [&](auto e) -> Xpr {
    auto x = eval(sym_xpr("@"), e);
    auto& l = std::get<Lst>(x);
    if (l.size() == 0) {
      return num_xpr(static_cast<Int>(_interval.count()));
    }
    else if (l.size() == 1) {
      auto x = eval(l.front(), e->current);
      if (auto const v = xpr_int(&x)) {
        _delta = 0ms;
        _interval = static_cast<std::chrono::milliseconds>(static_cast<std::size_t>(*v));
        if (_interval < 100ms) {_interval = 100ms;}
        _special_interval = _interval / 4;
        return x;
      }
      throw std::runtime_error("expected number");
    }
    else {
      throw std::runtime_error("expected '0' or '1' arguments");
    }
  }}, _env, Val::evaled};

  (*_env)["snake-size"] = Val{Fun{str_lst("(@)"), [&](auto e) -> Xpr {
    auto x = eval(sym_xpr("@"), e);
    auto& l = std::get<Lst>(x);
    if (l.size() == 0) {
      return num_xpr(static_cast<Int>(_sprite.size() + _ext));
    }
    else if (l.size() == 1) {
      auto x = eval(l.front(), e->current);
      if (auto const v = xpr_int(&x)) {
        auto size = static_cast<std::size_t>(*v);
        if (size < 3) {size = 3;}
        if (size > _sprite.size()) {
          _ext += size - (_sprite.size() + _ext);
        }
        else {
          _ext = 0;
          _sprite.erase(std::next(_sprite.begin(), size), _sprite.end());
        }
        return x;
      }
      throw std::runtime_error("expected number");
    }
    else {
      throw std::runtime_error("expected '0' or '1' arguments");
    }
  }}, _env, Val::evaled};

  _input[Key::Space] = *read("(pause)");
  _input['1'] = *read("(snake-coil)");
  _input['2'] = *read("(snake-reverse)");
  _input['3'] = *read("(fixed)");
  _input[','] = *read("(left2)");
  _input['.'] = *read("(right2)");
  _input['<'] = *read("(pn (left2) (left2))");
  _input['>'] = *read("(pn (right2) (right2))");
  _input[Key::Up] = *read("(up)");
  _input['w'] = *read("(up)");
  _input['k'] = *read("(up)");
  _input[Key::Down] = *read("(down)");
  _input['s'] = *read("(down)");
  _input['j'] = *read("(down)");
  _input[Key::Left] = *read("(left)");
  _input['a'] = *read("(left)");
  _input['h'] = *read("(left)");
  _input[Key::Right] = *read("(right)");
  _input['d'] = *read("(right)");
  _input['l'] = *read("(right)");
}

Snake::~Snake() {
}

void Snake::on_winch(Size const& size) {
  if (_init) {
    _init = false;
    auto const& board = std::dynamic_pointer_cast<Root>(_ctx->_root)->_scenes.at("board");
    auto x = (board->_size.w - 5) / 2;
    if (x % 2) {x -= 1;}
    _sprite.emplace_front(Block{Pos{x, 0}, &_style.head, _text.head.at(Dir::Up)});
  }
  _size = size;
}

bool Snake::on_input(Read::Ctx const& ctx) {
  if (auto const v = std::get_if<Key>(&ctx)) {
    if (auto const x = _input.find(v->ch); x != _input.end()) {
      eval(x->second, _ctx->_env);
      return true;
    }
  }
  // else if (auto const v = std::get_if<Mouse>(&ctx)) {
  //   // std::cerr << "mouse> snake " << v->pos.x << ":" << v->pos.y << "\n";
  //   // scene  pos  size
  //   // main   0:0  42:26
  //   // board  0:2  42:24
  //   // grid   2:1  38:22
  //   // head  18:0   2:1
  // auto const& board = std::dynamic_pointer_cast<Root>(_ctx->_root)->_scenes.at("board");
  //   auto const x = v->pos.x - board->_pos.x - 2;
  //   auto const y = v->pos.y - board->_pos.y - 1;
  //   if (v->ch == Mouse::Press_left) {
  //     if ((x == _sprite.front().pos.x || x == _sprite.front().pos.x + 1) && y == _sprite.front().pos.y) {
  //       std::cerr << "mouse> snake head " << x << ":" << y << "\n";
  //     }
  //     else {
  //       std::cerr << "mouse> snake body " << x << ":" << y << "\n";
  //     }
  //   }
  //   if (auto const x = _input.find(v->ch); x != _input.end()) {
  //     eval(x->second, _ctx->_env);
  //     return true;
  //   }
  // }
  return false;
}

bool Snake::on_update(Tick const delta) {
  // special animation
  if (_special != Special::Normal) {
    if (_ctx->_time - _special_time > 20000ms) {
      rainbow(false);
    }
    else if (_ctx->_time - _special_time > 16000ms) {
      _special_delta += delta;
      while (_special_delta >= _special_interval) {
        _special_delta -= _special_interval;
        double const step {-100.0 / _sprite.size()};
        _color.step(step);
        _special ^= Flicker;
      }
    }
    else {
      _special_delta += delta;
      while (_special_delta >= _special_interval) {
        _special_delta -= _special_interval;
        double const step {-100.0 / _sprite.size()};
        _color.step(step);
      }
    }
  }

  // blink animation
  {
    _blink_delta += delta;
    if (_blink_delta >= _blink_interval) {
      while (_blink_delta >= _blink_interval) {
        _blink_delta -= _blink_interval;
        if (++_state_eyes_idx >= _state_eyes.size()) {
          _state_eyes_idx = 0;
          _state_eyes.front().second = static_cast<Tick>(random_range(4, 8) * 1000000000);
        }
        _blink_interval = _state_eyes.at(_state_eyes_idx).second;
      }
    }
  }

  switch (_state) {
    case Stopped: {
      state_stopped();
      return false;
    }
    case Moving: case Fixed: {
      _delta += delta;
      if (_delta >= _interval) {
        while (_delta >= _interval) {
          _delta -= _interval;
          state_moving();
        }
      }
      return true;
    }
  }

  return false;
}

bool Snake::on_render(Buffer& buf) {
  auto const& board = std::dynamic_pointer_cast<Root>(_ctx->_root)->_scenes.at("board");

  if (_special != Special::Normal) {
    if (_special & Rainbow) {
      if (_special & Flicker) {
        for (auto it = _sprite.rbegin(); it != _sprite.rend(); ++it) {
          buf.cursor(Pos(it->pos.x + board->_pos.x + 2, it->pos.y + board->_pos.y + 1));
          buf(Cell{this, 0, *it->style, std::string(it->value)});
        }
      }
      else {
        double const step {-100.0 / _sprite.size()};
        for (auto it = _sprite.rbegin(); it != _sprite.rend(); ++it) {
          buf.cursor(Pos(it->pos.x + board->_pos.x + 2, it->pos.y + board->_pos.y + 1));
          auto const rgb = _color.rgb();
          buf(Cell{this, 0, _style.head.type, _style.head.attr, _style.head.fg, Color(static_cast<std::uint8_t>(rgb.r), static_cast<std::uint8_t>(rgb.g), static_cast<std::uint8_t>(rgb.b)), std::string(it->value)});
          _color.step(step);
        }
      }
    }
  }
  else {
    for (auto it = _sprite.rbegin(); it != _sprite.rend(); ++it) {
      buf.cursor(Pos(it->pos.x + board->_pos.x + 2, it->pos.y + board->_pos.y + 1));
      buf(Cell{this, 0, *it->style, std::string(it->value)});
    }
  }

  auto& head = _sprite.front();

  // apply blink state
  _state_eyes.at(_state_eyes_idx).first(buf.col(Pos(head.pos.x + board->_pos.x + 2, head.pos.y + board->_pos.y + 1)));
  _state_eyes.at(_state_eyes_idx).first(buf.col(Pos(head.pos.x + board->_pos.x + 3, head.pos.y + board->_pos.y + 1)));

  buf.col(Pos(head.pos.x + board->_pos.x + 2, board->_pos.y + board->_size.h - 1)).style.fg = head.style->bg;
  buf.col(Pos(head.pos.x + board->_pos.x + 3, board->_pos.y + board->_size.h - 1)).style.fg = head.style->bg;
  buf.col(Pos(head.pos.x + board->_pos.x + 2, board->_pos.y)).style.fg = head.style->bg;
  buf.col(Pos(head.pos.x + board->_pos.x + 3, board->_pos.y)).style.fg = head.style->bg;

  buf.col(Pos(board->_pos.x + 1, head.pos.y + board->_pos.y + 1)).style.fg = head.style->bg;
  buf.col(Pos(board->_pos.x + board->_size.w - 2, head.pos.y + board->_pos.y + 1)).style.fg = head.style->bg;

  return false;
}

void Snake::state_stopped() {
}

void Snake::state_moving() {
  Pos head;
  Dir dir;

  if (_state == Fixed) {
    head = _sprite.front().pos;
    if (_dir.empty()) {return;}
    dir = _dir.front();
    _dir.pop_front();
    switch (dir) {
      case Up: {
        if (dir == Down) {return;}
        switch (dir) {
          case Up: {head.y += 1; break;}
          case Left: {head.x -= 2; break;}
          case Right: {head.x += 2; break;}
        }
        break;
      }
      case Down: {
        if (dir == Up) {return;}
        switch (dir) {
          case Down: {head.y -= 1; break;}
          case Left: {head.x -= 2; break;}
          case Right: {head.x += 2; break;}
        }
        break;
      }
      case Left: {
        if (dir == Right) {return;}
        switch (dir) {
          case Left: {head.x -= 2; break;}
          case Up: {head.y += 1; break;}
          case Down: {head.y -= 1; break;}
        }
        break;
      }
      case Right: {
        if (dir == Left) {return;}
        switch (dir) {
          case Right: {head.x += 2; break;}
          case Up: {head.y += 1; break;}
          case Down: {head.y -= 1; break;}
        }
        break;
      }
    }
  }
  else {
    head = _sprite.front().pos;
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
          case Left: {head.x -= 2; break;}
          case Right: {head.x += 2; break;}
        }
        break;
      }
      case Down: {
        if (dir == Up) {dir = Down;}
        switch (dir) {
          case Down: {head.y -= 1; break;}
          case Left: {head.x -= 2; break;}
          case Right: {head.x += 2; break;}
        }
        break;
      }
      case Left: {
        if (dir == Right) {dir = Left;}
        switch (dir) {
          case Left: {head.x -= 2; break;}
          case Up: {head.y += 1; break;}
          case Down: {head.y -= 1; break;}
        }
        break;
      }
      case Right: {
        if (dir == Left) {dir = Right;}
        switch (dir) {
          case Right: {head.x += 2; break;}
          case Up: {head.y += 1; break;}
          case Down: {head.y -= 1; break;}
        }
        break;
      }
    }
  }

  // hit wall
  if (_hit_wall) {
    bool hit_wall {false};
    auto const& board = std::dynamic_pointer_cast<Root>(_ctx->_root)->_scenes.at("board");
    if (head.x < 0 || head.x >= board->_size.w - 4 ||
        head.y < 0 || head.y >= board->_size.h - 2) {
      hit_wall = true;
    }
    if (hit_wall) {
      // std::cerr << "collide> " << "snake -> wall" << "\n";
      _dir.clear();
      return;
    }
  }
  else if (_hit_wall_portal) {
    auto const& board = std::dynamic_pointer_cast<Root>(_ctx->_root)->_scenes.at("board");
    if (static_cast<long>(head.x) < 0) {
      head.x = board->_size.w - 5;
      if (head.x % 2) {head.x -= 1;}
    }
    else if (head.x >= board->_size.w - 4) {
      head.x = 0;
    }
    else if (static_cast<long>(head.y) < 0) {
      head.y = board->_size.h - 3;
    }
    else if (head.y >= board->_size.h - 2) {
      head.y = 0;
    }
  }
  else if (_hit_wall_egg) {
    bool hit_wall {false};
    auto const& egg = std::dynamic_pointer_cast<Root>(_ctx->_root)->_scenes.at("egg");
    auto const& board = std::dynamic_pointer_cast<Root>(_ctx->_root)->_scenes.at("board");
    if (static_cast<long>(head.x) < 0) {
      if (head.y == egg->_pos.y) {
        head.x = board->_size.w - 5;
        if (head.x % 2) {head.x -= 1;}
      }
      else {
        hit_wall = true;
      }
    }
    else if (head.x >= board->_size.w - 4) {
      if (head.y == egg->_pos.y) {
        head.x = 0;
      }
      else {
        hit_wall = true;
      }
    }
    else if (static_cast<long>(head.y) < 0) {
      if (head.x == egg->_pos.x) {
        head.y = board->_size.h - 3;
      }
      else {
        hit_wall = true;
      }
    }
    else if (head.y >= board->_size.h - 2) {
      if (head.x == egg->_pos.x) {
        head.y = 0;
      }
      else {
        hit_wall = true;
      }
    }
    if (hit_wall) {
      // std::cerr << "collide> " << "snake -> wall" << "\n";
      _dir.clear();
      return;
    }
  }

  // hit snake
  bool hit_snake {false};
  if (_hit_body) {
    for (auto const& cell : _sprite) {
      if (head.x == cell.pos.x && head.y == cell.pos.y) {
        hit_snake = true;
        break;
      }
    }
    if (hit_snake) {
      // std::cerr << "collide> " << "snake -> snake" << "\n";
      _dir.clear();
      return;
    }
  }

  // std::size_t len_body {0};
  // for (auto it_body = _sprite.begin(); it_body != _sprite.end(); ++it_body) {
  //   ++len_body;
  //   if (head.x == it_body->pos.x && head.y == it_body->pos.y) {
  //     hit_snake = true;
  //     _ext += _sprite.size() - len_body;
  //     _sprite.erase(it_body, _sprite.end());
  //     break;
  //   }
  // }

  // hit egg
  bool hit_egg {false};
  auto& egg = *std::dynamic_pointer_cast<Egg>(std::dynamic_pointer_cast<Root>(_ctx->_root)->_scenes.at("egg"));
  if (head.x == egg._pos.x && head.y == egg._pos.y) {
    hit_egg = true;
  }

  if (! hit_snake) {
    if (_ext) {
      _ext -= 1;
    }
    else {
      _sprite.pop_back();
    }
  }

  _dir_prev = dir;
  _sprite.emplace_front(Block{head, &_style.head, _text.head.at(dir)});
  _sprite.at(1).style = &_style.body.at(_style.idx);
  _sprite.at(1).value = _text.body;
  if (++_style.idx >= _style.body.size()) {_style.idx = 0;}

  if (hit_egg) {
    // std::cerr << "collide> " << "snake -> egg" << "\n";
    if (egg._style.type == Egg::Styles::Rainbow) {
      rainbow(true);
    }
    egg.spawn();
    _ext += 2;
    _interval -= 4ms;
    if (_interval < 100ms) {_interval = 100ms;}
    _special_interval = _interval / 4;
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

void Snake::rainbow(bool const val) {
  if (val) {
    _special_time = _ctx->_time;
    _special = Snake::Rainbow;
    _hit_wall = false;
    _hit_wall_egg = false;
    _hit_wall_portal = true;
    _hit_body = false;
  }
  else {
    _special = Snake::Normal;
    _hit_wall = false;
    _hit_wall_egg = true;
    _hit_wall_portal = false;
    _hit_body = true;
  }
}

// Egg -----------------------------------------------------------------------

Egg::Egg(Ctx ctx) : Scene(ctx) {
  _interval = std::chrono::duration_cast<Tick>((_duration / 2) / _style.max);
  _style.color = _style.color_map.at(_style.type);
  _style.color.lum(30);
  auto const rgb = _style.color.rgb();
  _style.style.bg = Color(rgb.r, rgb.g, rgb.b);
}

Egg::~Egg() {
}

void Egg::on_winch(Size const& size) {
  if (_init) {
    _init = false;
    auto const& board = *std::dynamic_pointer_cast<Root>(_ctx->_root)->_scenes.at("board");
    auto x = (board._size.w - 5) / 2;
    if (x % 2) {x -= 1;}
    _pos = Pos(x, (board._size.h - 2) / 2);
  }
}

bool Egg::on_input(Read::Ctx const& ctx) {
  return false;
}

bool Egg::on_update(Tick const delta) {
  _delta += delta;
  if (_delta >= _interval) {
    while (_delta >= _interval) {
      _delta -= _interval;
      if (_style.type == Styles::Rainbow) {
        _style.color.step(1);
      }
      else {
        if (_style.dir == Styles::Lighten) {
          if (++_style.idx == _style.max) {
            _style.dir = Styles::Darken;
          }
          _style.color.lum(_style.color.lum() + 1);
        }
        else {
          if (--_style.idx == 0) {
            _style.dir = Styles::Lighten;
          }
          _style.color.lum(_style.color.lum() - 1);
        }
      }
      auto const rgb = _style.color.rgb();
      _style.style.bg = Color(rgb.r, rgb.g, rgb.b);
    }
    return true;
  }
  return false;
}

bool Egg::on_render(Buffer& buf) {
  auto const& board = std::dynamic_pointer_cast<Root>(_ctx->_root)->_scenes.at("board");

  // egg
  buf.cursor(Pos(_pos.x + board->_pos.x + 2, _pos.y + board->_pos.y + 1));
  buf(Cell{this, 0, _style.style, _text});

  // border x-axis
  buf.col(Pos(_pos.x + board->_pos.x + 2, board->_pos.y + board->_size.h - 1)).style.fg = _style.style.bg;
  buf.col(Pos(_pos.x + board->_pos.x + 3, board->_pos.y + board->_size.h - 1)).style.fg = _style.style.bg;
  buf.col(Pos(_pos.x + board->_pos.x + 2, board->_pos.y)).style.fg = _style.style.bg;
  buf.col(Pos(_pos.x + board->_pos.x + 3, board->_pos.y)).style.fg = _style.style.bg;

  // border y-axis
  buf.col(Pos(board->_pos.x + 1, _pos.y + board->_pos.y + 1)).style.fg = _style.style.bg;
  buf.col(Pos(board->_pos.x + board->_size.w - 2, _pos.y + board->_pos.y + 1)).style.fg = _style.style.bg;

  return true;
}

void Egg::spawn() {
  // TODO handle when egg can't spawn anywhere
  auto const& board = *std::dynamic_pointer_cast<Board>(std::dynamic_pointer_cast<Root>(_ctx->_root)->_scenes.at("board"));
  auto& snake = *std::dynamic_pointer_cast<Snake>(std::dynamic_pointer_cast<Root>(_ctx->_root)->_scenes.at("snake"));
  auto const is_valid = [&]() {
    for (auto const& e : snake._sprite) {
      if (_pos.x == e.pos.x && _pos.y == e.pos.y) {return false;}
    }
    return true;
  };
  do {
    _pos = {random_range(0, board._size.w - 5), random_range(0, board._size.h - 3)};
    if (_pos.x % 2) {_pos.x -= 1;}
  }
  while (!is_valid());

  if (++count % 10 == 0 || random_range(0, 100) < 8) {
    _style.type = Styles::Rainbow;
    _delta = 0ms;
    _style.idx = 0;
    _style.dir = Styles::Lighten;
    _style.color = _style.color_map.at(_style.type);
  }
  else {
    _style.type = Styles::Normal;
    _delta = 0ms;
    _style.idx = 0;
    _style.dir = Styles::Lighten;
    _style.color = _style.color_map.at(_style.type);
    _style.color.lum(30);
  }

  auto const rgb = _style.color.rgb();
  _style.style.bg = Color(rgb.r, rgb.g, rgb.b);
}

// Prompt -----------------------------------------------------------------------

Prompt::Prompt(Ctx ctx) : Scene(ctx) {
  _pos = Pos(0, 0);
  auto const& _env = _ctx->_env;
  // _readline.hist_load("./history.txt"); // debug
  // _readline.style(_color.text);
  // _readline.prompt(">", _color.prompt);
  // _readline.prompt(">");
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

void Prompt::on_winch(Size const& size) {
  _size = size;
  if (_readline._mode == Readline::Mode::autocomplete_init || _readline._mode == Readline::Mode::autocomplete) {
    _readline.normal();
  }
  _readline.size(_size.w, _size.h);
  _readline.refresh();
}

bool Prompt::on_input(Read::Ctx const& ctx) {
  if (auto const v = std::get_if<Key>(&ctx)) {
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
            auto v = eval(*x, _ctx->_env);
            _buf = print(v);
            _status = true;
          }
        }
        catch (std::exception const& e) {
          _buf = print(*read("(err \""s + e.what() + "\")"s));
          _status = false;
        }
        _delta = 0ms;
        _state = Display;
        std::dynamic_pointer_cast<Root>(_ctx->_root)->_focus = "snake";
      }
      else {
        _delta = 0ms;
        _state = Clear;
        std::dynamic_pointer_cast<Root>(_ctx->_root)->_focus = "snake";
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
      _state = Clear;
      return true;
    }
  }
  return false;
}

bool Prompt::on_render(Buffer& buf) {
  switch (_state) {
    case Clear: {
      return false;
    }
    case Typing: {
      _readline.refresh();
      if (_readline._mode == Readline::Mode::autocomplete_init || _readline._mode == Readline::Mode::autocomplete) {
        buf.cursor(Pos(_pos.x, _pos.y + 1));
        for (std::size_t x = 0; x < _size.w; ++x) {
          buf(Cell{this, 0, _style.text, " "});
        }
        buf.cursor(Pos(_pos.x, _pos.y + 1));
        buf(Cell{this, 0, _style.prompt, _readline._autocomplete._lhs});
        buf(Cell{this, 0, _style.text, _readline._autocomplete._text});
        buf.cursor(Pos(_pos.x + _size.w - 1, _pos.y + 1));
        buf(Cell{this, 0, _style.prompt, _readline._autocomplete._rhs});
        for (std::size_t i = 0; i < _readline._autocomplete._hls; ++i) {
          buf.col(Pos(_readline._autocomplete._hli + i, _pos.y + 1)).style.attr |= Style::Reverse;
        }
      }
      buf.cursor(_pos);
      buf(Cell{this, 0, _style.prompt, _readline._prompt.lhs});
      buf(Cell{this, 0, _style.text, _readline._input.fmt});
      buf(Cell{this, 0, _style.prompt, _readline._prompt.rhs});
      buf.col(Pos(_readline._input.cur + 1, _pos.y)).style.attr |= Style::Reverse;
      return true;
    }
    case Display: {
      buf.cursor(_pos);
      buf(Cell{this, 0, _status ? _style.success : _style.error, _readline._prompt.lhs});
      Text::View vbuf {_buf};
      buf(Cell{this, 0, _style.text, std::string(vbuf.colstr(0, _size.w - 1))});
      return true;
    }
  }
  return false;
}

// Status -----------------------------------------------------------------------

Status::Status(Ctx ctx) : Scene(ctx) {
}

Status::~Status() {
}

void Status::on_winch(Size const& size) {
  _size = size;
}

bool Status::on_input(Read::Ctx const& ctx) {
  return false;
}

bool Status::on_update(Tick const delta) {
  widget_fps();
  widget_dir();
  return false;
}

bool Status::on_render(Buffer& buf) {
  buf.cursor(Pos(0, 1));
  for (std::size_t x = 0; x < _size.w; ++x) {
    buf(Cell{this, 0, _style.line, _text.line});
  }
  buf.cursor(Pos(0, 1));
  buf(Cell{this, 0, _style.name, _text.name});
  buf(Cell{this, 0, _style.val, " " + _text.fpsv});
  buf(Cell{this, 0, _style.val, " " + std::to_string(_ctx->_fps_dropped)});
  buf(Cell{this, 0, _style.val, " " + _text.frames});
  buf(Cell{this, 0, _style.val, " " + _text.time});
  buf(Cell{this, 0, _style.val, " " + _text.dir});
  return false;
}

void Status::widget_fps() {
  _text.fpsv = std::to_string(_ctx->_fps_actual);
  _text.frames = std::to_string(_ctx->_frames);
  _text.time = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(_ctx->_time).count());
}

void Status::widget_dir() {
  auto& snake = *std::dynamic_pointer_cast<Snake>(std::dynamic_pointer_cast<Root>(_ctx->_root)->_scenes.at("snake"));
  switch (snake._dir_prev) {
    case Snake::Up: {
      _text.dir = _sym.up;
      break;
    }
    case Snake::Down: {
      _text.dir = _sym.down;
      break;
    }
    case Snake::Left: {
      _text.dir = _sym.left;
      break;
    }
    case Snake::Right: {
      _text.dir = _sym.right;
      break;
    }
  }
  _text.dir += " ";
  std::size_t len = 0;
  auto& snake_dir = snake._dir;
  for (auto const& e : snake_dir) {
    ++len;
    switch (e) {
      case Snake::Up: {
        _text.dir += _sym.up;
        break;
      }
      case Snake::Down: {
        _text.dir += _sym.down;
        break;
      }
      case Snake::Left: {
        _text.dir += _sym.left;
        break;
      }
      case Snake::Right: {
        _text.dir += _sym.right;
        break;
      }
    }
  }
  for (; len < 8; ++len) {
    _text.dir += "-";
  }
}

// Buffer -----------------------------------------------------------------------

// TODO handle out of range buffer read writes

void Buffer::operator()(Cell const& cell) {
  OB::Text::View view {cell.text};
  for (auto const& e : view) {
    if (e.cols == 2) {
      if (_pos.x + 1 == _size.w - 1) {
        _pos.x = 0;
        if (++_pos.y >= _size.h) {
          _pos.y = 0;
        }
      }
      auto& val = _value.at(_size.h - _pos.y - 1).at(_pos.x);
      if (cell.zidx >= val.zidx) {
        val = Cell{cell.scene, cell.zidx, cell.style, std::string{e.str}};
      }
      if (++_pos.x >= _size.w) {
        _pos.x = 0;
        if (++_pos.y >= _size.h) {
          _pos.y = 0;
        }
      }
    }
    else {
      auto& val = _value.at(_size.h - _pos.y - 1).at(_pos.x);
      if (cell.zidx >= val.zidx) {
        val = Cell{cell.scene, cell.zidx, cell.style, std::string{e.str}};
      }
      if (++_pos.x >= _size.w) {
        _pos.x = 0;
        if (++_pos.y >= _size.h) {
          _pos.y = 0;
        }
      }
    }
  }
}

Cell& Buffer::at(Pos const& pos) {
  return _value.at(pos.y).at(pos.x);
}

std::vector<Cell>& Buffer::row(std::size_t y) {
  return _value.at(_size.h - y - 1);
}

Cell& Buffer::col(Pos const& pos) {
  return _value.at(_size.h - pos.y - 1).at(pos.x);
}

Size Buffer::size() {
  return _size;
}

Pos Buffer::cursor() {
  return _pos;
}

void Buffer::cursor(Pos const& pos) {
  _pos = pos;
}

void Buffer::size(Size const& size) {
  _size = size;
  _pos = Pos();
  _value.clear();
  for (std::size_t h = 0; h < _size.h; ++h) {
    _value.emplace_back();
    for (std::size_t w = 0; w < _size.w; ++w) {
      _value.back().emplace_back();
    }
  }
}

bool Buffer::empty() {
  return _value.empty();
}

void Buffer::clear() {
  _pos = Pos();
  _size = Size();
  _value.clear();
}

// Engine ------------------------------------------------------------------------

Engine::Engine(OB::Parg& pg) : _pg {pg} {
  _line.reserve(1000000);
}

void Engine::start() {
  _io.run();
}

void Engine::stop() {
  _io.stop();
}

void Engine::winch() {
  Term::size(_size.w, _size.h);
  _buf.size(_size);
  _buf_prev = _buf;
  _root->on_winch(_size);
}

void Engine::screen_init() {
  std::cout
  << aec::cursor_hide
  << aec::screen_push
  << aec::cursor_hide
  << aec::mouse_enable
  // << aec::focus_enable
  << aec::screen_clear
  << aec::cursor_home
  << std::flush;
  _term_mode.set_raw();
}

void Engine::screen_deinit() {
  std::cout
  // << aec::focus_disable
  << aec::mouse_disable
  << aec::nl
  << aec::screen_pop
  << aec::cursor_show
  << std::flush;
  _term_mode.set_cooked();
}

void Engine::lang_init() {
  env_init(_env, 0, nullptr);

  (*_env)["fps"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
    auto x = eval(sym_xpr("a"), e);
    if (auto const v = xpr_int(&x)) {
      _fps = static_cast<int>(*v);
      _tick = static_cast<Tick>(1000000000 / _fps);
      return x;
    }
    throw std::runtime_error("expected 'Int'");
  }}, _env, Val::evaled};
}

void Engine::await_signal() {
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
    auto const snake = std::dynamic_pointer_cast<Snake>(std::dynamic_pointer_cast<Root>(_root)->_scenes.at("snake"));
    snake->_state = Snake::State::Stopped;
    screen_deinit();
    kill(getpid(), SIGSTOP);
  });

  _sig.on_signal(SIGCONT, [&](auto const& ec, auto sig) {
    // std::cerr << "\nEvent: " << Belle::Signal::str(sig) << "\n";
    _sig.wait();
    screen_init();
    winch();
    _tick_begin = Clock::now();
    await_tick();
  });

  _sig.wait();
}

void Engine::await_read() {
  _read.on_read([&](auto const& ctx) {
    auto nctx = ctx;
    bool found {false};
    std::visit([&](auto& e) {found = on_read(e);}, nctx);
    if (!found) {
      if (auto const& x = std::get_if<Mouse>(&nctx)) {
        _buf.col(Pos(x->pos.x, x->pos.y)).scene->on_input(nctx);
      }
      else {
        _root->on_input(nctx);
      }
    }
  });

  _read.run();
}

void Engine::await_tick() {
  auto const delta = std::chrono::duration_cast<Tick>(_tick - _tick_timer.time<Tick>());
  if (delta >= 0ms) {
    _timer.expires_at(_tick_timer.end() + delta);
  }
  else {
    _timer.expires_at(_tick_timer.end() + (_tick - (_tick_timer.time<Tick>() % _tick)));
  }
  _timer.async_wait([&](auto ec) {
    if (ec) {return;}
    on_tick();
  });
}

void Engine::on_tick() {
  _tick_end = Clock::now();
  auto delta = std::chrono::duration_cast<Tick>(_tick_end - _tick_begin);
  _time += delta;
  _tick_begin = _tick_end;
  if (delta.count() > 0) {
    _fps_actual = std::round(1000000000.0 / delta.count());
  }
  if (delta > _tick) {
    int const dropped {static_cast<int>((delta.count() / _tick.count())) - 1};
    _fps_dropped += dropped;
  }

  _tick_timer.clear();
  _tick_timer.start(_tick_begin);

  _root->on_update(delta);
  _root->on_render(_buf);

  for (std::size_t y = 0; y < _buf.size().h; ++y) {
    for (std::size_t x = 0; x < _buf.size().w; ++x) {
      auto const& cell = _buf.at(Pos(x, y));
      auto const& prev = _buf_prev.at(Pos(x, y));

      if ((cell.text != prev.text) ||
          (cell.style.attr != prev.style.attr) ||
          (cell.style.type != prev.style.type) ||
          (cell.style.fg.r != prev.style.fg.r || cell.style.fg.g != prev.style.fg.g || cell.style.fg.b != prev.style.fg.b) ||
          (cell.style.bg.r != prev.style.bg.r || cell.style.bg.g != prev.style.bg.g || cell.style.bg.b != prev.style.bg.b)) {

        bool diff_attr {_style.attr != cell.style.attr};
        bool diff_type {_style.type != cell.style.type};
        bool diff_fg {_style.fg.r != cell.style.fg.r || _style.fg.g != cell.style.fg.g || _style.fg.b != cell.style.fg.b};
        bool diff_bg {_style.bg.r != cell.style.bg.r || _style.bg.g != cell.style.bg.g || _style.bg.b != cell.style.bg.b};

        _line += aec::cursor_set(x + 1, y + 1);

        if (diff_attr) {
          _style = Style();
          _style.attr = cell.style.attr;
          _line += aec::clear;
          diff_fg = true;
          diff_bg = true;
          if (_style.attr != Style::Null) {
            if (_style.attr & Style::Bold) {_line += aec::bold;}
            if (_style.attr & Style::Reverse) {_line += aec::reverse;}
          }
        }

        if (diff_type) {
          _style.type = cell.style.type;
        }

        if (_style.type == Style::Bit_24) {
          if (diff_fg) {
            _style.fg = cell.style.fg;
            _line += "\x1b[38;2;";
            _line += std::to_string(_style.fg.r);
            _line += ";";
            _line += std::to_string(_style.fg.g);
            _line += ";";
            _line += std::to_string(_style.fg.b);
            _line += "m";
          }

          if (diff_bg) {
            _style.bg = cell.style.bg;
            _line += "\x1b[48;2;";
            _line += std::to_string(_style.bg.r);
            _line += ";";
            _line += std::to_string(_style.bg.g);
            _line += ";";
            _line += std::to_string(_style.bg.b);
            _line += "m";
          }
        }

        _line += cell.text;
      }
    }
    _bsize += _line.size();
    write();
  }
  // if (_bsize) {
  //   std::cerr << "write> " << _bsize << "\n";
  // }
  _bsize = 0;
  _buf_prev = _buf;

  ++_frames;
  _tick_timer.stop();
  await_tick();
}

bool Engine::on_read(Read::Null& ctx) {
  // TODO log error
  // std::cerr << "read> null: " << hex_encode(ctx.str) << "\n";
  return true;
}

bool Engine::on_read(Read::Mouse& ctx) {
  ctx.pos.x -= 1;
  ctx.pos.y = _size.h - ctx.pos.y;
  // std::cerr << "mouse> " << ctx.pos.x << ":" << ctx.pos.y << "\n";
  if (auto const x = _input.find(ctx.ch); x != _input.end()) {
    eval(x->second, _env);
    return true;
  }
  return false;
}

bool Engine::on_read(Read::Key& ctx) {
  // std::cerr << "read>  key: " << hex_encode(ctx.str) << "\n";
  switch (ctx.ch) {
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
  }
  if (auto const x = _input.find(ctx.ch); x != _input.end()) {
    eval(x->second, _env);
    return true;
  }
  return false;
}

void Engine::write() {
  if (_line.empty()) {return;}
  int num {0};
  char const* ptr {_line.data()};
  std::size_t size {_line.size()};
  // std::cerr << "write> " << size << "\n";
  while (size > 0 && static_cast<std::size_t>(num = ::write(STDIN_FILENO, ptr, size)) != size) {
    if (num < 0) {
      if (errno == EINTR || errno == EAGAIN) {continue;}
      throw std::runtime_error("write failed");
    }
    size -= static_cast<std::size_t>(num);
    ptr += static_cast<std::size_t>(num);
    // std::cerr << "write> " << size << "\n";
  }
  _line.clear();
}

void Engine::run() {
  try {
    screen_init();
    _root = std::make_shared<Root>(this);
    winch();
    lang_init();
    await_signal();
    await_read();
    _tick_begin = Clock::now();
    await_tick();
    start();
    screen_deinit();
  }
  catch(...) {
    screen_deinit();
    throw;
  }
}

}; // namespace Nyble

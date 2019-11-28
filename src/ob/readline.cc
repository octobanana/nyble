#include "ob/readline.hh"
#include "ob/string.hh"
#include "ob/text.hh"
#include "ob/term.hh"

#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <deque>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <filesystem>

namespace OB
{

namespace aec = OB::Term::ANSI_Escape_Codes;
namespace fs = std::filesystem;

void Readline::autocomplete(std::function<std::vector<std::string>()> const& val_)
{
  _autocomplete._update = val_;
}

void Readline::boundaries(std::string const& val_)
{
  _boundaries = val_;
}

std::string Readline::word_under_cursor(std::string const& delimiters_)
{
  auto const word = std::string(_input.str.prev_word(_input.off + _input.idx - 1, OB::Text::View(delimiters_)));

  return word.size() == 1 && word.find_first_of(_boundaries) != std::string::npos ? "" : word;
}

Readline& Readline::style(std::string const& style_)
{
  _style.input = style_;
  _autocomplete._style.normal = style_;

  return *this;
}

Readline& Readline::prompt(std::string const& str_, std::string const& style_)
{
  _prompt.str = str_;
  _style.prompt = style_;
  _autocomplete._style.prompt = style_;
  _prompt.fmt = aec::wrap(str_, style_);

  return *this;
}

std::string Readline::render() const
{
  std::ostringstream ss;

  // -------------------------------------------------------------
  switch (_mode)
  {
    case Mode::autocomplete_init:
    case Mode::autocomplete:
    {
      ss
      << aec::cursor_hide
      << aec::cr
      << _autocomplete
      << aec::clear;

      break;
    }

    default:
    {
      ss
      << aec::cursor_hide
      << aec::cr
      << _style.input
      << OB::String::repeat(_width, " ")
      << aec::cr
      << _prompt.lhs
      << _style.input
      << _input.fmt
      << aec::clear
      << _prompt.rhs
      << aec::cr
      << aec::cursor_right(_input.cur + 1)
      << aec::cursor_show;

      break;
    }
  }
  // -------------------------------------------------------------

  // ss
  // << aec::cursor_hide
  // << aec::cr
  // << _style.input
  // << OB::String::repeat(_width, " ")
  // << aec::cr
  // << _prompt.lhs
  // << _style.input
  // << _input.fmt
  // << aec::clear
  // << _prompt.rhs;

  // switch (_mode)
  // {
  //   case Mode::autocomplete_init:
  //   case Mode::autocomplete:
  //   {
  //     ss
  //     << aec::nl
  //     << _autocomplete
  //     << aec::cursor_up(1);

  //     break;
  //   }

  //   default:
  //   {
  //     if (_mode_prev == Mode::autocomplete_init || _mode_prev == Mode::autocomplete)
  //     {
  //       ss
  //       << aec::nl
  //       << aec::erase_line
  //       << aec::cursor_up(1);
  //     }

  //     break;
  //   }
  // }

  // ss
  // << aec::cr
  // << aec::cursor_right(_input.cur + 1)
  // << aec::cursor_show;

  return ss.str();
}

Readline& Readline::refresh()
{
  _prompt.lhs = _prompt.fmt;
  _prompt.rhs.clear();

  if (_input.str.cols() + 2 > _width)
  {
    std::size_t pos {_input.off + _input.idx - 1};
    std::size_t cols {0};

    for (; pos != OB::Text::String::npos && cols < _width - 2; --pos)
    {
      cols += _input.str.at(pos).cols;
    }

    if (pos == OB::Text::String::npos)
    {
      pos = 0;
    }

    std::size_t end {_input.off + _input.idx - pos};
    _input.fmt.str(_input.str.substr(pos, end));

    if (_input.fmt.cols() > _width - 2)
    {
      while (_input.fmt.cols() > _width - 2)
      {
        _input.fmt.erase(0, 1);
      }

      _input.cur = _input.fmt.cols();

      if (_input.cur == OB::Text::String::npos)
      {
        _input.cur = 0;
      }

      _prompt.lhs = aec::wrap("<", _style.prompt);
    }
    else
    {
      _input.cur = _input.fmt.cols();

      if (_input.cur == OB::Text::String::npos)
      {
        _input.cur = 0;
      }

      while (_input.fmt.cols() <= _width - 2)
      {
        _input.fmt.append(std::string(_input.str.at(end++).str));
      }

      _input.fmt.erase(_input.fmt.size() - 1, 1);
    }

    if (_input.off + _input.idx < _input.str.size())
    {
      _prompt.rhs = aec::wrap(
        OB::String::repeat(_width - _input.fmt.cols() - 2, " ") + ">",
        _style.prompt);
    }
  }
  else
  {
    _input.fmt = _input.str;
    _input.cur = _input.fmt.cols(0, _input.idx);

    if (_input.cur == OB::Text::String::npos)
    {
      _input.cur = 0;
    }
  }

  return *this;
}

Readline& Readline::size(std::size_t const width_, std::size_t const height_)
{
  _width = width_;
  _height = height_;

  _autocomplete.width(width_);

  return *this;
}

Readline& Readline::clear()
{
  mode(Mode::normal);
  _input = {};

  return *this;
}

std::string Readline::get()
{
  return _res;
}

bool Readline::operator()(OB::Text::Char32 input)
{
  bool loop {true};

  switch (input.ch())
  {
    case OB::Term::Key::escape:
    {
      if (_mode != Mode::normal)
      {
        mode(Mode::normal);
        _input.off = _input.offp;
        _input.idx = _input.idxp;
        _input.str = _input.buf;
        hist_reset();
        refresh();

        break;
      }

      loop = false;
      _input.save_file = false;
      _input.clear_input = true;

      break;
    }

    case OB::Term::ctrl_key('r'):
    case OB::Term::Key::tab:
    {
      if (_mode != Mode::autocomplete_init && _mode != Mode::autocomplete)
      {
        ac_init();
      }
      else
      {
        mode(Mode::autocomplete);
        ac_next();
      }

      break;
    }

    case OB::Term::ctrl_key('c'):
    {
      // exit the command prompt
      mode(Mode::normal);
      loop = false;
      _input.save_file = false;
      _input.save_local = false;
      _input.clear_input = true;

      break;
    }

    // TODO impl copy/paste like shell
    case OB::Term::ctrl_key('u'):
    {
      mode(Mode::normal);
      _input.clipboard = _input.str;
      edit_clear();
      refresh();
      hist_reset();

      break;
    }

    case OB::Term::ctrl_key('y'):
    {
      mode(Mode::normal);
      edit_insert(_input.clipboard);
      refresh();
      hist_reset();

      break;
    }

    case OB::Term::Key::newline:
    {
      switch (_mode)
      {
        case Mode::autocomplete_init:
        case Mode::autocomplete:
        {
          mode(Mode::normal);

          break;
        }

        default:
        {
          // submit the input string
          loop = false;

          break;
        }
      }

      break;
    }

    case OB::Term::Key::up:
    case OB::Term::ctrl_key('p'):
    {
      switch (_mode)
      {
        case Mode::normal:
        {
          mode(Mode::history_init);
          hist_next();

          break;
        }

        case Mode::history_init:
        {
          mode(Mode::history);
          hist_next();

          break;
        }

        case Mode::history:
        {
          hist_next();

          break;
        }

        case Mode::autocomplete_init:
        {
          mode(Mode::autocomplete);
          ac_next_section();

          break;
        }

        case Mode::autocomplete:
        {
          ac_next_section();

          break;
        }
      }

      break;
    }

    case OB::Term::Key::down:
    case OB::Term::ctrl_key('n'):
    {
      switch (_mode)
      {
        case Mode::normal:
        {
          mode(Mode::history_init);
          hist_prev();

          break;
        }

        case Mode::history_init:
        {
          mode(Mode::history);
          hist_prev();

          break;
        }

        case Mode::history:
        {
          hist_prev();

          break;
        }

        case Mode::autocomplete_init:
        {
          mode(Mode::autocomplete);
          ac_prev_section();

          break;
        }

        case Mode::autocomplete:
        {
          ac_prev_section();

          break;
        }
      }

      break;
    }

    case OB::Term::Key::right:
    case OB::Term::ctrl_key('f'):
    {
      switch (_mode)
      {
        case Mode::autocomplete_init:
        {
          mode(Mode::autocomplete);
          ac_next();

          break;
        }

        case Mode::autocomplete:
        {
          ac_next();

          break;
        }

        default:
        {
          curs_right();

          break;
        }
      }

      break;
    }

    case OB::Term::Key::left:
    case OB::Term::ctrl_key('b'):
    {
      switch (_mode)
      {
        case Mode::autocomplete_init:
        {
          mode(Mode::autocomplete);
          ac_prev();

          break;
        }

        case Mode::autocomplete:
        {
          ac_prev();

          break;
        }

        default:
        {
          curs_left();

          break;
        }
      }

      break;
    }

    case OB::Term::Key::end:
    case OB::Term::ctrl_key('e'):
    {
      switch (_mode)
      {
        case Mode::autocomplete_init:
        {
          mode(Mode::autocomplete);
          ac_end();

          break;
        }

        case Mode::autocomplete:
        {
          ac_end();

          break;
        }

        default:
        {
          curs_end();

          break;
        }
      }

      break;
    }

    case OB::Term::Key::home:
    case OB::Term::ctrl_key('a'):
    {
      switch (_mode)
      {
        case Mode::autocomplete_init:
        {
          mode(Mode::autocomplete);
          ac_begin();

          break;
        }

        case Mode::autocomplete:
        {
          ac_begin();

          break;
        }

        default:
        {
          curs_begin();

          break;
        }
      }

      break;
    }

    case OB::Term::Key::delete_:
    case OB::Term::ctrl_key('d'):
    {
      mode(Mode::normal);
      edit_delete();
      refresh();
      hist_reset();

      break;
    }

    case OB::Term::Key::backspace:
    case OB::Term::ctrl_key('h'):
    {
      mode(Mode::normal);
      edit_backspace_autopair();
      refresh();
      hist_reset();

      break;
    }

    default:
    {
      mode(Mode::normal);
      if (input.ch() < 0xF0000 &&
        (input.ch() == OB::Term::Key::space ||
        OB::Text::is_graph(static_cast<std::int32_t>(input.ch()))))
      {
        edit_insert_autopair(input);
        refresh();
        hist_reset();
      }

      break;
    }
  }

  if (loop)
  {
    return false;
  }

  _res = normalize(_input.str);

  if (! _res.empty())
  {
    if (_input.save_local)
    {
      hist_push(_res);
    }

    if (_input.save_file && _input.str.str().front() != ' ')
    {
      hist_save(_res);
    }
  }

  hist_reset();

  if (_input.clear_input)
  {
    _res.clear();
  }

  return true;
}

void Readline::curs_begin()
{
  // move cursor to start of line

  if (_input.idx || _input.off)
  {
    _input.idx = 0;
    _input.off = 0;

    refresh();
  }
}

void Readline::curs_end()
{
  // move cursor to end of line

  if (_input.str.empty())
  {
    return;
  }

  if (_input.off + _input.idx < _input.str.size())
  {
    if (_input.str.cols() + 2 > _width)
    {
      _input.off = _input.str.size() - _width + 2;
      _input.idx = _width - 2;
    }
    else
    {
      _input.idx = _input.str.size();
    }

    refresh();
  }
}

void Readline::curs_left()
{
  // move cursor left

  if (_input.off || _input.idx)
  {
    if (_input.off)
    {
      --_input.off;
    }
    else
    {
      --_input.idx;
    }

    refresh();
  }
}

void Readline::curs_right()
{
  // move cursor right

  if (_input.off + _input.idx < _input.str.size())
  {
    if (_input.idx + 2 < _width)
    {
      ++_input.idx;
    }
    else
    {
      ++_input.off;
    }

    refresh();
  }
}

void Readline::edit_insert(std::string const& str)
{
  // insert or append char to input buffer

  auto size {_input.str.size()};
  _input.str.insert(_input.off + _input.idx, str);

  if (size != _input.str.size())
  {
    size = _input.str.size() - size;

    if (_input.idx + size + 1 < _width)
    {
      _input.idx += size;
    }
    else
    {
      _input.off += _input.idx + size - _width + 1;
      _input.idx = _width - 1;
    }
  }
}

void Readline::edit_clear()
{
  // clear line

  _input.idx = 0;
  _input.off = 0;
  _input.str.clear();
}

bool Readline::edit_delete()
{
  // erase char under cursor

  if (_input.str.empty())
  {
    _input.str.clear();

    return false;
  }

  if (_input.off + _input.idx < _input.str.size())
  {
    if (_input.idx + 2 < _width)
    {
      _input.str.erase(_input.off + _input.idx, 1);
    }
    else
    {
      _input.str.erase(_input.idx, 1);
    }

    return true;
  }
  else if (_input.off || _input.idx)
  {
    if (_input.off)
    {
      _input.str.erase(_input.off + _input.idx - 1, 1);
      --_input.off;
    }
    else
    {
      --_input.idx;
      _input.str.erase(_input.idx, 1);
    }

    return true;
  }

  return false;
}

bool Readline::edit_backspace()
{
  // erase char behind cursor

  if (_input.str.empty())
  {
    _input.str.clear();

    return false;
  }

  _input.str.erase(_input.off + _input.idx - 1, 1);

  if (_input.off || _input.idx)
  {
    if (_input.off)
    {
      --_input.off;
    }
    else if (_input.idx)
    {
      --_input.idx;
    }

    return true;
  }

  return false;
}

void Readline::hist_next()
{
  // cycle backwards in history

  if (_history().empty() && _history.search().empty())
  {
    return;
  }

  bool bounds {_history.search().empty() ?
    (_history.idx < _history().size() - 1) :
    (_history.idx < _history.search().size() - 1)};

  if (bounds || _history.idx == History::npos)
  {
    if (_history.idx == History::npos)
    {
      _input.buf = _input.str;

      if (! _input.buf.empty())
      {
        hist_search(_input.buf);
      }
    }

    ++_history.idx;

    if (_history.search().empty())
    {
      // normal search
      _input.str = _history().at(_history.idx);
    }
    else
    {
      // fuzzy search
      _input.str = _history().at(_history.search().at(_history.idx).idx);
    }

    if (_input.str.size() + 1 >= _width)
    {
      _input.off = _input.str.size() - _width + 2;
      _input.idx = _width - 2;
    }
    else
    {
      _input.off = 0;
      _input.idx = _input.str.size();
    }

    refresh();
  }
}

void Readline::hist_prev()
{
  // cycle forwards in history

  if (_history.idx != History::npos)
  {
    --_history.idx;

    if (_history.idx == History::npos)
    {
      _input.str = _input.buf;
    }
    else if (_history.search().empty())
    {
      // normal search
      _input.str = _history().at(_history.idx);
    }
    else
    {
      // fuzzy search
      _input.str = _history().at(_history.search().at(_history.idx).idx);
    }

    if (_input.str.size() + 1 >= _width)
    {
      _input.off = _input.str.size() - _width + 2;
      _input.idx = _width - 2;
    }
    else
    {
      _input.off = 0;
      _input.idx = _input.str.size();
    }

    refresh();
  }
}

void Readline::hist_reset()
{
  _history.search.clear();
  _history.idx = History::npos;
}

void Readline::hist_search(std::string const& str)
{
  _history.search.clear();

  OB::Text::String input {OB::Text::normalize_foldcase(
    std::regex_replace(OB::Text::trim(str), std::regex("\\s+"),
    " ", std::regex_constants::match_not_null))};

  if (input.empty())
  {
    return;
  }

  std::size_t idx {0};
  std::size_t count {0};
  std::size_t weight {0};
  std::string prev_hist {" "};
  std::string prev_input {" "};
  OB::Text::String hist;

  for (std::size_t i = 0; i < _history().size(); ++i)
  {
    hist.str(OB::Text::normalize_foldcase(_history().at(i)));

    if (hist.size() <= input.size())
    {
      continue;
    }

    idx = 0;
    count = 0;
    weight = 0;
    prev_hist = " ";
    prev_input = " ";

    for (std::size_t j = 0, seq = 0; j < hist.size(); ++j)
    {
      if (idx < input.size() &&
        hist.at(j).str == input.at(idx).str)
      {
        ++seq;
        count += 1;

        if (seq > 1)
        {
          count += 1;
        }

        if (prev_hist == " " && prev_input == " ")
        {
          count += 1;
        }

        prev_input = input.at(idx).str;
        ++idx;

        // short circuit to keep history order
        // comment out to search according to closest match
        if (idx == input.size())
        {
          break;
        }
      }
      else
      {
        seq = 0;
        weight += 2;

        if (prev_input == " ")
        {
          weight += 1;
        }
      }

      prev_hist = hist.at(j).str;
    }

    if (idx != input.size())
    {
      continue;
    }

    while (count && weight)
    {
      --count;
      --weight;
    }

    _history.search().emplace_back(weight, i);
  }

  std::sort(_history.search().begin(), _history.search().end(),
  [](auto const& lhs, auto const& rhs)
  {
    // default to history order if score is equal
    return lhs.score == rhs.score ? lhs.idx < rhs.idx : lhs.score < rhs.score;
  });
}

void Readline::hist_push(std::string const& str)
{
  if (_history().empty())
  {
    _history().emplace_front(str);
  }
  else if (_history().back() != str)
  {
    if (auto pos = std::find(_history().begin(), _history().end(), str);
      pos != _history().end())
    {
      _history().erase(pos);
    }

    _history().emplace_front(str);
  }
}

void Readline::hist_load(fs::path const& path)
{
  if (! path.empty())
  {
    std::ifstream ifile {path};

    if (ifile && ifile.is_open())
    {
      std::string line;

      while (std::getline(ifile, line))
      {
        hist_push(line);
      }
    }

    hist_open(path);
  }
}

void Readline::hist_save(std::string const& str)
{
  if (_history.file.is_open())
  {
    _history.file
    << str << "\n"
    << std::flush;
  }
}

void Readline::hist_open(fs::path const& path)
{
  _history.file.open(path, std::ios::app);

  if (! _history.file.is_open())
  {
    throw std::runtime_error("could not open file '" + path.string() + "'");
  }
}

void Readline::ac_init()
{
  mode(Mode::autocomplete_init);

  _input.offp = _input.off;
  _input.idxp = _input.idx;
  _input.buf = _input.str;

  _autocomplete.word(word_under_cursor(_boundaries));
  _autocomplete.generate().refresh();

  ac_sync();
}

void Readline::ac_sync()
{
  _input.str = _input.buf;
  auto const word = _autocomplete._match.at(_autocomplete._off + _autocomplete._idx);

  _input.str.replace(_input.offp + _input.idxp - _autocomplete.word().size(), _autocomplete.word().size(), word);

  // place cursor at end of new word
  std::size_t const pos {_input.offp + _input.idxp - _autocomplete.word().size() + word.size()};
  if (pos + 1 >= _width)
  {
    _input.off = pos - _width + 2;
    _input.idx = _width - 2;
  }
  else
  {
    _input.idx = pos;
  }

  refresh();
}

void Readline::ac_begin()
{
  _autocomplete.begin().refresh();
  ac_sync();
}

void Readline::ac_end()
{
  _autocomplete.end().refresh();
  ac_sync();
}

void Readline::ac_prev()
{
  _autocomplete.prev().refresh();
  ac_sync();
}

void Readline::ac_next()
{
  _autocomplete.next().refresh();
  ac_sync();
}

void Readline::ac_prev_section()
{
  _autocomplete.prev_section().refresh();
  ac_sync();
}

void Readline::ac_next_section()
{
  _autocomplete.next_section().refresh();
  ac_sync();
}

void Readline::edit_insert_autopair(OB::Text::Char32 const& val)
{
  auto const i = _input.off + _input.idx;
  auto const lhs = val.ch();
  auto const rhs = i < _input.str.size() ? OB::Text::Char32(std::string(_input.str.at(i))).ch() : '\0';
  auto const prv = i > 0 ? OB::Text::Char32(std::string(_input.str.at(i - 1))).ch() : '\0';

  switch (lhs)
  {
    case '"':
    {
      if (rhs == '"')
      {
        if (prv == '\\')
        {
          edit_insert(val.str());
        }
        else
        {
          curs_right();
        }
      }
      else if (prv != '\\')
      {
        edit_insert(val.str());
        edit_insert("\"");
        curs_left();
      }
      else
      {
        edit_insert(val.str());
      }

      break;
    }

    case '(':
    {
      if (prv == '\\')
      {
        edit_insert(val.str());
      }
      else
      {
        edit_insert(val.str());
        edit_insert(")");
        curs_left();
      }

      break;
    }

    case ')':
    {
      if (rhs == ')')
      {
        if (prv == '\\')
        {
          edit_insert(val.str());
        }
        else
        {
          curs_right();
        }
      }
      else
      {
        edit_insert(val.str());
      }

      break;
    }

    case '[':
    {
      if (prv == '\\')
      {
        edit_insert(val.str());
      }
      else
      {
        edit_insert(val.str());
        edit_insert("]");
        curs_left();
      }

      break;
    }

    case ']':
    {
      if (rhs == ']')
      {
        if (prv == '\\')
        {
          edit_insert(val.str());
        }
        else
        {
          curs_right();
        }
      }
      else
      {
        edit_insert(val.str());
      }

      break;
    }

    case '{':
    {
      if (prv == '\\')
      {
        edit_insert(val.str());
      }
      else
      {
        edit_insert(val.str());
        edit_insert("}");
        curs_left();
      }

      break;
    }

    case '}':
    {
      if (rhs == '}')
      {
        if (prv == '\\')
        {
          edit_insert(val.str());
        }
        else
        {
          curs_right();
        }
      }
      else
      {
        edit_insert(val.str());
      }

      break;
    }

    default:
    {
      edit_insert(val.str());

      break;
    }
  }
}

void Readline::edit_backspace_autopair()
{
  auto const i = _input.off + _input.idx;

  if (i < 1 || i >= _input.str.size())
  {
    edit_backspace();

    return;
  }

  auto const lhs = OB::Text::Char32(std::string(_input.str.at(i - 1))).ch();
  auto const rhs = OB::Text::Char32(std::string(_input.str.at(i))).ch();
  auto const prv = i > 1 ? OB::Text::Char32(std::string(_input.str.at(i - 2))).ch() : '\0';

  switch (lhs)
  {
    case '"':
    {
      if (rhs == '"' && prv != '\\')
      {
        edit_delete();
      }

      break;
    }

    case '(':
    {
      if (rhs == ')' && prv != '\\')
      {
        edit_delete();
      }

      break;
    }

    case '[':
    {
      if (rhs == ']' && prv != '\\')
      {
        edit_delete();
      }

      break;
    }

    case '{':
    {
      if (rhs == '}' && prv != '\\')
      {
        edit_delete();
      }

      break;
    }

    default:
    {
      break;
    }
  }

  edit_backspace();
}

std::string Readline::normalize(std::string const& str) const
{
  // trim leading and trailing whitespace
  // collapse sequential whitespace
  // return std::regex_replace(OB::Text::trim(str), std::regex("\\s+"),
  //   " ", std::regex_constants::match_not_null);
  return str;
}

void Readline::mode(Readline::Mode const mode_)
{
  _mode_prev = _mode;
  _mode = mode_;
}

std::ostream& operator<<(std::ostream& os, Readline const& obj)
{
  os << obj.render();

  return os;
}

} // namespace OB

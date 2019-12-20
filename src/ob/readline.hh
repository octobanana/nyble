#ifndef OB_READLINE_HH
#define OB_READLINE_HH

#include "ob/term.hh"
#include "ob/text.hh"
#include "ob/string.hh"

#include <cstddef>

#include <stack>
#include <deque>
#include <vector>
#include <string>
#include <limits>
#include <fstream>
#include <filesystem>

namespace OB
{

namespace fs = std::filesystem;
namespace aec = OB::Term::ANSI_Escape_Codes;

class Readline
{
public:

  Readline() = default;

  friend std::ostream& operator<<(std::ostream& os, Readline const& obj);

  void autocomplete(std::function<std::vector<std::string>()> const& val_);
  void boundaries(std::string const& val_);
  std::string word_under_cursor(std::string const& delimiters);
  Readline& style(std::string const& style_ = {});
  Readline& prompt(std::string const& str_, std::string const& style_ = {});
  Readline& size(std::size_t const width_, std::size_t const height_);
  bool operator()(OB::Text::Char32 input);
  std::string render() const;
  std::string get();
  Readline& clear();
  Readline& refresh();
  Readline& normal();

  void hist_push(std::string const& str);
  void hist_load(fs::path const& path);

// private:

  // cursor
  void curs_begin();
  void curs_end();
  void curs_left();
  void curs_right();

  // edit
  void edit_clear();
  bool edit_delete();
  void edit_insert(std::string const& str);
  bool edit_backspace();
  // TODO autopair should not run when pasting
  void edit_insert_autopair(OB::Text::Char32 const& val);
  void edit_backspace_autopair();

  // history
  void hist_prev();
  void hist_next();
  void hist_reset();
  void hist_search(std::string const& str);
  void hist_open(fs::path const& path);
  void hist_save(std::string const& str);

  // autocomplete
  void ac_init();
  void ac_sync();
  void ac_begin();
  void ac_end();
  void ac_prev();
  void ac_next();
  void ac_prev_section();
  void ac_next_section();

  std::string normalize(std::string const& str) const;

  enum class Mode
  {
    normal,
    history_init,
    history,
    autocomplete_init,
    autocomplete,
  };

  void mode(Mode const mode_);

  Mode _mode {Mode::normal};
  Mode _mode_prev {Mode::normal};

  std::string _boundaries {" "};

  // width and height of the terminal
  std::size_t _width {0};
  std::size_t _height {0};

  std::string _res;

  struct Style
  {
    std::string prompt;
    std::string input;
  } _style;

  struct Prompt
  {
    std::string lhs;
    std::string rhs;
    std::string fmt;
    std::string str {":"};
  } _prompt;

  struct Input
  {
    bool save_local {true};
    bool save_file {true};
    bool clear_input {false};
    std::size_t offp {0};
    std::size_t idxp {0};
    std::size_t off {0};
    std::size_t idx {0};
    std::size_t cur {0};
    std::string buf;
    std::string clipboard;
    OB::Text::String str;
    OB::Text::String fmt;
  } _input;

  struct History
  {
    static std::size_t constexpr npos {std::numeric_limits<std::size_t>::max()};

    struct Search
    {
      struct Result
      {
        Result(std::size_t s, std::size_t i) noexcept :
          score {s},
          idx {i}
        {
        }

        std::size_t score {0};
        std::size_t idx {0};
      };

      using value_type = std::deque<Result>;

      value_type& operator()()
      {
        return val;
      }

      bool empty()
      {
        return val.empty();
      }

      void clear()
      {
        idx = History::npos;
        val.clear();
      }

      std::size_t idx {0};
      value_type val;
    } search;

    using value_type = std::deque<std::string>;

    value_type& operator()()
    {
      return val;
    }

    value_type val;
    std::size_t idx {npos};

    std::ofstream file;
  } _history;

  struct Autocomplete
  {
    friend std::ostream& operator<<(std::ostream& os, Autocomplete const& obj)
    {
      os << obj.render();

      return os;
    }

    void width(std::size_t const width_)
    {
      _width = width_;
    }

    std::string render() const
    {
      return _value;
    }

    Autocomplete& refresh()
    {
      _lhs = _off ? "<" : "";
      _rhs = _off + _max != _match.size() ? ">" : " ";

      std::ostringstream ss;

      for (std::size_t i = 0, n = _off ? 1ul : 0ul; i < _max; ++i)
      {
        if (i == _idx)
        {
          _hli = n;
          _hls = _view.at(i + _off).cols();
          ss << _match.at(i + _off);
        }
        else
        {
          ss << _match.at(i + _off);
        }

        if (i + 1 < _max)
        {
          ss << " ";
        }

        if (i < _idx)
        {
          n += _view.at(i + _off).cols() + 1;
        }
      }

      _text = ss.str();

      return *this;
    }

    // Autocomplete& refresh()
    // {
    //   std::ostringstream ss;

    //   if (_off)
    //   {
    //     ss
    //     << aec::clear
    //     << _style.prompt
    //     << "<"
    //     << aec::clear
    //     << _style.normal
    //     << OB::String::repeat(_width - 2, aec::space)
    //     << aec::clear
    //     << _style.prompt
    //     << (_off + _max != _match.size() ? ">" : " ")
    //     << aec::clear
    //     << _style.normal
    //     << aec::cr << aec::cursor_right(1);
    //   }
    //   else
    //   {
    //     ss
    //     << aec::clear
    //     << _style.normal
    //     << OB::String::repeat(_width - 1, aec::space)
    //     << aec::clear
    //     << _style.prompt
    //     << (_off + _max != _match.size() ? ">" : " ")
    //     << aec::clear
    //     << _style.normal
    //     << aec::cr;
    //   }

    //   for (std::size_t i = 0; i < _max; ++i)
    //   {
    //     if (i == _idx)
    //     {
    //       ss
    //       << aec::clear
    //       << _style.select
    //       << _match.at(i + _off)
    //       << aec::clear
    //       << _style.normal;
    //     }
    //     else
    //     {
    //       ss
    //       << _match.at(i + _off);
    //     }

    //     if (i + 1 < _max)
    //     {
    //       ss
    //       << aec::space;
    //     }
    //   }

    //   ss
    //   << aec::clear;

    //   _value = ss.str();

    //   return *this;
    // }

    Autocomplete& begin()
    {
      // move to begin of list
      // idx = 0
      // off = 0
      while (_off)
      {
        prev_section();
      }

      return *this;
    }

    Autocomplete& end()
    {
      while (_off + _max < _match.size())
      {
        next_section();
      }

      return *this;
    }

    Autocomplete& next()
    {
      if (_idx + 1 < _max)
      {
        ++_idx;
      }
      else if (_off + _max < _match.size())
      {
        _off += _max;
        _maxp.push(_max);
        setup();
      }

      return *this;
    }

    Autocomplete& prev()
    {
      if (_idx)
      {
        --_idx;
      }
      else if (_off)
      {
        _off -= _maxp.top();
        _maxp.pop();
        setup();
        _idx = _max - 1;
      }

      return *this;
    }

    Autocomplete& next_section()
    {
      _idx = _max - 1;
      next();

      return *this;
    }

    Autocomplete& prev_section()
    {
      _idx = 0;
      prev();
      _idx = 0;

      return *this;
    }

    void setup()
    {
      _idx = 0;
      _max = 0;

      if (_view.empty())
      {
        return;
      }

      std::size_t constexpr space {1};
      std::size_t len {_off ? 1ul : 0ul};
      std::size_t tmp {len};
      for (std::size_t i = 0; i < _view.size() && _off + i < _view.size(); ++i, ++_max)
      {
        tmp += _view.at(_off + i).cols() + space;
        if (tmp > _width)
        {
          break;
        }
        len += tmp;
      }
    }

    Autocomplete& word(std::string const& str_)
    {
      _word = str_;

      return *this;
    }

    std::string const& word()
    {
      return _word;
    }

    // call each time autocomplete mode is entered
    Autocomplete& generate()
    {
      _idx = 0;
      _off = 0;
      _max = 0;
      _maxp = {};
      _match.clear();
      _view.clear();

      if (! _update)
      {
        return *this;
      }

      // TODO remove duplicates there could be same named vars in different scopes
      if (_word.size())
      {
        _match = find_similar(_word, _update());
      }
      else
      {
        _match = _update();
        std::sort(_match.begin(), _match.end(),
        [](auto const& lhs, auto const& rhs)
        {
          return (lhs.size() == rhs.size()) ?
            (lhs < rhs) :
            (lhs.size() < rhs.size());
        });
      }

      for (auto const& e : _match)
      {
        _view.emplace_back(OB::Text::View(e));
      }

      setup();

      return *this;
    }

    std::vector<std::string> find_similar(
      std::string const& key, std::vector<std::string> const& values) const
    {
      int const weight_max {8};
      size_t const similar_max {1024};

      // TODO use single vec and erase elements that are > weight_max
      std::vector<std::string> results;
      std::vector<std::pair<int, std::string>> dist;

      for (auto const& val : values)
      {
        if (val.size() < key.size())
        {
          continue;
        }

        int const weight = OB::String::starts_with(val, key) ? 0 :
          OB::String::damerau_levenshtein(key, val, 1, 2, 4, 0);

        if (weight < weight_max)
        {
          dist.emplace_back(weight, val);
        }
      }

      std::sort(dist.begin(), dist.end(),
      [](auto const& lhs, auto const& rhs)
      {
        return (lhs.first == rhs.first) ?
          (lhs.second.size() == rhs.second.size() ?
           lhs.second < rhs.second :
           lhs.second.size() < rhs.second.size()) :
          (lhs.first < rhs.first);
      });

      for (auto const& [key, val] : dist)
      {
        results.emplace_back(val);
      }

      if (results.size() > similar_max)
      {
        results.erase(results.begin() + similar_max, results.end());
      }

      return results;
    }

    struct Style
    {
      std::string prompt;
      std::string normal;
      std::string select {aec::reverse};
    } _style;

    std::size_t _hli {0}; // highlight index
    std::size_t _hls {0}; // highlight size
    std::string _lhs;
    std::string _rhs;
    std::string _text;

    // string value
    std::string _value;

    // word to autocomplete on
    std::string _word;

    // word offset
    std::size_t _off {0};

    // current selected word index
    std::size_t _idx {0};

    // current max number of words that fit on the current screen width
    std::size_t _max {0};

    // previous max number of words that fit on the current screen width
    std::stack<std::size_t> _maxp;

    // screen width
    std::size_t _width {0};

    std::vector<std::string> _match;
    std::vector<OB::Text::View> _view;
    std::function<std::vector<std::string>()> _update;
  } _autocomplete;
};

} // namespace OB

#endif // OB_READLINE_HH

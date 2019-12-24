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

#include "ob/lispp.hh"
#include "ob/term.hh"
#include "ob/text.hh"

#include <boost/multiprecision/gmp.hpp>
#include <boost/multiprecision/mpfr.hpp>

#include <cstdlib>
#include <cstdint>
#include <map>
#include <list>
#include <deque>
#include <bitset>
#include <chrono>
#include <string>
#include <thread>
#include <vector>
#include <limits>
#include <utility>
#include <fstream>
#include <sstream>
#include <variant>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <filesystem>
#include <functional>
#include <string_view>
#include <type_traits>
#include <unordered_map>

namespace fs = std::filesystem;
namespace iom = OB::Term::iomanip;
namespace aec = OB::Term::ANSI_Escape_Codes;
using namespace std::string_literals;

std::string escape(std::string str) {
  std::size_t pos {str.find_first_of("\n\t\r\a\b\f\v\"\'\?")};
  for (;; pos = str.find_first_of("\n\t\r\a\b\f\v\"\'\?", ++pos)) {
    if (pos == std::string::npos) {
      break;
    }
    switch (str.at(pos)) {
      case '\n': {
        str.replace(pos++, 1, "\\n");
        break;
      }
      case '\t': {
        str.replace(pos++, 1, "\\t");
        break;
      }
      case '\r': {
        str.replace(pos++, 1, "\\r");
        break;
      }
      case '\a': {
        str.replace(pos++, 1, "\\a");
        break;
      }
      case '\b': {
        str.replace(pos++, 1, "\\b");
        break;
      }
      case '\f': {
        str.replace(pos++, 1, "\\f");
        break;
      }
      case '\v': {
        str.replace(pos++, 1, "\\v");
        break;
      }
      case '\?': {
        str.replace(pos++, 1, "\\?");
        break;
      }
      // case '\'': {
      //   str.replace(pos++, 1, "\\'");
      //   break;
      // }
      case '"': {
        str.replace(pos++, 1, "\\\"");
        break;
      }
      default: {
        break;
      }
    }
  }
  return str;
}

std::string unescape(std::string str) {
  std::size_t pos {str.find_first_of("\\")};
  for (;; pos = str.find_first_of("\\", ++pos)) {
    if (pos == std::string::npos || pos + 1 == std::string::npos) {
      break;
    }
    switch (str.at(pos + 1)) {
      case 'n': {
        str.replace(pos, 2, "\n");
        break;
      }
      case 't': {
        str.replace(pos, 2, "\t");
        break;
      }
      case 'r': {
        str.replace(pos, 2, "\r");
        break;
      }
      case 'a': {
        str.replace(pos, 2, "\a");
        break;
      }
      case 'b': {
        str.replace(pos, 2, "\b");
        break;
      }
      case 'f': {
        str.replace(pos, 2, "\f");
        break;
      }
      case 'v': {
        str.replace(pos, 2, "\v");
        break;
      }
      case '?': {
        str.replace(pos, 2, "\?");
        break;
      }
      // case '\'': {
      //   str.replace(pos, 2, "'");
      //   break;
      // }
      case '"': {
        str.replace(pos, 2, "\"");
        break;
      }
      default: {
        break;
      }
    }
  }
  return str;
}

std::string plural(std::string const& str, std::string const& end, std::size_t const num) {
  if (num == 1) {return str;}
  return str + end;
}

std::string repeat(std::size_t const num, std::string const& str) {
  if (num == 0) {return {};}
  if (num == 1) {return str;}
  std::string res;
  res.reserve(str.size() * num);
  for (std::size_t i {0}; i < num; ++i) {res += str;}
  return res;
}

std::string replace(std::string str, std::string const& key, std::string const& val, std::size_t size = std::numeric_limits<std::size_t>::max()) {
  for (std::size_t pos {0}; size-- > 0;) {
    pos = str.find(key, pos);
    if (pos == std::string::npos) {break;}
    str.replace(pos, key.size(), val);
    pos += val.size();
  }
  return str;
}

std::string replace(std::string str, std::vector<std::pair<std::string, std::string>> const& vals) {
  for (auto const& [key, val] : vals) {str = replace(str, key, val);}
  return str;
}

std::deque<std::string> split(std::string const& str, std::string const& delim, std::size_t size = std::numeric_limits<std::size_t>::max()) {
  std::string buf;
  std::deque<std::string> tks;
  std::size_t start {0};
  auto end = str.find(delim);
  while ((size-- > 0) && (end != std::string::npos)) {
    buf = str.substr(start, end - start);
    if (buf.size()) {tks.emplace_back(buf);}
    start = end + delim.size();
    end = str.find(delim, start);
  }
  buf = str.substr(start, end);
  if (buf.size()) {tks.emplace_back(buf);}
  return tks;
}

template<typename T, typename F1, typename F2> void for_each(T& t, F1 const& f1, F2 const& f2) {
  if (t.empty()) {return;}
  auto end = --t.end();
  for (auto i = t.begin(); i != end; ++i) {auto& e = *i; f1(e);}
  f2(t.back());
}

struct Typ { enum {
  Int, Rat, Flo, Num, Sym, Str, Atm, Fun, Lst, Xpr, Ukn,
};};

std::vector<std::string> typ_str {
  "Int", "Rat", "Flo", "Num", "Sym", "Str", "Atm", "Fun", "Lst", "Xpr", "Ukn"
};

std::unordered_map<std::string, std::string> paren_begin {
  {"(", ")"},
  {"[", "]"},
  {"{", "}"}
};

std::unordered_map<std::string, std::string> paren_end {
  {")", "("},
  {"]", "["},
  {"}", "{"}
};

Val& Env::operator[](Sym const& sym) {
  return inner[sym];
}

std::optional<Env::Inner::iterator> Env::find(Sym const& sym) {
  if (auto p = inner.find(sym); p != inner.end()) {return p;}
  return outer ? outer->find(sym) : std::nullopt;
}

std::optional<Env::Inner::iterator> Env::find_inner(Sym const& sym) {
  if (auto p = inner.find(sym); p != inner.end()) {return p;}
  return std::nullopt;
}

std::optional<Env::Inner::iterator> Env::find_outer(Sym const& sym) {
  return outer ? outer->find(sym) : std::nullopt;
}

std::optional<Env::Inner::iterator> Env::find_current(Sym const& sym) {
  return current ? current->find(sym) : std::nullopt;
}

std::optional<Env::Inner::iterator> Env::find_current_inner(Sym const& sym) {
  return current ? current->find_inner(sym) : std::nullopt;
}

void Env::dump() {
  for (auto const& [k, v] : inner) {
    std::cerr << k << "\t" << aec::fg_white << "|" << aec::fg_yellow << static_cast<int>(v.ctx) << aec::fg_white << "|" << aec::clear << "\t" << aec::fg_green << print(v.xpr) << aec::clear << "\n";
  }
  if (outer) {
    std::cerr << "\n";
    outer->dump();
  }
}

void Env::dump_inner() {
  for (auto const& [k, v] : inner) {
    std::cerr << k << "\t" << aec::fg_white << "|" << aec::fg_yellow << static_cast<int>(v.ctx) << aec::fg_white << "|" << aec::clear << "\t" << aec::fg_green << print(v.xpr) << aec::clear << "\n";
  }
}

void Env::list(Xpr& x) {
  auto& l = std::get<Lst>(x);
  Xpr nx;
  auto& nl = std::get<Lst>(nx);
  for (auto const& [k, v] : inner) {
    nl.emplace_back(Xpr{Lst{sym_xpr(k), v.xpr}});
  }
  l.emplace_back(nx);
  if (outer) {outer->list(x);}
}

std::shared_ptr<Env> Fun::bind(Sym const& sym, Lst& l, std::shared_ptr<Env> e) {
  if (args.empty() && l.size()) {throw std::runtime_error("'" + sym + "' expected '0' arguments");}
  auto const has_rest {[this]() {
    if (auto const s = xpr_sym(&args.back()); s && *s == "@") {
      return true;
    }
    return false;
  }()};
  if (has_rest) {
    if (l.size() < args.size() - 1) {
      throw std::runtime_error("'" + sym + "' expected at least '" + std::to_string(args.size() - 1) + "' " + plural("argument", "s", args.size() - 1));
    }
    auto ev = std::make_shared<Env>(env, e);
    auto v = l.begin();
    for (auto s = args.begin(), end = --args.end(); s != end; ++s, ++v) {
      if (auto const sym = xpr_sym(&*s)) {
        (*ev)[*sym] = {*v, e};
        continue;
      }
      throw std::runtime_error("'" + sym + "' fn binding expected 'Sym'");
    }
    Xpr rest;
    auto& r = std::get<Lst>(rest);
    r.splice(r.begin(), l, v, l.end());
    // TODO should rest args be evaled?
    (*ev)["@"] = {rest, e, Val::evaled};
    return ev;
  }
  if (l.size() != args.size()) {
    throw std::runtime_error("'" + sym + "' expected '" + std::to_string(args.size()) + "' " + plural("argument", "s", args.size()));
  }
  auto ev = std::make_shared<Env>(env, e);
  auto s = args.begin();
  for (auto& v : l) {
    if (auto const sym = xpr_sym(&*s)) {
      (*ev)[*sym] = {v, e};
      ++s;
      continue;
    }
    throw std::runtime_error("'" + sym + "' fn binding expected 'Sym'");
  }
  return ev;
}

Tks str_tks(std::string_view& str) {
  // TODO handle multiline xprs
  Tks tks;
  std::string r = R"RAW(\s*([()\[\]{}',]|\\.[^\\d]*|"[^"\\]*(?:\\.[^"\\]*)*"|;.*|[^\s()\[\]{}'"`,#;]*)(.*))RAW";
  OB::Text::Regex rx;
  while (rx.match(r, str).size() && rx.at(0).group.at(0).size()) {
    auto const& m = rx.at(0);
    tks.emplace_back(std::string(m.group.at(0)));
    str = m.group.at(1).data();
  }
  return tks;
}

Atm tok_atm(Tok const& tk) {
  try {
    if (tk.find(".") != Tok::npos) {
      return num_atm(Flo{tk});
    }
    if (tk.find("/") != Tok::npos) {
      auto v = Rat{tk};
      mpq_canonicalize(v.backend().data());
      if (numerator(v) == denominator(v) || denominator(v) == 1) {
        return num_atm(Int{numerator(v)});
      }
      return num_atm(v);
    }
    return num_atm(Int{tk});
  }
  catch (...) {
    return sym_atm(tk);
  }
}

std::optional<Xpr> tks_xpr(Tks& ts) {
  if (ts.empty()) {throw std::runtime_error("unexpected 'EOF'");}
  auto tk = ts.front(); ts.pop_front();
  if (paren_begin.find(tk) != paren_begin.end()) {
    auto const& p = paren_begin.at(tk);
    Xpr x;
    auto const l {xpr_lst(&x)};
    while (ts.front() != p) {
      if (auto const x = tks_xpr(ts)) {l->emplace_back(*x);}
    }
    ts.pop_front();
    return x;
  }
  else if (paren_end.find(tk) != paren_end.end()) {
    throw std::runtime_error("unexpected '" + tk + "'");
  }
  else if (tk.size() >= 2 && tk.front() == '"' && tk.back() == '"') {
    if (tk.size() == 2) {return str_xpr("");}
    return str_xpr(escape(tk.substr(1, tk.size() - 2)));
  }
  // else if (tk == "!") {
  //   if (auto const x = tks_xpr(ts)) {
  //     return Xpr{Lst{sym_xpr("eval"), *x}};
  //   }
  //   throw std::runtime_error("expected Xpr after '`'");
  // }
  else if (tk == "'") {
    if (auto const x = tks_xpr(ts)) {
      return Xpr{Lst{sym_xpr("quote"), *x}};
    }
    throw std::runtime_error("expected Xpr after '\\\''");
  }
  else if (tk == "`") {
    if (auto const x = tks_xpr(ts)) {
      return Xpr{Lst{sym_xpr("template"), *x}};
    }
    throw std::runtime_error("expected Xpr after '`'");
  }
  else if (tk == ".") {
    if (auto const x = tks_xpr(ts)) {
      return Xpr{Lst{sym_xpr("unquote"), *x}};
    }
    throw std::runtime_error("expected Xpr after ','");
  }
  else if (tk == ",") {
    if (auto const x = tks_xpr(ts)) {
      return Xpr{Lst{sym_xpr("unquote-splice"), *x}};
    }
    throw std::runtime_error("expected Xpr after '#'");
  }
  else if (tk.front() == ';') {
    return {};
  }
  else {
    return Xpr{tok_atm(tk)};
  }
}

std::optional<Xpr> read(std::string_view& str) {
  auto tks = str_tks(str);
  return tks_xpr(tks);
}

std::optional<Xpr> read(std::string_view&& str) {
  auto tks = str_tks(str);
  return tks_xpr(tks);
}

std::size_t type(Xpr const& x) {
  if (auto const a = xpr_atm(&x)) {
    if (auto const s = atm_str(a)) {return Typ::Str;}
    if (auto const s = atm_sym(a)) {return Typ::Sym;}
    if (auto const n = atm_num(a)) {
      if (auto const v = num_int(n)) {return Typ::Int;}
      if (auto const v = num_rat(n)) {return Typ::Rat;}
      if (auto const v = num_flo(n)) {return Typ::Flo;}
    }
  }
  if (auto const a = xpr_fun(&x)) {
    return Typ::Fun;
  }
  if (auto const a = xpr_lst(&x)) {
    return Typ::Lst;
  }
  return Typ::Ukn;
}

template<typename T> std::string print(T const& t) {
  std::ostringstream os;
  os << "[";
  for_each(t,
    [&](auto const& e) {os << "'" << e << "', ";},
    [&](auto const& e) {os << "'" << e << "'";});
  os << "]\n";
  return os.str();
}

std::string show(Xpr const& x) {
  std::ostringstream os;
  std::function<void(Xpr const&)> const to_string_impl = [&](Xpr const& x) {
    if (auto const a = xpr_atm(&x)) {
      if (auto const s = atm_str(a)) {os << unescape(s->str());}
      else if (auto const s = atm_sym(a)) {os << *s;}
      else if (auto const n = atm_num(a)) {
        if (auto const v = num_int(n)) {os << *v;}
        else if (auto const v = num_rat(n)) {os << *v;}
        else if (auto const v = num_flo(n)) {os << *v;}
      }
    }
    else if (auto const l = xpr_lst(&x)) {
      if (l->size() == 2) {
        if (auto const s = xpr_sym(&l->front())) {
          // if (*s == "eval") {
          //   os << "!";
          //   to_string_impl(l->back());
          //   return;
          // }
          if (*s == "quote") {
            os << "'";
            to_string_impl(l->back());
            return;
          }
          if (*s == "template") {
            os << "`";
            to_string_impl(l->back());
            return;
          }
          if (*s == "unquote") {
            os << ".";
            to_string_impl(l->back());
            return;
          }
          if (*s == "unquote-splice") {
            os << ",";
            to_string_impl(l->back());
            return;
          }
        }
      }
      os << "(";
      for_each(*l,
        [&](auto const& e) {to_string_impl(e); os << " ";},
        [&](auto const& e) {to_string_impl(e);});
      os << ")";
    }
    else if (auto const f = xpr_fun(&x)) {
      os << "#<Fn>";
    }
  };
  to_string_impl(x);
  return os.str();
}

std::string print(Xpr const& x) {
  std::ostringstream os;
  std::function<void(Xpr const&)> const to_string_impl = [&](Xpr const& x) {
    if (auto const a = xpr_atm(&x)) {
      if (auto const s = atm_str(a)) {os << "\"" << s->str() << "\"";}
      else if (auto const s = atm_sym(a)) {os << *s;}
      else if (auto const n = atm_num(a)) {
        if (auto const v = num_int(n)) {os << *v;}
        else if (auto const v = num_rat(n)) {os << *v;}
        else if (auto const v = num_flo(n)) {os << *v;}
      }
    }
    else if (auto const l = xpr_lst(&x)) {
      if (l->size() == 2) {
        if (auto const s = xpr_sym(&l->front())) {
          // if (*s == "eval") {
          //   os << "!";
          //   to_string_impl(l->back());
          //   return;
          // }
          if (*s == "quote") {
            os << "'";
            to_string_impl(l->back());
            return;
          }
          if (*s == "template") {
            os << "`";
            to_string_impl(l->back());
            return;
          }
          if (*s == "unquote") {
            os << ".";
            to_string_impl(l->back());
            return;
          }
          if (*s == "unquote-splice") {
            os << ",";
            to_string_impl(l->back());
            return;
          }
        }
      }
      os << "(";
      for_each(*l,
        [&](auto const& e) {to_string_impl(e); os << " ";},
        [&](auto const& e) {to_string_impl(e);});
      os << ")";
    }
    else if (auto const f = xpr_fun(&x)) {
      os << "#<Fn>";
    }
  };
  to_string_impl(x);
  return os.str();
}

std::string cprint(Xpr const& x) {
  std::ostringstream os;
  std::function<void(Xpr const&)> const to_string_impl = [&](Xpr const& x) {
    if (auto const a = xpr_atm(&x)) {
      if (auto const s = atm_str(a)) {os << aec::fg_green << "\"" << s->str() << "\"" << aec::clear;}
      else if (auto const s = atm_sym(a)) {os << aec::fg_magenta << *s << aec::clear;}
      else if (auto const n = atm_num(a)) {
        if (auto const v = num_int(n)) {os << aec::fg_yellow << *v << aec::clear;}
        else if (auto const v = num_rat(n)) {os << aec::fg_yellow << numerator(*v) << aec::fg_white << "/" << aec::fg_yellow << denominator(*v) << aec::clear;}
        else if (auto const v = num_flo(n)) {os << aec::fg_yellow << *v << aec::clear;}
      }
    }
    else if (auto const l = xpr_lst(&x)) {
      if (l->size() == 2) {
        if (auto const s = xpr_sym(&l->front())) {
          // if (*s == "eval") {
          //   os << aec::fg_cyan << "!" << aec::clear;
          //   to_string_impl(l->back());
          //   return;
          // }
          if (*s == "quote") {
            os << aec::fg_cyan << "'" << aec::clear;
            to_string_impl(l->back());
            return;
          }
          if (*s == "template") {
            os << aec::fg_cyan << "`" << aec::clear;
            to_string_impl(l->back());
            return;
          }
          if (*s == "unquote") {
            os << aec::fg_cyan << "." << aec::clear;
            to_string_impl(l->back());
            return;
          }
          if (*s == "unquote-splice") {
            os << aec::fg_cyan << "," << aec::clear;
            to_string_impl(l->back());
            return;
          }
        }
      }
      os << aec::fg_white << "(" << aec::clear;
      for_each(*l,
        [&](auto const& e) {to_string_impl(e); os << " ";},
        [&](auto const& e) {to_string_impl(e);});
      os << aec::fg_white << ")" << aec::clear;
    }
    else if (auto const f = xpr_fun(&x)) {
      os << aec::fg_blue << "#<Fn>" << aec::clear;
    }
  };
  to_string_impl(x);
  return os.str();
}

bool find_sym(Xpr const& xpr, Sym const& sym) {
  std::function<bool(Xpr const&)> const impl = [&](Xpr const& x) {
    bool res {false};
    if (auto const a = xpr_atm(&x)) {
      if (auto const s = atm_sym(a)) {
        // std::cerr << "DBG> compare " << *s << " to " << sym << "\n";
        if (*s == sym) {
          res = true;
        }
      }
      // else if (auto const s = atm_str(a)) {}
      // else if (auto const n = atm_num(a)) {
      //   if (auto const v = num_int(n)) {}
      //   else if (auto const v = num_rat(n)) {}
      //   else if (auto const v = num_flo(n)) {}
      // }
    }
    else if (auto const l = xpr_lst(&x)) {
      for (auto const& e : *l) {
        if ((res = impl(e))) {break;}
      }
    }
    // else if (auto const f = xpr_fun(&x)) {}
    return res;
  };
  return impl(xpr);
}

void resolve_sym(Xpr& xpr, Sym const& sym, Xpr const& rpl) {
  std::function<void(Xpr&)> const impl = [&](Xpr& x) {
    if (auto const a = xpr_atm(&x)) {
      if (auto const s = atm_sym(a)) {
        if (*s == sym) {
          // std::cerr << "DBG> resolve " << cprint(x) << " -> " << cprint(rpl) << "\n";
          x = rpl;
        }
      }
      // else if (auto const s = atm_str(a)) {}
      // else if (auto const n = atm_num(a)) {
      //   if (auto const v = num_int(n)) {}
      //   else if (auto const v = num_rat(n)) {}
      //   else if (auto const v = num_flo(n)) {}
      // }
    }
    else if (auto const l = xpr_lst(&x)) {
      for (auto& e : *l) {impl(e);}
    }
    // else if (auto const f = xpr_fun(&x)) {}
  };
  impl(xpr);
}

Xpr eval(Xpr&& xr, std::shared_ptr<Env> ev) {
  // return eval_impl(xr, ev);
  try {
    auto res = eval_impl(xr, ev);
    // std::cerr << aec::fg_magenta << "> " << cprint(xr) << " -> " << cprint(res) << aec::clear << "\n";
    return res;
  }
  catch (...) {
    // std::cerr << aec::fg_red << "> " << aec::clear << cprint(xr) << "\n";
    throw;
  }
  // catch (std::exception const& e) {
  //   std::cerr << aec::fg_red << "> " << aec::clear << cprint(xr) << "\n";
  //   std::cout << aec::fg_red << "> " << aec::clear << cprint(*read("(err \""s + e.what() + "\")"s)) << "\n";
  //   std::string input;
  //   std::string prompt {aec::fg_red + "> " + aec::clear};
  //   std::cout << prompt;
  //   while (std::getline(std::cin, input)) {
  //     if (input.size()) {
  //       if (input == ":b") {throw;}
  //       if (input == ":c") {break;}
  //       if (auto x = read(input)) {
  //         std::cout << aec::fg_magenta << "< " << aec::clear << cprint(*x) << "\n";
  //         auto v = eval(*x, ev);
  //         std::cout << aec::fg_green << "> " << aec::clear << cprint(v) << "\n";
  //       }
  //     }
  //     std::cout << prompt;
  //   }
  //   return eval(xr, ev);
  // }
}

Xpr eval(Xpr& xr, std::shared_ptr<Env> ev) {
  // return eval_impl(xr, ev);
  try {
    auto res = eval_impl(xr, ev);
    // std::cerr << aec::fg_magenta << "> " << cprint(xr) << " -> " << cprint(res) << aec::clear << "\n";
    return res;
  }
  catch (...) {
    // std::cerr << aec::fg_red << "> " << aec::clear << cprint(xr) << "\n";
    throw;
  }
  // catch (std::exception const& e) {
  //   std::cerr << aec::fg_red << "> " << aec::clear << cprint(xr) << "\n";
  //   std::cout << aec::fg_red << "> " << aec::clear << cprint(*read("(err \""s + e.what() + "\")"s)) << "\n";
  //   std::string input;
  //   std::string prompt {aec::fg_red + "> " + aec::clear};
  //   std::cout << prompt;
  //   while (std::getline(std::cin, input)) {
  //     if (input.size()) {
  //       if (input == ":b") {throw;}
  //       if (input == ":c") {break;}
  //       if (auto x = read(input)) {
  //         std::cout << aec::fg_magenta << "< " << aec::clear << cprint(*x) << "\n";
  //         auto v = eval(*x, ev);
  //         std::cout << aec::fg_green << "> " << aec::clear << cprint(v) << "\n";
  //       }
  //     }
  //     std::cout << prompt;
  //   }
  //   return eval(xr, ev);
  // }
}

Xpr eval_impl(Xpr& xr, std::shared_ptr<Env> ev) {
  if (auto const a = xpr_atm(&xr)) {
    if (auto const s = atm_str(a)) {return str_xpr(*s);}
    if (auto const s = atm_sym(a)) {
      if (auto p = ev->find(*s)) {
        auto& v = (*p)->second;
        if (!(v.ctx & Val::evaled)) {
          v.xpr = eval(v.xpr, v.env);
          v.ctx |= Val::evaled;
        }
        return v.xpr;
      }
      throw std::runtime_error("unbound symbol '" + *s + "'");
    }
    if (auto const n = atm_num(a)) {
      if (auto const p = num_int(n)) {return num_xpr(*p);}
      if (auto const p = num_rat(n)) {return num_xpr(*p);}
      if (auto const p = num_flo(n)) {return num_xpr(*p);}
    }
    throw std::runtime_error("unknown atom");
  }
  if (auto const l = xpr_lst(&xr)) {
    if (l->empty()) {return xr;}
    auto fn = [&]() {
      if (auto const v = xpr_lst(&l->front())) {
        return eval(l->front(), ev);
      }
      return l->front();
    }();
    if (auto const f = xpr_fun(&fn)) {
      Xpr args {*l};
      auto& l = std::get<Lst>(args);
      l.pop_front();
      Xpr res {f->fn(f->bind({"#<Fn>"}, l, ev))};
      return res;
    }
    if (auto const i = xpr_int(&fn)) {
      // TODO make sure index is > 0 && < len
      if (l->size() != 2) {throw std::runtime_error("'int' expected '1' argument");}
      Xpr v {eval(*std::next(l->begin(), 1), ev)};
      if (auto const l = xpr_lst(&v)) {
        if (*i >= l->size()) {throw std::runtime_error("'int' is out of range '" + std::to_string(static_cast<std::size_t>(*i)) + "' >= '" + std::to_string(l->size()) + "'");}
        return *std::next(l->begin(), static_cast<Lst::difference_type>(*i));
      }
      if (auto const s = xpr_str(&v)) {
        if (*i >= s->size()) {throw std::runtime_error("'int' is out of range '" + std::to_string(static_cast<std::size_t>(*i)) + "' >= '" + std::to_string(s->size()) + "'");}
        return str_xpr(s->at(static_cast<std::size_t>(*i)));
      }
      throw std::runtime_error("'int' expected '1' argument of type 'list'");
    }
    if (auto const a = xpr_atm(&fn)) {
      if (auto const s = atm_sym(a)) {
        if (*s == "@") {
          if (l->size() > 2) {throw std::runtime_error("'@' expected '1' argument of type 'list'");}
          auto x = eval(l->back(), ev);
          if (auto const v = xpr_lst(&x)) {
            if (v->size() < 2) {return Xpr{};}
            return (v->pop_front(), x);
          }
          if (auto const v = xpr_str(&x)) {
            if (v->size() < 2) {return Xpr{};}
            return (*v = v->substr(1), x);
          }
          throw std::runtime_error("invalid type '" + typ_str.at(type(x)) + "'");
        }
        Xpr v {sym_xpr(*s)};
        Xpr func {eval(v, ev)};
        if (auto const i = xpr_int(&func)) {
          if (l->size() != 2) {throw std::runtime_error("'int' expected '1' argument");}
          Xpr v {eval(*std::next(l->begin(), 1), ev)};
          if (auto const l = xpr_lst(&v)) {
            if (*i >= l->size()) {throw std::runtime_error("'int' is out of range '" + std::to_string(static_cast<std::size_t>(*i)) + "' >= '" + std::to_string(l->size()) + "'");}
            return *std::next(l->begin(), static_cast<Lst::difference_type>(*i));
          }
          if (auto const s = xpr_str(&v)) {
            if (*i >= s->size()) {throw std::runtime_error("'int' is out of range '" + std::to_string(static_cast<std::size_t>(*i)) + "' >= '" + std::to_string(s->size()) + "'");}
            return str_xpr(s->at(static_cast<std::size_t>(*i)));
          }
          throw std::runtime_error("'int' expected '1' argument of type 'list'");
        }
        else if (auto const f = xpr_fun(&func)) {
          Xpr args {*l};
          auto& l = std::get<Lst>(args);
          l.pop_front();
          Xpr res {f->fn(f->bind(*s, l, ev))};
          return res;

          // TODO add function memoization

          // auto xp = print(args);
          // if (auto m = f->memo->find(xp); m != f->memo->end()) {
          //   std::cerr << "cache> " << print(*l->begin()) << " | " << xp << " -> " << print(m->second) << "\n";
          //   return m->second;
          // }
          // auto xp = print(xr);
          // if (auto m = ev->memo->find(xp); m != ev->memo->end()) {
          //   // std::cerr << "cache> " << print(*l->begin()) << " | " << xp << " -> " << print(m->second) << "\n";
          //   return m->second;
          // }
          // Xpr res {f->fn(args, ev)};
          // (*f->memo)[xp] = res;
          // std::cerr << "memo> " << print(*l->begin()) << " | " << xp << " -> " << print(res) << "\n";
          // (*ev->memo)[xp] = res;
          // ev->dump_inner();
          // std::cerr << "memo> " << print(*l->begin()) << " | " << xp << " -> " << print(res) << "\n";
          // return res;
        }
      }
    }
    throw std::runtime_error("unknown 'Fn' '" + print(fn) + "'");
  }
  throw std::runtime_error("invalid eval");
}

// TODO handle comparisons of lists, strings, ...
// TODO overload + for adding lists and strings
void env_init(std::shared_ptr<Env> ev, int argc, char** argv) {
  auto constexpr builtin {Val::evaled};

  auto const cmp = [](Xpr& lhs, Xpr& rhs, auto fn) -> bool {
    if (auto const a = xpr_num(&lhs)) {
      if (auto const b = xpr_num(&rhs)) {
        if (a->index() == b->index()) {
          if (auto const v = num_int(a)) {return fn(*v, std::get<Int>(*b));}
          if (auto const v = num_rat(a)) {return fn(*v, std::get<Rat>(*b));}
          if (auto const v = num_flo(a)) {return fn(*v, std::get<Flo>(*b));}
        }
        if (auto const v1 = num_int(a)) {
          if (auto const v2 = num_rat(b)) {return fn(Rat{*v1}, *v2);}
          if (auto const v2 = num_flo(b)) {return fn(Flo{*v1}, *v2);}
        }
        if (auto const v1 = num_rat(a)) {
          if (auto const v2 = num_int(b)) {return fn(*v1, Rat{*v2});}
          if (auto const v2 = num_flo(b)) {return fn(Flo{*v1}, *v2);}
        }
        if (auto const v1 = num_flo(a)) {
          if (auto const v2 = num_int(b)) {return fn(*v1, Flo{*v2});}
          if (auto const v2 = num_rat(b)) {return fn(*v1, Flo{*v2});}
        }
      }
    }
    if (auto const a = xpr_str(&lhs)) {
      if (auto const b = xpr_str(&rhs)) {
        return fn(a->str(), b->str());
      }
    }
    if (auto const a = xpr_sym(&lhs)) {
      if (auto const b = xpr_sym(&rhs)) {
        return fn(*a, *b);
      }
    }
    throw std::runtime_error("invalid comparison of types '" + typ_str.at(type(lhs)) + "' and '" + typ_str.at(type(rhs)) + "'");
  };

  auto const math = [](auto const a, auto const b, auto fn) -> Num {
    if (a->index() == b->index()) {
      if (auto const v = num_int(a)) {return Num{static_cast<Int>(fn(*v, std::get<Int>(*b)))};}
      if (auto const v = num_rat(a)) {return Num{static_cast<Rat>(fn(*v, std::get<Rat>(*b)))};}
      if (auto const v = num_flo(a)) {return Num{static_cast<Flo>(fn(*v, std::get<Flo>(*b)))};}
    }
    if (auto const v1 = num_int(a)) {
      if (auto const v2 = num_rat(b)) {return Num{static_cast<Rat>(fn(Rat{*v1}, *v2))};}
      if (auto const v2 = num_flo(b)) {return Num{static_cast<Flo>(fn(Flo{*v1}, *v2))};}
    }
    if (auto const v1 = num_rat(a)) {
      if (auto const v2 = num_int(b)) {return Num{static_cast<Rat>(fn(*v1, Rat{*v2}))};}
      if (auto const v2 = num_flo(b)) {return Num{static_cast<Flo>(fn(Flo{*v1}, *v2))};}
    }
    if (auto const v1 = num_flo(a)) {
      if (auto const v2 = num_int(b)) {return Num{static_cast<Flo>(fn(*v1, Flo{*v2}))};}
      if (auto const v2 = num_rat(b)) {return Num{static_cast<Flo>(fn(*v1, Flo{*v2}))};}
    }
    throw std::runtime_error("invalid 'Num'");
  };

  auto const num_normalize = [](Num& n) -> Xpr {
    auto s = print(num_xpr(n));
    if (s.find(".") != Tok::npos) {
      return num_xpr(Flo{s});
    }
    if (s.find("/") != Tok::npos) {
      auto v = Rat{s};
      mpq_canonicalize(v.backend().data());
      if (numerator(v) == denominator(v) || denominator(v) == 1) {
        return num_xpr(Int{numerator(v)});
      }
      return num_xpr(v);
    }
    return num_xpr(Int{s});
  };

  (*ev)["Int"] = Val{sym_xpr("Int"), ev, builtin};
  (*ev)["Rat"] = Val{sym_xpr("Rat"), ev, builtin};
  (*ev)["Flo"] = Val{sym_xpr("Flo"), ev, builtin};
  (*ev)["Num"] = Val{sym_xpr("Flo"), ev, builtin};
  (*ev)["Sym"] = Val{sym_xpr("Sym"), ev, builtin};
  (*ev)["Str"] = Val{sym_xpr("Str"), ev, builtin};
  (*ev)["Atm"] = Val{sym_xpr("Atm"), ev, builtin};
  (*ev)["Fun"] = Val{sym_xpr("Fun"), ev, builtin};
  (*ev)["Lst"] = Val{sym_xpr("Lst"), ev, builtin};
  (*ev)["Xpr"] = Val{sym_xpr("Xpr"), ev, builtin};

  (*ev)["T"] = Val{sym_xpr("T"), ev, builtin};
  (*ev)["F"] = Val{sym_xpr("F"), ev, builtin};

  (*ev)["@"] = Val{([&]() {
    Xpr args;
    auto& l = std::get<Lst>(args);
    for (int i = 0; i < argc; ++i) {
      l.emplace_back(str_xpr(std::string(argv[i])));
    }
    return args;
  }()), ev, builtin};

  (*ev)["??"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
    return sym_xpr(typ_str.at(type(eval(sym_xpr("a"), e))));
  }}, ev, builtin};

  (*ev)["!!"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
    auto a = eval(sym_xpr("a"), e);
    auto const v = xpr_sym(&a);
    if (v && *v == "F") {return sym_xpr("T");}
    return sym_xpr("F");
  }}, ev, builtin};

  (*ev)["&&"] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
    auto a = eval(sym_xpr("a"), e);
    auto const lhs = xpr_sym(&a);
    if (lhs && *lhs == "F") {return sym_xpr("F");}
    auto b = eval(sym_xpr("b"), e);
    auto const rhs = xpr_sym(&b);
    if (rhs && *rhs == "F") {return sym_xpr("F");}
    return b;
  }}, ev, builtin};

  (*ev)["||"] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
    auto a = eval(sym_xpr("a"), e);
    auto const lhs = xpr_sym(&a);
    if (!lhs || *lhs != "F") {return a;}
    auto b = eval(sym_xpr("b"), e);
    auto const rhs = xpr_sym(&b);
    if (!rhs || *rhs != "F") {return b;}
    return sym_xpr("F");
  }}, ev, builtin};

  (*ev)["=="] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
    auto a = eval(sym_xpr("a"), e);
    auto b = eval(sym_xpr("b"), e);
    return cmp(a, b, [](auto const lhs, auto const rhs) {return lhs == rhs;}) ? sym_xpr("T") : sym_xpr("F");
  }}, ev, builtin};

  (*ev)["!="] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
    auto a = eval(sym_xpr("a"), e);
    auto b = eval(sym_xpr("b"), e);
    return cmp(a, b, [](auto const lhs, auto const rhs) {return lhs != rhs;}) ? sym_xpr("T") : sym_xpr("F");
  }}, ev, builtin};

  (*ev)["<"] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
    auto a = eval(sym_xpr("a"), e);
    auto b = eval(sym_xpr("b"), e);
    return cmp(a, b, [](auto const lhs, auto const rhs) {return lhs < rhs;}) ? sym_xpr("T") : sym_xpr("F");
  }}, ev, builtin};

  (*ev)["<="] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
    auto a = eval(sym_xpr("a"), e);
    auto b = eval(sym_xpr("b"), e);
    return cmp(a, b, [](auto const lhs, auto const rhs) {return lhs <= rhs;}) ? sym_xpr("T") : sym_xpr("F");
  }}, ev, builtin};

  (*ev)[">"] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
    auto a = eval(sym_xpr("a"), e);
    auto b = eval(sym_xpr("b"), e);
    return cmp(a, b, [](auto const lhs, auto const rhs) {return lhs > rhs;}) ? sym_xpr("T") : sym_xpr("F");
  }}, ev, builtin};

  (*ev)[">="] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
    auto a = eval(sym_xpr("a"), e);
    auto b = eval(sym_xpr("b"), e);
    return cmp(a, b, [](auto const lhs, auto const rhs) {return lhs >= rhs;}) ? sym_xpr("T") : sym_xpr("F");
  }}, ev, builtin};

  (*ev)["*"] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
    auto a = eval(sym_xpr("a"), e);
    auto b = eval(sym_xpr("b"), e);
    if (auto const lhs = xpr_num(&a)) {
      if (auto const rhs = xpr_num(&b)) {
        auto n = math(lhs, rhs, [](auto const a, auto const b) {return a * b;});
        return num_normalize(n);
      }
    }
    else if (auto const lhs = xpr_str(&a)) {
      if (auto const rhs = xpr_int(&b)) {return str_xpr(repeat(static_cast<std::size_t>(*rhs), lhs->str()));}
    }
    throw std::runtime_error("invalid types '" + typ_str.at(type(a)) + "' and '" + typ_str.at(type(b)) + "'");
  }}, ev, builtin};

  (*ev)["/"] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
    auto a = eval(sym_xpr("a"), e);
    auto b = eval(sym_xpr("b"), e);
    if (auto const lhs = xpr_int(&a)) {
      if (auto const rhs = xpr_int(&b)) {
        auto n = Num{Rat{*lhs, *rhs}};
        return num_normalize(n);
      }
    }
    if (auto const lhs = xpr_num(&a)) {
      if (auto const rhs = xpr_num(&b)) {
        auto n = math(lhs, rhs, [](auto const a, auto const b) {return a / b;});
        return num_normalize(n);
      }
    }
    throw std::runtime_error("invalid types '" + typ_str.at(type(a)) + "' and '" + typ_str.at(type(b)) + "'");
  }}, ev, builtin};

  (*ev)["+"] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
    auto a = eval(sym_xpr("a"), e);
    auto b = eval(sym_xpr("b"), e);
    if (auto const lhs = xpr_num(&a)) {
      if (auto const rhs = xpr_num(&b)) {
        auto n = math(lhs, rhs, [](auto const a, auto const b) {return a + b;});
        return num_normalize(n);
      }
    }
    else if (auto const lhs = xpr_str(&a)) {
      if (auto const rhs = xpr_str(&b)) {lhs->append(rhs->str()); return a;}
      if (auto const rhs = xpr_num(&b)) {lhs->append(print(b)); return a;}
      if (auto const rhs = xpr_sym(&b)) {lhs->append(print(b)); return a;}
      if (auto const rhs = xpr_lst(&b)) {lhs->append(print(b)); return a;}
    }
    else if (auto const lhs = xpr_lst(&a)) {
      if (auto const rhs = xpr_lst(&b)) {lhs->splice(lhs->end(), *rhs); return a;}
      if (auto const rhs = xpr_num(&b)) {lhs->emplace_back(b); return a;}
      if (auto const rhs = xpr_sym(&b)) {lhs->emplace_back(b); return a;}
      if (auto const rhs = xpr_str(&b)) {lhs->emplace_back(b); return a;}
    }
    throw std::runtime_error("invalid types '" + typ_str.at(type(a)) + "' and '" + typ_str.at(type(b)) + "'");
  }}, ev, builtin};

  (*ev)["-"] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
    auto a = eval(sym_xpr("a"), e);
    auto b = eval(sym_xpr("b"), e);
    if (auto const lhs = xpr_num(&a)) {
      if (auto const rhs = xpr_num(&b)) {
        auto n = math(lhs, rhs, [](auto const a, auto const b) {return a - b;});
        return num_normalize(n);
      }
    }
    throw std::runtime_error("invalid types '" + typ_str.at(type(a)) + "' and '" + typ_str.at(type(b)) + "'");
  }}, ev, builtin};

  (*ev)["%"] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
    auto a = eval(sym_xpr("a"), e);
    auto const lhs = xpr_int(&a);
    if (!lhs) {throw std::runtime_error("invalid type '" + typ_str.at(type(a)) + "'");}
    auto b = eval(sym_xpr("b"), e);
    auto const rhs = xpr_int(&b);
    if (!rhs) {throw std::runtime_error("invalid type '" + typ_str.at(type(b)) + "'");}
    return num_xpr(static_cast<Int>(*lhs % *rhs));
  }}, ev, builtin};

  // TODO add type functions
  // int, rat, flo, num, sym, str, fun, lst

  // (*ev)["read"] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
  //   auto a = eval(sym_xpr("a"), e);
  //   auto b = eval(sym_xpr("b"), e);
  //   throw std::runtime_error("invalid types '" + typ_str.at(type(a)) + "' and '" + typ_str.at(type(b)) + "'");
  // }}, ev, builtin};

  (*ev)["try"] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
    try {
      return eval(sym_xpr("a"), e);
    }
    catch (std::exception const& err) {
      auto b = eval(sym_xpr("b"), e);
      if (auto const lhs = xpr_fun(&b)) {
        Xpr x;
        auto& l = std::get<Lst>(x);
        l.emplace_back(b);
        l.emplace_back(str_xpr(err.what()));
        return eval(x, e);
      }
      throw std::runtime_error("invalid type '" + typ_str.at(type(b)) + "', expected 'Fn'");
    }
  }}, ev, builtin};

  (*ev)["throw"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
    auto a = eval(sym_xpr("a"), e);
    throw std::runtime_error(show(a));
  }}, ev, builtin};

  (*ev)["eval"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
    return eval(sym_xpr("a"), e);

    // auto a = eval(sym_xpr("a"), e);
    // return eval(a, e->current);

    // return eval((*e->find_inner("a"))->second.xpr, e->current);
  }}, ev, builtin};

  (*ev)["apply"] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
    auto a = eval(sym_xpr("a"), e);
    auto b = eval(sym_xpr("b"), e);
    if (auto const lhs = xpr_fun(&a)) {
      if (auto const rhs = xpr_lst(&b)) {
        rhs->emplace_front(a);
        return eval(b, e->current);
      }
    }
    throw std::runtime_error("invalid types '" + typ_str.at(type(a)) + "' and '" + typ_str.at(type(b)) + "'");
  }}, ev, builtin};

  (*ev)["map"] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
    auto a = eval(sym_xpr("a"), e);
    auto b = eval(sym_xpr("b"), e);
    if (auto const lhs = xpr_fun(&a)) {
      if (auto const rhs = xpr_lst(&b)) {
        for (auto& v : *rhs) {
          Xpr x;
          auto& l = std::get<Lst>(x);
          l.emplace_back(a);
          l.emplace_back(v);
          v = eval(x, e->current);
        }
        return b;
      }
      if (auto const rhs = xpr_str(&b)) {
        std::size_t i {0};
        for (auto& v : *rhs) {
          Xpr x;
          auto& l = std::get<Lst>(x);
          l.emplace_back(a);
          l.emplace_back(str_xpr(v));
          auto c = eval(x, e->current);
          if (auto const n = xpr_str(&c)) {
            if (rhs->str() != n->str()) {
              rhs->replace(i, 1, n->str());
            }
          }
          else {
            throw std::runtime_error("invalid type '" + typ_str.at(type(c)) + "', expected 'Str'");
          }
          ++i;
        }
        return b;
      }
    }
    throw std::runtime_error("invalid types '" + typ_str.at(type(a)) + "' and '" + typ_str.at(type(b)) + "'");
  }}, ev, builtin};

  (*ev)["filter"] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
    auto a = eval(sym_xpr("a"), e);
    auto b = eval(sym_xpr("b"), e);
    if (auto const lhs = xpr_fun(&a)) {
      if (auto const rhs = xpr_lst(&b)) {
        for (auto it = rhs->begin(); it != rhs->end();) {
          Xpr x;
          auto& l = std::get<Lst>(x);
          l.emplace_back(a);
          l.emplace_back(*it);
          auto v = eval(x, e->current);
          if (auto const s = xpr_sym(&v)) {
            if (*s == "F") {++it; continue;}
            if (*s == "T") {it = rhs->erase(it); continue;}
            throw std::runtime_error("invalid type '" + typ_str.at(type(v)) + "' expected 'T' or 'F'");
          }
          throw std::runtime_error("invalid type '" + typ_str.at(type(v)) + "' expected 'Sym'");
        }
        return b;
      }
    }
    throw std::runtime_error("invalid types '" + typ_str.at(type(a)) + "' and '" + typ_str.at(type(b)) + "'");
  }}, ev, builtin};

  (*ev)["reduce"] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
    auto a = eval(sym_xpr("a"), e);
    auto b = eval(sym_xpr("b"), e);
    if (auto const lhs = xpr_fun(&a)) {
      if (auto const rhs = xpr_lst(&b)) {
        for (auto it = rhs->begin(); rhs->size() > 1; it = rhs->begin()) {
          Xpr x;
          auto& l = std::get<Lst>(x);
          l.emplace_back(a);
          l.emplace_back(*it);
          l.emplace_back(*++it);
          rhs->erase(rhs->begin(), std::next(rhs->begin(), 2));
          rhs->emplace_front(eval(x, e->current));
        }
        return rhs->front();
      }
    }
    throw std::runtime_error("invalid types '" + typ_str.at(type(a)) + "' and '" + typ_str.at(type(b)) + "'");
  }}, ev, builtin};

  (*ev)["env"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
    Xpr x;
    e->current->list(x);
    return x;
  }}, ev, builtin};

  // (*ev)["env"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
  //   e->current->dump();
  //   return sym_xpr("T");
  // }}, ev, builtin};

  // (*ev)["show"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
  //   e->current->dump_inner();
  //   return sym_xpr("T");
  // }}, ev, builtin};

  // (*ev)["<<"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
  //   std::cout << print(eval(sym_xpr("a"), e)) << "\n";
  //   return sym_xpr("T");
  // }}, ev, builtin};

  (*ev)["<<"] = Val{Fun{str_lst("(@)"), [&](auto e) -> Xpr {
    auto x = eval(sym_xpr("@"), e);
    auto& l = std::get<Lst>(x);
    for (auto it = l.begin(); it != l.end(); ++it) {
      std::cout << show(eval(*it, e->current));
    }
    std::cout << std::flush;
    return sym_xpr("T");
  }}, ev, builtin};

  (*ev)[">>"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
    std::string input;
    std::string prompt {aec::fg_magenta + "> " + aec::clear};
    if (std::getline(std::cin, input)) {
      if (input.size()) {
        try {if (auto x = read(input)) {return *x;}}
        catch (std::exception const& e) {
          std::cout << aec::fg_red << "> " << aec::clear << "\"" << e.what() << "\"" << "\n";
        }
      }
    }
    return sym_xpr("F");
  }}, ev, builtin};

  (*ev)["slp"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
    auto a = eval(sym_xpr("a"), e);
    if (auto const v = xpr_int(&a)) {
      std::this_thread::sleep_for(std::chrono::seconds(static_cast<long>(*v)));
      return sym_xpr("T");
    }
    throw std::runtime_error("expected 'Int'");
  }}, ev, builtin};

  (*ev)["len"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
    auto a = eval(sym_xpr("a"), e);
    if (auto const v = xpr_lst(&a)) {
      return num_xpr(static_cast<Int>(v->size()));
    }
    if (auto const v = xpr_str(&a)) {
      return num_xpr(static_cast<Int>(v->size()));
    }
    throw std::runtime_error("invalid type '" + typ_str.at(type(a)) + "'");
  }}, ev, builtin};

  (*ev)["if"] = Val{Fun{str_lst("(a b c)"), [&](auto e) -> Xpr {
    auto a = eval(sym_xpr("a"), e);
    if (auto const v = xpr_sym(&a)) {
      if (*v == "F") {return eval(sym_xpr("c"), e);}
    }
    else if (auto const v = xpr_lst(&a)) {
      if (v->empty()) {return eval(sym_xpr("c"), e);}
    }
    return eval(sym_xpr("b"), e);
  }}, ev, builtin};

  (*ev)["let"] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
    auto a = (*e->find_inner("a"))->second.xpr;
    // auto a = eval(sym_xpr("a"), e);
    if (auto const s = xpr_sym(&a)) {
      if (auto p = e->find_current_inner(*s)) {
        auto& v = (*p)->second;
        if (v.ctx & Val::constant) throw std::runtime_error("constant binding");
        // v = Val{(*e->find_inner("b"))->second.xpr, e->current, Val::constant};
        v = Val{eval(sym_xpr("b"), e), e->current, Val::constant | Val::evaled};
        // auto b = (*e->find_inner("b"))->second.xpr;
        // resolve_sym(b, *s, eval(a, e->current));
        // v = Val{b, e->current};
        return a;
      }
      // (*e->current)[*s] = Val{(*e->find_inner("b"))->second.xpr, e->current, Val::constant};
      (*e->current)[*s] = Val{eval(sym_xpr("b"), e), e->current, Val::constant | Val::evaled};
      return a;
    }
    throw std::runtime_error("expected symbol");
  }}, ev, builtin};

  (*ev)["var"] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
    auto a = (*e->find_inner("a"))->second.xpr;
    // auto a = eval(sym_xpr("a"), e);
    if (auto const s = xpr_sym(&a)) {
      if (auto p = e->find_current_inner(*s)) {
        auto& v = (*p)->second;
        if (v.ctx & Val::constant) throw std::runtime_error("constant binding");
        // v = Val{(*e->find_inner("b"))->second.xpr, e->current};
        v = Val{eval(sym_xpr("b"), e), e->current, Val::evaled};
        // auto b = (*e->find_inner("b"))->second.xpr;
        // resolve_sym(b, *s, eval(a, e->current));
        // v = Val{b, e->current};
        return a;
      }
      // (*e->current)[*s] = Val{(*e->find_inner("b"))->second.xpr, e->current};
      (*e->current)[*s] = Val{eval(sym_xpr("b"), e), e->current, Val::evaled};
      return a;
    }
    throw std::runtime_error("expected symbol");
  }}, ev, builtin};

  (*ev)["set"] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
    auto a = (*e->find_inner("a"))->second.xpr;
    // auto a = eval(sym_xpr("a"), e);
    if (auto const s = xpr_sym(&a)) {
      if (auto p = e->find_current(*s)) {
        auto& v = (*p)->second;
        if (v.ctx & Val::constant) throw std::runtime_error("constant binding");
        // v = Val{(*e->find_inner("b"))->second.xpr, e->current};
        // v = Val{(*e->find_inner("b"))->second.xpr, std::make_shared<Env>(v.env, e->current)};
        // v = Val{(*e->find_inner("b"))->second.xpr, v.env};
        // v = Val{eval(sym_xpr("b"), e), e->current};
        // v = Val{(*e->find_inner("b"))->second.xpr, e->current};
        v = Val{eval(sym_xpr("b"), e), e->current, Val::evaled};

        // auto b = (*e->find_inner("b"))->second.xpr;
        // auto const contains_self = find_sym(b, *s);
        // std::cerr << "DBG> self? " << std::boolalpha << contains_self << "\n";

        // auto b = (*e->find_inner("b"))->second.xpr;
        // resolve_sym(b, *s, eval(a, e->current));
        // v = Val{b, e->current};
        return a;
      }
      throw std::runtime_error("unbound symbol '" + *s + "'");
    }
    throw std::runtime_error("expected symbol");
  }}, ev, builtin};

  (*ev)["get"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
    auto a = (*e->find_inner("a"))->second.xpr;
    // auto a = eval(sym_xpr("a"), e);
    if (auto const s = xpr_sym(&a)) {
      if (auto p = e->find_current(*s)) {
        auto& v = (*p)->second;
        return v.xpr;
      }
      throw std::runtime_error("unbound symbol '" + *s + "'");
    }
    throw std::runtime_error("expected symbol");
  }}, ev, builtin};

  (*ev)["quote"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
    return (*e->find_inner("a"))->second.xpr;
  }}, ev, builtin};

  // TODO backquote comma splice
  // (*ev)["template"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
  //   //
  // }}, ev, builtin};

  // (*ev)["lst"] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
  //   auto a = eval(sym_xpr("a"), e);
  //   auto b = eval(sym_xpr("b"), e);
  //   if (auto const l = xpr_lst(&b)) {
  //     l->emplace_front(a);
  //     return b;
  //   }
  //   return Xpr{Lst{a, b}};
  // }}, ev, builtin};

  (*ev)["lst"] = Val{Fun{str_lst("(@)"), [&](auto e) -> Xpr {
    auto x = eval(sym_xpr("@"), e);
    auto& l = std::get<Lst>(x);
    for (auto it = l.begin(); it != l.end(); ++it) {
      *it = eval(*it, e->current);
    }
    return x;
  }}, ev, builtin};

  (*ev)["str?"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
    auto x = eval(sym_xpr("a"), e);
    if (auto const a = xpr_str(&x)) {
      return sym_xpr("T");
    }
    return sym_xpr("F");
  }}, ev, builtin};

  (*ev)["sym?"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
    auto x = eval(sym_xpr("a"), e);
    if (auto const a = xpr_sym(&x)) {
      return sym_xpr("T");
    }
    return sym_xpr("F");
  }}, ev, builtin};

  (*ev)["num?"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
    auto x = eval(sym_xpr("a"), e);
    if (auto const a = xpr_num(&x)) {
      return sym_xpr("T");
    }
    return sym_xpr("F");
  }}, ev, builtin};

  (*ev)["atm?"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
    auto x = eval(sym_xpr("a"), e);
    if (auto const a = xpr_atm(&x)) {
      return sym_xpr("T");
    }
    return sym_xpr("F");
  }}, ev, builtin};

  (*ev)["nul?"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
    auto x = eval(sym_xpr("a"), e);
    if (auto const l = xpr_lst(&x)) {
      if (l->empty()) {return sym_xpr("T");}
    }
    else if (auto const s = xpr_str(&x)) {
      if (s->empty()) {return sym_xpr("T");}
    }
    return sym_xpr("F");
  }}, ev, builtin};

  (*ev)["sys"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
    auto x = eval(sym_xpr("a"), e);
    if (auto const s = xpr_str(&x)) {
      std::cout << "\n";
      std::system(s->c_str());
      return sym_xpr("T");
    }
    throw std::runtime_error("invalid type '" + typ_str.at(type(x)) + "'");
  }}, ev, builtin};

  (*ev)["ln"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
    auto x = eval(sym_xpr("a"), e);
    if (auto const s = xpr_str(&x)) {
      std::ifstream file {s->str()};
      if (! file.is_open()) {throw std::runtime_error("could not open file '" + s->str() + "'");}
      std::string input;
      while (std::getline(file, input)) {
        if (input.size()) {
          if (auto x = read(input)) {
            eval(*x, e->current);
          }
        }
      }
      return sym_xpr("T");
    }
    throw std::runtime_error("invalid type '" + typ_str.at(type(x)) + "'");
  }}, ev, builtin};

  (*ev)["ld"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
    auto x = eval(sym_xpr("a"), e);
    if (auto const s = xpr_str(&x)) {
      std::ifstream file {s->str()};
      if (! file.is_open()) {throw std::runtime_error("could not open file '" + s->str() + "'");}
      std::string input;
      while (std::getline(file, input)) {
        if (input.size()) {
          if (auto x = read(input)) {
            std::cout << aec::fg_magenta << "< " << aec::clear << cprint(*x) << "\n";
            auto v = eval(*x, ev);
            std::cout << aec::fg_green << "> " << aec::clear << cprint(v) << "\n";
          }
        }
      }
      return sym_xpr("T");
    }
    throw std::runtime_error("invalid type '" + typ_str.at(type(x)) + "'");
  }}, ev, builtin};

  // (*ev)["case"] = Val{Fun{str_lst("(@)"), [&](auto e) -> Xpr {
  // }}, ev, builtin};

  // (*ev)["cond"] = Val{Fun{str_lst("(@)"), [&](auto e) -> Xpr {
  //   auto x = eval(sym_xpr("@"), e);
  //   auto& l = std::get<Lst>(x);
  //   Xpr res {sym_xpr("F")};
  //   bool t {true};
  //   while (t) {
  //     for (auto it = l.begin(); it != l.end(); ++it) {
  //       res = eval(*it, e->current);
  //       if (auto const s = xpr_sym(&res); s && *s == "F") {
  //         t = false; break;
  //       }
  //     }
  //   }
  //   return res;
  // }}, ev, builtin};

  // (*ev)["pl"] = Val{Fun{str_lst("(@)"), [&](auto e) -> Xpr {
  //   auto x = eval(sym_xpr("@"), e);
  //   auto& l = std::get<Lst>(x);
  //   Xpr res {sym_xpr("F")};
  //   if (l.empty()) {return res;}
  //   bool t {true};
  //   while (t) {
  //     for (auto it = l.begin(); it != l.end(); ++it) {
  //       res = eval(*it, e->current);
  //       if (auto const s = xpr_sym(&res); s && *s == "F") {
  //         t = false; break;
  //         std::cerr << "DBG> " << "F" << "\n";
  //       }
  //     }
  //   }
  //   return res;
  // }}, ev, builtin};

  // (*ev)["pr"] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
  //   auto a = eval(sym_xpr("a"), e);
  //   auto b = eval(sym_xpr("b"), e);
  //   Xpr res {sym_xpr("F")};
  //   if (auto const n = xpr_int(&a)) {
  //     for (std::size_t i = 0; i < static_cast<std::size_t>(*n); ++i) {
  //       res = eval(b, e->current);
  //     }
  //     return res;
  //   }
  //   throw std::runtime_error("expected 'Int'");
  // }}, ev, builtin};

  (*ev)["fmt"] = Val{Fun{str_lst("(a @)"), [&](auto e) -> Xpr {
    auto a = eval(sym_xpr("a"), e);
    if (auto const s = xpr_str(&a)) {
      auto x = eval(sym_xpr("@"), e);
      auto& l = std::get<Lst>(x);
      std::size_t pos {0};
      auto it = l.begin();
      for (; it != l.end(); ++it) {
        pos = s->find("~", pos);
        if (pos == Str::npos || pos + 1 == Str::npos) {break;}
        std::string val {s->at(pos + 1)};
        switch (val[0]) {
          case '~': {
            s->erase(pos++, 1);
            break;
          }
          case 's': {
            std::string nval {show(eval(*it, e->current))};
            s->replace(pos, val.size() + 1, nval);
            pos += nval.size();
            break;
          }
          default: {
            ++pos;
            break;
          }
        }
      }
      return a;
    }
    throw std::runtime_error("invalid type '" + typ_str.at(type(a)) + "'");
  }}, ev, builtin};

  (*ev)["do"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
    // auto x = eval(sym_xpr("a"), e);
    auto x = (*e->find_inner("a"))->second.xpr;
    Xpr res {sym_xpr("F")};
    // for (std::size_t i = 0; i < 1000; ++i) {
    for (;;) {
      res = eval(x, e->current);
      // std::cerr << "DBG> do " << cprint(res) << "\n";
      if (auto const s = xpr_sym(&res); s && *s == "F") {
        return res;
      }
    }
    // throw std::runtime_error("exceeded loop max");
  }}, ev, builtin};

  (*ev)["pn"] = Val{Fun{str_lst("(@)"), [&](auto e) -> Xpr {
    auto x = eval(sym_xpr("@"), e);
    auto& l = std::get<Lst>(x);
    Xpr res {sym_xpr("F")};
    for (auto it = l.begin(); it != l.end(); ++it) {
      res = eval(*it, e->current);
    }
    return res;
  }}, ev, builtin};

  // (*ev)["yield"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
  //   auto x = eval(sym_xpr("a"), e);
  //   auto x = (*e->find_inner("a"))->second.xpr;
  //   Xpr res {sym_xpr("F")};
  //   // for (std::size_t i = 0; i < 1000; ++i) {
  //   for (;;) {
  //     res = eval(x, e->current);
  //     // std::cerr << "DBG> do " << cprint(res) << "\n";
  //     if (auto const s = xpr_sym(&res); s && *s == "F") {
  //       return res;
  //     }
  //   }
  //   // throw std::runtime_error("exceeded loop max");
  // }}, ev, builtin};

  (*ev)["fn"] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
    return Xpr{Fun{std::get<Lst>((*e->find_inner("a"))->second.xpr), [body = (*e->find_inner("b"))->second.xpr](std::shared_ptr<Env> e) mutable -> Xpr {
      // return Xpr{Fun{std::get<Lst>(eval(sym_xpr("a"), e)), [body = eval(sym_xpr("b"), e)](std::shared_ptr<Env> e) mutable -> Xpr {
      return eval(body, e);
    }, e->current}};
  }}, ev, builtin};
}

void repl(int argc, char** argv) {
  auto ev = std::make_shared<Env>();
  env_init(ev, argc, argv);
  std::string input;
  std::string prompt {aec::fg_magenta + "> " + aec::clear};
  std::cout << "Welcome to lispp v0.2.0.\n" << prompt;
  while (std::getline(std::cin, input)) {
    if (input.size()) {
      try {
        if (auto x = read(input)) {
          std::cout << aec::fg_magenta << "< " << aec::clear << cprint(*x) << "\n";
          auto v = eval(*x, ev);
          std::cout << aec::fg_green << "> " << aec::clear << cprint(v) << "\n";
        }
      }
      catch (std::exception const& e) {
        std::cout << aec::fg_red << "> " << aec::clear << "\"" << e.what() << "\"" << "\n";
      }
    }
    std::cout << prompt;
  }
  std::cout << "\n";
}

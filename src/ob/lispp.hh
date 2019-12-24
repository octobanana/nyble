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

// TODO static link binary
// TODO add type system

#include "ob/term.hh"
#include "ob/text.hh"

// TODO find replacement with permissible licence for static linking
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

struct Typ;
extern std::vector<std::string> typ_str;
extern std::unordered_map<std::string, std::string> paren_begin;
extern std::unordered_map<std::string, std::string> paren_end;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using f32 = float;
using f64 = double;

using Int = boost::multiprecision::mpz_int;
using Rat = boost::multiprecision::mpq_rational;
using Flo = boost::multiprecision::mpfr_float;
using Num = std::variant<Int, Rat, Flo>;

using Sym = std::string;

using Str = OB::Text::String;

using Atm = std::variant<Num, Sym, Str>;

struct Xpr;

using Lst = std::list<Xpr>;

struct Env;

struct Fun {
  using Fn = std::function<Xpr(std::shared_ptr<Env>)>;
  Lst args {};
  Fn fn {nullptr};
  std::shared_ptr<Env> env {nullptr};
  std::optional<Lst::iterator> yield {std::nullopt};
  // std::shared_ptr<std::unordered_map<std::string, Xpr>> memo {std::make_shared<std::unordered_map<std::string, Xpr>>()};
  std::shared_ptr<Env> bind(Sym const& sym, Lst& l, std::shared_ptr<Env> e);
};

struct Xpr : std::variant<Lst, Fun, Atm> {};

struct Val {
  enum : u64 {
    nil = 0,
    evaled = 1<<0,
    constant = 1<<1,
    pure = 1<<2,
    lazy = 1<<3,
  };
  Xpr xpr;
  std::shared_ptr<Env> env {nullptr};
  u64 ctx {nil};
};

struct Env {
  using Inner = std::map<Sym, Val>;
  using Outer = std::shared_ptr<Env>;
  Inner inner {};
  Outer outer {nullptr};
  Outer current {nullptr};
  std::shared_ptr<std::unordered_map<std::string, Xpr>> memo {std::make_shared<std::unordered_map<std::string, Xpr>>()};
  Env(Outer outer = nullptr, Outer current = nullptr) : outer {outer}, current {current} {}
  Val& operator[](Sym const& sym);
  std::optional<Inner::iterator> find(Sym const& sym);
  std::optional<Inner::iterator> find_inner(Sym const& sym);
  std::optional<Inner::iterator> find_outer(Sym const& sym);
  std::optional<Inner::iterator> find_current(Sym const& sym);
  std::optional<Inner::iterator> find_current_inner(Sym const& sym);
  void dump();
  void dump_inner();
  void list(Xpr& x);
};

using Tok = std::string;
using Tks = std::deque<std::string>;

Tks str_tks(std::string_view& str);
Atm tok_atm(Tok const& tk);
std::optional<Xpr> tks_xpr(Tks& ts);
std::optional<Xpr> read(std::string_view& str);
std::optional<Xpr> read(std::string_view&& str);
std::size_t type(Xpr const& x);
template<typename T> std::string print(T const& t);
std::string show(Xpr const& x);
std::string print(Xpr const& x);
std::string cprint(Xpr const& x);
bool find_sym(Xpr const& xpr, Sym const& sym);
void resolve_sym(Xpr& xpr, Sym const& sym, Xpr const& rpl);
Xpr eval_impl(Xpr&, std::shared_ptr<Env>);
Xpr eval(Xpr& xr, std::shared_ptr<Env> ev);
Xpr eval(Xpr&& xr, std::shared_ptr<Env> ev);
void env_init(std::shared_ptr<Env> ev, int argc, char** argv);
void repl(int argc, char** argv);

#define xpr_atm(x) (std::get_if<Atm>(x))
#define xpr_num(x) (std::get_if<Num>(std::get_if<Atm>(x)))
#define xpr_int(x) (std::get_if<Int>(std::get_if<Num>(std::get_if<Atm>(x))))
#define xpr_rat(x) (std::get_if<Rat>(std::get_if<Num>(std::get_if<Atm>(x))))
#define xpr_flo(x) (std::get_if<Flo>(std::get_if<Num>(std::get_if<Atm>(x))))
#define xpr_sym(x) (std::get_if<Sym>(std::get_if<Atm>(x)))
#define xpr_str(x) (std::get_if<Str>(std::get_if<Atm>(x)))
#define xpr_lst(x) (std::get_if<Lst>(x))
#define xpr_fun(x) (std::get_if<Fun>(x))
#define atm_num(x) (std::get_if<Num>(x))
#define atm_sym(x) (std::get_if<Sym>(x))
#define atm_str(x) (std::get_if<Str>(x))
#define atm_int(x) (std::get_if<Int>(std::get_if<Num>(x)))
#define atm_rat(x) (std::get_if<Rat>(std::get_if<Num>(x)))
#define atm_flo(x) (std::get_if<Flo>(std::get_if<Num>(x)))
#define num_int(x) (std::get_if<Int>(x))
#define num_rat(x) (std::get_if<Rat>(x))
#define num_flo(x) (std::get_if<Flo>(x))
#define fun_xpr(x) (Xpr{Fun{(x)}})
#define num_atm(x) (Atm{Num{(x)}})
#define num_xpr(x) (Xpr{Atm{Num{(x)}}})
#define sym_atm(x) (Atm{Sym{(x)}})
#define sym_xpr(x) (Xpr{Atm{Sym{(x)}}})
#define str_atm(x) (Atm{Str{(x)}})
#define str_xpr(x) (Xpr{Atm{Str{(x)}}})
#define str_lst(x) std::get<Lst>(*read((x)))
#define holds(x, y) (std::holds_alternative<y>(x))

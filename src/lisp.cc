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

#include "lisp.hh"

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

std::string replace(std::string str, std::string const& key, std::string const& val, std::size_t size) {
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

std::deque<std::string> split(std::string const& str, std::string const& delim, std::size_t size) {
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

std::shared_ptr<Env> Fun::bind(Lst& l, std::shared_ptr<Env> e) {
  if (args.empty() && l.size()) {throw std::runtime_error("expected '0' arguments");}
  auto const has_rest {[this]() {
    if (auto const l = xpr_lst(&args.back()); l && l->empty()) {
      return true;
    }
    return false;
  }()};
  if (has_rest) {
    if (l.size() < args.size() - 1) {
      throw std::runtime_error("expected at least '" + std::to_string(args.size() - 1) + "' " + plural("argument", "s", args.size() - 1));
    }
    auto ev = std::make_shared<Env>(env, e);
    auto v = l.begin();
    for (auto s = args.begin(), end = --args.end(); s != end; ++s, ++v) {
      if (auto const sym = xpr_sym(&*s)) {
        (*ev)[*sym] = {*v, e};
        continue;
      }
      throw std::runtime_error("fn binding expected symbol");
    }
    Xpr rest;
    auto& r = std::get<Lst>(rest);
    r.splice(r.begin(), l, v, l.end());
    (*ev)["@"] = {rest, e, Val::evaled};
    return ev;
  }
  if (l.size() != args.size()) {
    throw std::runtime_error("expected '" + std::to_string(args.size()) + "' " + plural("argument", "s", args.size()));
  }
  auto ev = std::make_shared<Env>(env, e);
  auto s = args.begin();
  for (auto& v : l) {
    if (auto const sym = xpr_sym(&*s)) {
      (*ev)[*sym] = {v, e};
      ++s;
      continue;
    }
    throw std::runtime_error("fn binding expected symbol");
  }
  return ev;
}

Tks str_tks(std::string_view& str) {
  Tks tks;
  std::string r = R"RAW(\s*([()\[\]{}'!,]|\\.[^\\d]*|"[^"\\]*(?:\\.[^"\\]*)*"|;.*|[^\s()\[\]{}'"`,#;]*)(.*))RAW";
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
    return num_atm(Int{std::stod(tk)});
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
    return str_xpr(tk.substr(1, tk.size() - 2));
  }
  else if (tk == "!") {
    if (auto const x = tks_xpr(ts)) {
      return Xpr{Lst{sym_xpr("eval"), *x}};
    }
    throw std::runtime_error("expected Xpr after '`'");
  }
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

#define str_lst(x) std::get<Lst>(*read((x)))

std::size_t type(Xpr const& x) {
  if (auto const a = xpr_atm(&x)) {
    if (auto const s = atm_str(a)) {return Typ::Str;}
    if (auto const s = atm_sym(a)) {return Typ::Sym;}
    if (auto const n = atm_num(a)) {
      if (auto const v = num_int(n)) {return Typ::Int;}
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

std::string print(Xpr const& x) {
  std::ostringstream os;
  std::function<void(Xpr const&)> const to_string_impl = [&](Xpr const& x) {
    if (auto const a = xpr_atm(&x)) {
      if (auto const s = atm_str(a)) {os << "\"" + s->str() + "\"";}
      else if (auto const s = atm_sym(a)) {os << *s;}
      else if (auto const n = atm_num(a)) {
        if (auto const v = num_int(n)) {os << *v;}
      }
    }
    else if (auto const l = xpr_lst(&x)) {
      if (l->size() == 2) {
        if (auto const s = xpr_sym(&l->front())) {
          if (*s == "eval") {
            os << "!";
            to_string_impl(l->back());
            return;
          }
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
      os << "#<fn>";
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
      }
    }
    else if (auto const l = xpr_lst(&x)) {
      if (l->size() == 2) {
        if (auto const s = xpr_sym(&l->front())) {
          if (*s == "eval") {
            os << aec::fg_cyan << "!" << aec::clear;
            to_string_impl(l->back());
            return;
          }
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
      os << aec::fg_blue << "#<fn>" << aec::clear;
    }
  };
  to_string_impl(x);
  return os.str();
}

Xpr eval_impl(Xpr&, std::shared_ptr<Env>);
Xpr eval(Xpr& xr, std::shared_ptr<Env> ev);
Xpr eval(Xpr&& xr, std::shared_ptr<Env> ev);

Xpr eval(Xpr&& xr, std::shared_ptr<Env> ev) {
  try {
    auto res = eval_impl(xr, ev);
    // std::cerr << aec::fg_magenta << "> " << cprint(xr) << " -> " << cprint(res) << aec::clear << "\n";
    return res;
  }
  catch (...) {
    // std::cerr << aec::fg_red << "> " << aec::clear << cprint(xr) << "\n";
    throw;
  }
}

Xpr eval(Xpr& xr, std::shared_ptr<Env> ev) {
  try {
    auto res = eval_impl(xr, ev);
    // std::cerr << aec::fg_magenta << "> " << cprint(xr) << " -> " << cprint(res) << aec::clear << "\n";
    return res;
  }
  catch (...) {
    // std::cerr << aec::fg_red << "> " << aec::clear << cprint(xr) << "\n";
    throw;
  }
}

Xpr eval_impl(Xpr& xr, std::shared_ptr<Env> ev) {
  if (auto const a = xpr_atm(&xr)) {
    if (auto const s = atm_str(a)) {return str_xpr(*s);}
    if (auto const s = atm_sym(a)) {
      if (auto p = ev->find(*s)) {
        auto& v = (*p)->second;
        if (!(v.ctx & Val::evaled)) {
          v.ctx |= Val::evaled;
          v.xpr = eval(v.xpr, v.env);
        }
        return v.xpr;
      }
      throw std::runtime_error("unbound symbol '" + *s + "'");
    }
    if (auto const n = atm_num(a)) {
      if (auto const p = num_int(n)) {return num_xpr(*p);}
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
      Xpr res {f->fn(f->bind(l, ev))};
      return res;
    }
    if (auto const i = xpr_int(&fn)) {
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
        Xpr v {sym_xpr(*s)};
        Xpr func {eval(v, ev)};
        if (auto const f = xpr_fun(&func)) {
          Xpr args {*l};
          auto& l = std::get<Lst>(args);
          l.pop_front();
          Xpr res {f->fn(f->bind(l, ev))};
          return res;
        }
      }
    }
    throw std::runtime_error("unknown function '" + print(fn) + "'");
  }
  throw std::runtime_error("invalid eval");
}

void env_init(std::shared_ptr<Env> ev, int argc, char** argv) {
  auto constexpr builtin {Val::evaled};

  auto const cmp = [](Xpr& lhs, Xpr& rhs, auto fn) -> bool {
    if (auto const a = xpr_num(&lhs)) {
      if (auto const b = xpr_num(&rhs)) {
        if (a->index() == b->index()) {
          return fn(std::get<Int>(*a), std::get<Int>(*b));
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
      return Num{static_cast<Int>(fn(std::get<Int>(*a), std::get<Int>(*b)))};
    }
    throw std::runtime_error("invalid number");
  };

  (*ev)["Int"] = Val{sym_xpr("Int"), ev, builtin};
  (*ev)["Num"] = Val{sym_xpr("Flo"), ev, builtin};
  (*ev)["Sym"] = Val{sym_xpr("Sym"), ev, builtin};
  (*ev)["Str"] = Val{sym_xpr("Str"), ev, builtin};
  (*ev)["Atm"] = Val{sym_xpr("Atm"), ev, builtin};
  (*ev)["Fun"] = Val{sym_xpr("Fun"), ev, builtin};
  (*ev)["Lst"] = Val{sym_xpr("Lst"), ev, builtin};
  (*ev)["Xpr"] = Val{sym_xpr("Xpr"), ev, builtin};

  (*ev)["T"] = Val{sym_xpr("T"), ev, builtin};
  (*ev)["F"] = Val{sym_xpr("T"), ev, builtin};

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
        return num_xpr(n);
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
    if (auto const lhs = xpr_num(&a)) {
      if (auto const rhs = xpr_num(&b)) {
        auto n = math(lhs, rhs, [](auto const a, auto const b) {return a / b;});
        return num_xpr(n);
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
        return num_xpr(n);
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
        return num_xpr(n);
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
    return num_xpr(static_cast<Int>(static_cast<i64>(*lhs) % static_cast<i64>(*rhs)));
  }}, ev, builtin};

  (*ev)["throw"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
    auto a = eval(sym_xpr("a"), e);
    throw std::runtime_error(print(a));
  }}, ev, builtin};

  (*ev)["eval"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
    auto a = eval(sym_xpr("a"), e);
    return eval(a, e->current);
  }}, ev, builtin};

  (*ev)["apply"] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
    auto a = eval(sym_xpr("a"), e);
    auto b = eval(sym_xpr("b"), e);
    if (auto const lhs = xpr_fun(&a)) {
      if (auto const rhs = xpr_lst(&b)) {
        rhs->emplace_front(a);
        return eval(b, e);
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
          v = eval(x, e);
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
          auto v = eval(x, e);
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
          rhs->emplace_front(eval(x, e));
        }
        return rhs->front();
      }
    }
    throw std::runtime_error("invalid types '" + typ_str.at(type(a)) + "' and '" + typ_str.at(type(b)) + "'");
  }}, ev, builtin};

  (*ev)["env"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
    e->current->dump();
    return sym_xpr("T");
  }}, ev, builtin};

  (*ev)["show"] = Val{Fun{str_lst("()"), [&](auto e) -> Xpr {
    e->current->dump_inner();
    return sym_xpr("T");
  }}, ev, builtin};

  (*ev)["slp"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
    auto a = eval(sym_xpr("a"), e);
    if (auto const v = xpr_int(&a)) {
      std::this_thread::sleep_for(std::chrono::seconds(static_cast<long>(*v)));
      return sym_xpr("T");
    }
    throw std::runtime_error("expected integer");
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
    if (auto const s = xpr_sym(&a)) {
      if (auto p = e->find_current(*s)) {
        auto& v = (*p)->second;
        if (v.ctx & Val::constant) throw std::runtime_error("constant binding");
        v = Val{(*e->find_inner("b"))->second.xpr, e->current, Val::constant};
        return a;
      }
      (*e->current)[*s] = Val{(*e->find_inner("b"))->second.xpr, e->current, Val::constant};
      return a;
    }
    throw std::runtime_error("expected symbol");
  }}, ev, builtin};

  (*ev)["var"] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
    auto a = (*e->find_inner("a"))->second.xpr;
    if (auto const s = xpr_sym(&a)) {
      if (auto p = e->find_current(*s)) {
        auto& v = (*p)->second;
        if (v.ctx & Val::constant) throw std::runtime_error("constant binding");
        v = Val{(*e->find_inner("b"))->second.xpr, e->current};
        return a;
      }
      (*e->current)[*s] = Val{(*e->find_inner("b"))->second.xpr, e->current};
      return a;
    }
    throw std::runtime_error("expected symbol");
  }}, ev, builtin};

  (*ev)["set"] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
    auto a = (*e->find_inner("a"))->second.xpr;
    if (auto const s = xpr_sym(&a)) {
      if (auto p = e->find_current(*s)) {
        auto& v = (*p)->second;
        if (v.ctx & Val::constant) throw std::runtime_error("constant binding");
        v = Val{(*e->find_inner("b"))->second.xpr, e->current};
        return a;
      }
      throw std::runtime_error("unbound symbol '" + *s + "'");
    }
    throw std::runtime_error("expected symbol");
  }}, ev, builtin};

  (*ev)["quote"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
    return (*e->find_inner("a"))->second.xpr;
  }}, ev, builtin};

  (*ev)["lst"] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
    auto a = eval(sym_xpr("a"), e);
    auto b = eval(sym_xpr("b"), e);
    if (auto const l = xpr_lst(&b)) {
      l->emplace_front(a);
      return b;
    }
    return Xpr{Lst{a, b}};
  }}, ev, builtin};

  (*ev)["car"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
    auto x = eval(sym_xpr("a"), e);
    if (auto const l = xpr_lst(&x)) {
      if (l->empty()) {throw std::runtime_error("null list");}
      return *l->begin();
    }
    throw std::runtime_error("expected list");
  }}, ev, builtin};

  (*ev)["cdr"] = Val{Fun{str_lst("(a)"), [&](auto e) -> Xpr {
    auto x = eval(sym_xpr("a"), e);
    if (auto const l = xpr_lst(&x)) {
      if (l->empty()) {throw std::runtime_error("null list");}
      return (l->pop_front(), x);
    }
    throw std::runtime_error("expected list");
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

  (*ev)["pn"] = Val{Fun{str_lst("(())"), [&](auto e) -> Xpr {
    auto x = eval(sym_xpr("@"), e);
    auto& l = std::get<Lst>(x);
    Xpr res {sym_xpr("F")};
    for (auto it = l.begin(); it != l.end(); ++it) {
      res = eval(*it, e->current);
    }
    return res;
  }}, ev, builtin};

  (*ev)["fn"] = Val{Fun{str_lst("(a b)"), [&](auto e) -> Xpr {
    return Xpr{Fun{std::get<Lst>((*e->find_inner("a"))->second.xpr), [body = (*e->find_inner("b"))->second.xpr](std::shared_ptr<Env> e) mutable -> Xpr {
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
        std::cout << aec::fg_red << "> " << aec::clear << cprint(*read("(err \""s + e.what() + "\")"s)) << "\n";
      }
    }
    std::cout << prompt;
  }
  std::cout << "\n";
}

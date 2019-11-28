#ifndef OB_STRING_HH
#define OB_STRING_HH

#include <cstddef>

#include <functional>
#include <utility>
#include <string>
#include <optional>
#include <regex>
#include <limits>
#include <vector>
#include <unordered_map>

namespace OB::String
{

std::string plural(std::string const& str, std::size_t num);

std::string plural(std::string const& str, std::string const& end, std::size_t num);

std::string file(std::string const& str);

std::string replace(std::string str, std::string const& key, std::string const& val,
  std::size_t size = std::numeric_limits<std::size_t>::max());

std::pair<std::string, std::size_t> replace(std::string str, std::regex const& rx, std::function<std::string(std::smatch const&)> fn, std::size_t size = std::numeric_limits<std::size_t>::max());

std::string format(std::string str, std::unordered_map<std::string, std::string> args);

std::string xformat(std::string str, std::unordered_map<std::string, std::string> args);

std::string to_string(double const val, int const pre = 2);

std::vector<std::string> split(std::string const& str, std::string const& delim,
  std::size_t size = std::numeric_limits<std::size_t>::max());

std::vector<std::string_view> split_view(std::string_view str, std::string_view delim,
  std::size_t size = std::numeric_limits<std::size_t>::max());

std::string lowercase(std::string const& str);

std::string trim(std::string str);

bool assert_rx(std::string const& str, std::regex rx);

std::optional<std::vector<std::string>> match(std::string const& str, std::regex rx);

std::string repeat(std::size_t const num, std::string const& str);

bool starts_with(std::string const& str, std::string const& val);

std::string escape(std::string str);

std::string unescape(std::string str);

std::size_t count(std::string const& str, std::string const& val);

std::size_t damerau_levenshtein(std::string const& lhs, std::string const& rhs,
  std::size_t const weight_insert = 1, std::size_t const weight_substitute = 1,
  std::size_t const weight_delete = 1, std::size_t const weight_transpose = 1);

} // namespace OB::String

#endif // OB_STRING_HH

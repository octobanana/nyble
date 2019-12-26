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

Copyright (c) 2018-2019 Brett Robinson <https://octobanana.com/>

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

#ifndef OB_ORDERED_MAP_HH
#define OB_ORDERED_MAP_HH

#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <cstddef>
#include <cstdint>

#include <deque>
#include <string>
#include <vector>
#include <limits>
#include <memory>
#include <utility>
#include <iterator>
#include <algorithm>
#include <filesystem>
#include <type_traits>
#include <unordered_map>
#include <initializer_list>

namespace OB {

// Ordered_Map: an insert ordered map
// enables random lookup and insert ordered iterators
// unordered map stores key value pairs
// queue holds insert ordered iterators to each key in the unordered map
template<typename K, typename V>
class Ordered_map final
{
public:

  // map iterators
  using m_iterator = typename std::unordered_map<K, V>::iterator;
  using m_const_iterator = typename std::unordered_map<K, V>::const_iterator;

  // index iterators
  using i_iterator = typename std::deque<m_iterator>::iterator;
  using i_const_iterator = typename std::deque<m_const_iterator>::const_iterator;
  using i_reverse_iterator = typename std::deque<m_iterator>::reverse_iterator;
  using i_const_reverse_iterator = typename std::deque<m_const_iterator>::const_reverse_iterator;


  Ordered_map()
  {
  }

  Ordered_map(std::initializer_list<std::pair<K, V>> const& lst)
  {
    for (auto const& [key, val] : lst)
    {
      _it.emplace_back(_map.insert({key, val}).first);
    }
  }

  ~Ordered_map()
  {
  }

  Ordered_map& operator()(K const& k, V const& v)
  {
    auto it = _map.insert_or_assign(k, v);

    if (it.second)
    {
      _it.emplace_back(it.first);
    }

    return *this;
  }

  i_iterator operator[](std::size_t index)
  {
    return _it[index];
  }

  i_const_iterator const operator[](std::size_t index) const
  {
    return _it[index];
  }

  V& at(K const& k)
  {
    return _map.at(k);
  }

  V const& at(K const& k) const
  {
    return _map.at(k);
  }

  m_iterator find(K const& k)
  {
    return _map.find(k);
  }

  m_const_iterator find(K const& k) const
  {
    return _map.find(k);
  }

  std::size_t size() const
  {
    return _it.size();
  }

  bool empty() const
  {
    return _it.empty();
  }

  Ordered_map& clear()
  {
    _it.clear();
    _map.clear();

    return *this;
  }

  Ordered_map& erase(K const& k)
  {
    auto it = _map.find(k);

    if (it != _map.end())
    {
      for (auto e = _it.begin(); e < _it.end(); ++e)
      {
        if ((*e) == it)
        {
          _it.erase(e);
          break;
        }
      }

      _map.erase(it);
    }

    return *this;
  }

  i_iterator begin()
  {
    return _it.begin();
  }

  i_const_iterator begin() const
  {
    return _it.begin();
  }

  i_const_iterator cbegin() const
  {
    return _it.cbegin();
  }

  i_reverse_iterator rbegin()
  {
    return _it.rbegin();
  }

  i_const_reverse_iterator rbegin() const
  {
    return _it.rbegin();
  }

  i_const_reverse_iterator crbegin() const
  {
    return _it.crbegin();
  }

  i_iterator end()
  {
    return _it.end();
  }

  i_const_iterator end() const
  {
    return _it.end();
  }

  i_const_iterator cend() const
  {
    return _it.cend();
  }

  i_reverse_iterator rend()
  {
    return _it.rend();
  }

  i_const_reverse_iterator rend() const
  {
    return _it.rend();
  }

  i_const_reverse_iterator crend() const
  {
    return _it.crend();
  }

  m_iterator map_begin()
  {
    return _map.begin();
  }

  m_const_iterator map_begin() const
  {
    return _map.begin();
  }

  m_const_iterator map_cbegin() const
  {
    return _map.cbegin();
  }

  m_iterator map_end()
  {
    return _map.end();
  }

  m_const_iterator map_end() const
  {
    return _map.end();
  }

  m_const_iterator map_cend() const
  {
    return _map.cend();
  }

private:

  std::unordered_map<K, V> _map;
  std::deque<m_iterator> _it;
}; // class Ordered_map

} // namespace OB::Belle

#endif // OB_ORDERED_MAP_HH

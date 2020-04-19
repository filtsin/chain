#pragma once

#include <tuple>
#include <type_traits>
#include <cassert>

namespace details {

template <size_t Index, typename Fn, typename This, typename ...T>
typename std::enable_if<Index == sizeof...(T), void>::type
visit_tuple_el_by_index(std::tuple<T...> &tuple, size_t index, const Fn &fn, This this_p) {
  assert(false && "index >= tuple.size()");
}

template <size_t Index, typename Fn, typename This, typename ...T>
typename std::enable_if<Index < sizeof...(T), void>::type
visit_tuple_el_by_index(std::tuple<T...> &tuple, size_t index, const Fn &fn, This this_p) {
  if (Index == index) {
    auto el = std::get<Index>(tuple);
    fn(this_p, std::get<Index>(tuple));
  } else {
    visit_tuple_el_by_index<Index + 1>(tuple, index, fn, this_p);
  }
}

template <typename Fn, typename This, typename ...T>
void visit_tuple_el(std::tuple<T...> &tuple, size_t index, const Fn &fn, This this_p) {
  visit_tuple_el_by_index<0>(tuple, index, fn, this_p);
}

template <typename Iterator>
class range_wrapper {
public:
  using type = Iterator;
  using value_type = typename std::iterator_traits<Iterator>::value_type;
  using reference = value_type&;
  using pointer = value_type*;

  range_wrapper() = default;

  range_wrapper(Iterator _begin, Iterator _end)
    : begin(_begin), end(_end), cur(_begin) {
  }

  void next() {
    assert(cur != end);
    cur = std::next(cur);
  }

  void prev() {
    assert(cur != begin);
    cur = std::prev(cur);
  }

  void set_end() {
    cur = end;
  }

  bool is_begin() const {
    return cur == begin;
  }

  bool is_end() const {
    return cur == end;
  }

  pointer deref() const {
    return cur.operator->();
  }

private:

  Iterator begin;
  Iterator end;
  Iterator cur;
};

template <typename T, typename ...Iterator>
class chain_iterator {
public:
  using iterator = chain_iterator;
  using value_type = T;
  using reference = T&;
  using pointer = T*;
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = ptrdiff_t;

  chain_iterator(size_t _index, std::tuple<range_wrapper<Iterator>...> _tuple)
    : index(_index), tuple(std::move(_tuple)) { }

  iterator operator++() {
    iterator copy(*this);
    visit_tuple_el(tuple, index, visit_next_iterator{}, this);
    return copy;
  }

  iterator operator--() {
    iterator copy(*this);
    visit_tuple_el(tuple, index, visit_prev_iterator{}, this);
    return copy;
  }

  iterator operator++(int) {
    visit_tuple_el(tuple, index, visit_next_iterator{}, this);
    return *this;
  }

  iterator operator--(int) {
    visit_tuple_el(tuple, index, visit_prev_iterator{}, this);
    return *this;
  }

  reference operator*() {
    visit_iterator obj;
    visit_tuple_el(tuple, index, obj, this);
    return *obj.p_value;
  }

  pointer operator->() {
    visit_iterator obj;
    visit_tuple_el(tuple, index, obj, this);
    return obj.p_value;
  }

  bool operator==(iterator &rhs) {
    if (index != rhs.index) { return false; }
    visit_iterator this_obj;
    visit_tuple_el(tuple, index, this_obj, this);
    visit_iterator rhs_obj;
    visit_tuple_el(rhs.tuple, index, rhs_obj, &rhs);
    return this_obj.p_value == rhs_obj.p_value;
  }

  bool operator!=(iterator &rhs) {
    return !(*this == rhs);
  }

private:
  size_t index;
  std::tuple<range_wrapper<Iterator>...> tuple;

  struct visit_next_iterator {
    template <typename U>
    void operator()(chain_iterator *iter, U &el) const {
      el.next();
      if (el.is_end() && iter->index < sizeof...(Iterator) - 1) {
        iter->index += 1;
      }
    }
  };

  struct visit_prev_iterator {
    template <typename U>
    void operator()(chain_iterator *iter, U &el) const {
      if (el.is_begin()) {
        iter->index -= 1;
        visit_tuple_el(iter->tuple, iter->index, visit_prev_iterator{}, iter);
      } else {
        el.prev();
      }
    }
  };

  struct visit_iterator {
    template <typename U>
    void operator()(const chain_iterator *iter, U &el) const {
      p_value = el.deref();
    }

    mutable pointer p_value;
  };
};

template <typename ...Pair>
class chain {
public:
  using tuple_type = std::tuple<range_wrapper<typename Pair::first_type>...>;
  using iterator_trait = std::iterator_traits<typename std::tuple_element<0, std::tuple<typename Pair::first_type...>>::type>;
  using value_type = typename iterator_trait::value_type;
  using iterator = chain_iterator<value_type, typename Pair::first_type...>;

  explicit chain(Pair... pair) {
    construct<0, sizeof...(Pair)>(std::make_tuple(pair...));
  }

  iterator begin() {
    return iterator { 0, chain_container };
  }

  iterator end() {
    tuple_type tuple = chain_container;
    std::get<sizeof...(Pair) - 1>(tuple).set_end();
    return iterator { sizeof...(Pair) - 1, std::move(tuple) };
  }

private:

  template <size_t Index, size_t N>
  typename std::enable_if<Index < N, void>::type
  construct(const std::tuple<Pair...> &tuple) {

    using fst_type = typename std::tuple_element<Index, std::tuple<Pair...>>::type::first_type;
    using fst_value_type = typename std::iterator_traits<fst_type>::value_type;

    using snd_type =  typename std::tuple_element<Index, std::tuple<Pair...>>::type::second_type;
    using snd_value_type = typename std::iterator_traits<snd_type>::value_type;

    static_assert(std::is_same<fst_type, snd_type>::value, "Each pair of iterators must be the same");
    static_assert(std::is_convertible<fst_value_type, value_type>::value, "Value type of iterators must be the same or convertible to first");

    std::get<Index>(chain_container) = range_wrapper<fst_type>{std::get<Index>(tuple).first, std::get<Index>(tuple).second};

    construct<Index + 1, N>(tuple);
  }

  template <size_t Index, size_t N>
  typename std::enable_if<Index == N, void>::type
  construct(const std::tuple<Pair...> &tuple) { }

  tuple_type chain_container;
};

template <typename ...Pair>
chain<Pair...> make_chain_inner(const std::tuple<Pair...> &tuple, Pair ...iterator) {
  return chain<Pair...>(iterator...);
}

}

template <typename ...Container>
auto make_chain(Container &...container) -> decltype(details::make_chain_inner(std::make_tuple(std::make_pair(std::begin(container), std::end(container))...), std::make_pair(std::begin(container), std::end(container))...)) {
  auto tuple = std::make_tuple(std::make_pair(std::begin(container), std::end(container))...);
  return details::make_chain_inner(tuple, std::make_pair(std::begin(container), std::end(container))...);
}

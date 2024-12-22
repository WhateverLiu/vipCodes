#pragma once
#include <tbb/tbb.h>
#include <thread>
#include <ranges>
#include <execution>


namespace tp {


int maxThread = std::thread::hardware_concurrency();
int & maxCore() { return maxThread; }


struct activate
{
  activate(int && Ncore = std::thread::hardware_concurrency())
  {
    Ncore = std::min<int>(std::thread::hardware_concurrency(), Ncore);
    maxCore() = Ncore;
  }
  ~activate() { maxCore() = std::thread::hardware_concurrency(); }
};




/**
 * An random access iterator class whose dereference is `std::size_t`.
 */
struct IntRAiter // Integer random access iterator.
{
  std::size_t i; 
  IntRAiter () = default;
  void reset(std::size_t i) { this->i = i; }
  IntRAiter (std::size_t i) { reset(i); }
  
  
  using iterator_category = std::random_access_iterator_tag;
  using value_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type*;
  using reference = value_type&;
  
  
  reference operator*() { return i; }
  pointer operator->() { return &i; }
  IntRAiter& operator++() { ++i; return *this; }
  IntRAiter operator++(int) { IntRAiter tmp = *this; ++(*this); return tmp; }
  IntRAiter& operator--() { --i; return *this; }
  IntRAiter operator--(int) { IntRAiter tmp = *this; --(*this); return tmp; }
  IntRAiter& operator+=(difference_type diff) { i += diff; return *this; }
  IntRAiter& operator-=(difference_type diff) { i -= diff; return *this; }
  difference_type operator-(const IntRAiter& other) const { return i - other.i; }
  IntRAiter operator+(difference_type diff) const { return IntRAiter(i + diff); }
  IntRAiter operator-(difference_type diff) const { return IntRAiter(i - diff); }
  bool operator==(const IntRAiter& other) const { return i == other.i; }
  bool operator!=(const IntRAiter& other) const { return i != other.i; }
  bool operator<(const IntRAiter& other) const { return i < other.i; }
  bool operator>(const IntRAiter& other) const { return i > other.i; }
  bool operator<=(const IntRAiter& other) const { return i <= other.i; }
  bool operator>=(const IntRAiter& other) const { return i >= other.i; }
  std::size_t operator[](difference_type offset) { return i + offset; }
};




void parFor ( std::size_t begin, 
              std::size_t end, 
              auto && f , 
              std::size_t grainSize)
{
  if ( maxCore() == 1)
  { 
    for (; begin < end and !f(begin, 0); ++begin);
    return;
  }
  auto g = [&](std::size_t i)->bool
  { 
    i = begin + i * grainSize;
    auto iend = std::min(i + grainSize, end);
    auto t = tbb::task_arena::current_thread_index();
    for (; i < iend and !f(i, t); ++i);
    return i < iend; // true means early return.
  }; 
  std::any_of(std::execution::par_unseq, 
              IntRAiter(0), 
              IntRAiter((end - begin + grainSize - 1) / grainSize), 
              g);
}




}





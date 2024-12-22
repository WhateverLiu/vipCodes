#pragma once
#include "multiMerge.hpp"
#include "ThreadPool2.hpp"


namespace Charlie {


/**
 * @param x  Pointer / iterator pointing to the beginning of sequence.
 * 
 * @param xend  Pointer / iterator pointing to the end of sequence.
 * 
 * @param f  Comparison functor (`x`, `y`). Return true if `x` 
 * should precede `y`.
 * 
 * 
 */
void parSort(auto x, auto xend, auto && f)
{
  using num = std::remove_reference<decltype(*x)>::type;
  vec<num> v(x, xend);
  uint64_t Nblock = tp::maxCore() * tp::maxCore(); 
  double blockSize = (xend - x) / double(Nblock);
  vec<std::pair<num*, num*> > blocks(Nblock);
  for (int i= 0, iend = Nblock; i < iend; ++i)
  {
    blocks[i].first = v.data() + uint64_t(std::round(i * blockSize));
    blocks[i].second = v.data() + uint64_t(std::round((i + 1) * blockSize));
  }
  
  
  tp::parFor(0, Nblock, [&](std::size_t i, std::size_t t)->bool
  {
    std::sort(blocks[i].first, blocks[i].second, f);
    return false;
  });
  
  
  kwayMerge(blocks.begin(), blocks.end(), x, xend, g);
}


void parSort(auto x, auto xend)
{
  using num = std::remove_reference<decltype(*x)>::type;
  auto f = std::less<num>();
  (*this)(x, xend, f);
}






}











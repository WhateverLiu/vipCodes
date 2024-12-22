#pragma once


namespace Charlie {
/**
 * Multimerge.
 * 
 * @param x  A pointer or iterator to a sequence of std::pair<Iter, Iter>s. 
 * In the i-th pair, the 1st element is a pointer/iterator pointing to 
 * the beginning of the i-th sequence, and the 2nd element points to the 
 * end of the i-th sequence. There are `xend - x` many 
 * sequences to be merged.
 * 
 * @param xend  End of the pointer / iterator sequence.
 * 
 * @param rst  Pointer / iterator pointing to the beginning of the container
 * for storing the merged result.
 * 
 * @param rstEnd  Pointer / iterator pointing to the end of the container
 * for storing the merged result.
 * 
 * @param  Comparison functor.
 * 
 * Contents in [x, xend) could be modified.
 * 
 * 
 */
void kwayMerge(auto x, auto xend, 
               auto rst, auto rstEnd, 
               auto && cmp)
{
  using E = std::remove_reference<decltype(*x)>::type;
  auto g = [&](const E & x, const E & y)->bool { return *x.first > *y.first; };
  std::make_heap(x, xend, g);
  while (x != xend and rst != rstEnd)
  {
    std::pop_heap(x, xend, g);
    --xend;
    *rst = *xend->first;
    ++rst;
    ++xend->first;
    if (xend->first != xend->second)
    {
      ++xend; 
      std::push_heap(x, xend, g);
    }
  }
}




}



























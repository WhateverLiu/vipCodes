#pragma once
#include "multiMerge.hpp"
#include "ThreadPool2.hpp"
#include "MiniPCG.hpp"
#include "tiktok.hpp"


namespace Charlie {


void partition(auto x, auto xend, auto && f, 
               auto & barriers, int depth)
{
  if (depth <= 0 or xend - x < 2) return;
  std::nth_element(x, x + (xend - x) / 2, xend, f);
  barriers.emplace_back( *(x + (xend - x) / 2) );
  partition(x, x + (xend - x) / 2, f, barriers, depth - 1);
  partition(x + (xend - x) / 2 + 1, xend, f, barriers, depth - 1);
}


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
  auto xsize = xend - x;
  if (tp::maxCore() <= 1 or xsize < 3000) { std::sort(x, xend, f); return; }
  
  
  using num = std::remove_reference<decltype(*x)>::type;
  int Nbarrier = 1 << int(std::round(std::log2(tp::maxCore() * 1.0)));
  // std::cout << "Nbarrier = " << Nbarrier << "\n";
  
  
  MiniPCG rng(42);
  std::uniform_int_distribution<int64_t> U(0, xsize - 1);
  
  
  // tiktok<std::chrono::microseconds> timer;
  // timer.tik();
  vec<num> samples;
  samples.reserve(std::max<int>(Nbarrier, 2048));
  for (int i = 0, iend = samples.capacity(); i < iend; ++i)
    samples.emplace_back(*(x + U(rng)));
  // std::cout << "samples populated = " << timer.tok() << "\n";
  
  
  // timer.tik();
  vec<num> barriers;
  if ( int(samples.size()) <= Nbarrier ) barriers.swap(samples);
  else
  {
    int depth = std::round(std::log2(double(Nbarrier)));
    // std::cout << "depth = " << depth << "\n";
    partition(samples.begin(), samples.end(), f, barriers, depth);
    vec<num>().swap(samples);
  }
  std::sort(barriers.begin(), barriers.end(), f);
  Nbarrier = barriers.size();
  // std::cout << "barriers made, time cost = " << timer.tok() << ", Nbarrier = " << Nbarrier << "\n";
  
  
  // timer.tik();
  vec<vec<vec<num> > > V(tp::maxCore());
  int64_t iniSize = std::max<int64_t>(
    1, xsize * 1.3 / (tp::maxCore() * (Nbarrier + 1)));
  for (auto & eachCore: V)
  {
    eachCore.resize(Nbarrier + 1);
    for (auto & chunk: eachCore) chunk.reserve(iniSize);
  }
  // std::cout << "V initialized = " << timer.tok() << "\n";
  
  
  // timer.tik();
  tp::parFor(0, xsize, [&](std::size_t i, std::size_t t)->bool
  {
    auto & y = x[i];
    auto it = std::lower_bound(barriers.begin(), barriers.end(), y, f) -
      barriers.begin();
    // int it = 0, itend = barriers.size();
    // for (; it != itend and f(barriers[it], y); ++it);
    V[t][it].emplace_back(y);
    return false;
  }, xsize / (tp::maxCore() * tp::maxCore() * tp::maxCore()) + 1);
  // std::cout << "V populated = " << timer.tok() << "\n";
  
  
  // timer.tik();
  auto barriersBars = vec<int64_t> (barriers.size() + 2, 0);
  int64_t * sizes = barriersBars.data() + 1;
  for (int i = 0, iend = barriersBars.size() - 1; i < iend; ++i)
  {
    for (auto & v: V) sizes[i] += v[i].size();
  }
  for (int i = 1, iend = barriersBars.size(); i < iend; ++i)
    barriersBars[i] += barriersBars[i - 1];
  // std::cout << "barrier bars made, time cost = " << timer.tok() << "\n";
  
  
  // timer.tik();
  tp::parFor(0, barriersBars.size() - 1, [&](std::size_t i, std::size_t t)->bool
  {
    auto begin = x + barriersBars[i];
    for (auto & v: V)
    {
      std::copy(v[i].begin(), v[i].end(), begin);
      begin += v[i].size();
    }
    std::sort(x + barriersBars[i], x + barriersBars[i + 1], f);
    return false;
  }, (barriersBars.size() - 1) / (tp::maxCore() * tp::maxCore()) + 1);
  // std::cout << "Final sorting = " << timer.tok() << "\n";
}



/*
void parSort(auto x, auto xend, auto && f)
{
  if (tp::maxCore() == 1) { std::sort(x, xend, f); return; }
  using num = std::remove_reference<decltype(*x)>::type;
  vec<num> v(x, xend);
  uint64_t Nblock = tp::maxCore(); 
  double blockSize = (xend - x) / double(Nblock);
  vec<std::pair<num*, num*> > blocks(Nblock);
  for (int i= 0, iend = Nblock; i < iend; ++i)
  {
    blocks[i].first = v.data() + uint64_t(std::round(i * blockSize));
    blocks[i].second = v.data() + uint64_t(std::round((i + 1) * blockSize));
  }
  
  
  tiktok<std::chrono::microseconds> timer;
  timer.tik();
  tp::parFor(0, Nblock, [&](std::size_t i, std::size_t t)->bool
  {
    std::sort(blocks[i].first, blocks[i].second, f);
    return false;
  });
  auto paraSortTime = timer.tok();
  
  
  timer.tik();
  if (Nblock != 1)
    kwayMerge(blocks.begin(), blocks.end(), x, xend, f);
  auto kwayMergeTime = timer.tok();
  
  
  std::cout << "paraSortTime = " << paraSortTime << ", ";
  std::cout << "kwayMergeTime = " << kwayMergeTime << "\n";
}
*/




void parSort(auto x, auto xend)
{
  using num = std::remove_reference<decltype(*x)>::type;
  auto f = std::less<num>();
  parSort(x, xend, f);
}






}











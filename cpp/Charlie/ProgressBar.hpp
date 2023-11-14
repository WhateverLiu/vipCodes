#pragma once


namespace Charlie
{
template <int size = 100> // Number of items to print, e.g. 100.
struct ProgressBar // Suitable in multithreading environment.
{
  int64_t which;
  std::size_t gap;
  bool p[size];
  ProgressBar(std::size_t imax)
  { 
    std::fill(p, p + size, false); 
    gap = (imax + (size - 1)) / size;
  }
  int64_t operator()(std::size_t i)
  {
    which = i / gap;
    if (!p[which]) 
    {
      p[which] = true;
      return which;
    }
    return -1;
  }
};
}


// =============================================================================
// Code example:
// =============================================================================
// Charlie::ProgressBar<100> pb(Nevent - NcoreEvent);
// if (verbose) Rcout << "Progress %: ";
// auto f = [&](std::size_t i, std::size_t t)->bool // A function to be loaded to many threads.
// {
//   if (verbose and t == 0)
//   {
//     auto p = pb(i - NcoreEvent);
//     if (p != -1) Rcout << p << " ";
//   }
//   ...
//   return false;
// }



#include <Rcpp.h>
using namespace Rcpp;
#include "../h/charlieThreadPool2.hpp"


int testRval(int &&x)
{
  x = 0;
  return x;
}


// [[Rcpp::export]]
int paraSummation(IntegerVector x, int maxCore = 15, int grainSize = 100)
{
  IntegerVector S(maxCore);
  CharlieThreadPool ctp(maxCore);
  auto f = [&](std::size_t i, std::size_t t)->bool
  {
    S[t] += (x[i] % 31 + x[i] % 131 + x[i] % 73 + x[i] % 37 + x[i] % 41) % 7;
    return false; // Return true indicates early return.
  };
  ctp.parFor(0, x.size(), std::move(f), grainSize);
  
  
  int maxCore2 = maxCore;
  testRval(std::move(maxCore));
  Rcout << "maxCore = " << maxCore << "\n";
  
  
  testRval(maxCore2);
  Rcout << "maxCore2 = " << maxCore2 << "\n";
  
  
  Rcout << "testRval = " << testRval(3 + 2 + 1 * 10) << "\n";
  
  
  return std::accumulate(S.begin(), S.end(), 0);
}


// =============================================================================
// R code to test:
// tmp2 = sample(1000L, 0.3e8, replace= T); 
// paraSummation(tmp2, maxCore = 2, grainSize = 100) - 
//   paraSummation(tmp2, maxCore = 100, grainSize = 100)
// =============================================================================

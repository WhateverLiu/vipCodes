// [[Rcpp::plugins(cpp20)]]
#include <Rcpp.h>
using namespace Rcpp;
#include "../cpp/Charlie/tiktok.hpp"
#include "../cpp/Charlie/MiniPCG.hpp"
#include <random>


template <bool useLinear = true>
auto test(int seed, int size, double & rst) 
{
  
  std::vector<double> v(size, 0);
  Charlie::tiktok<std::chrono::nanoseconds> timer;
  std::uniform_real_distribution<double> Ureal(0, 1);
  std::uniform_int_distribution<int> Uint(0, v.size() - 1);
  Charlie::MiniPCG rng(seed);
  
  
  std::size_t timecost = 0, timecostDoubleMul = 0;
  rst = 0;
  for (int i = 0; i < 10000; ++i)
  {
    int k = 0;
    for (auto & x: v) { x = Ureal(rng) + k; ++k; }
    
    
    for (int u = 0, uend = v.size(); u < uend; ++u)
    {
      auto t = Uint(rng);
      // Rcout << t << " ";
      timer.tik();
      if constexpr (useLinear)
        rst += *std::find(v.begin(), v.end(), v[t]);
      else
        rst += *std::lower_bound(v.begin(), v.end(), v[t]);
      timecost += timer.tok();
      
      
      double a = Ureal(rng);
      timer.tik();
      rst *= a;
      timecostDoubleMul += timer.tok();
    }
    // Rcout << "\n\n";
  }
  
  Rcout << "double multiplication time cost = " << timecostDoubleMul << "\n";
  
  
  return timecost;
}




// [[Rcpp::export]]
void testInR(int seed, int size)
{
  double rst1, rst2;
  auto lineartime = test<true>(seed, size, rst1);
  auto binsrchtime = test<false>(seed, size, rst2);
  Rcout << "rst1 = " << rst1 << ", rst2 = " << rst2 << "\n";
  Rcout << "linear time = " << lineartime << "\n";
  Rcout << "binary search time = " << binsrchtime << "\n";
}



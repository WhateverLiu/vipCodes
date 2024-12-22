// [[Rcpp::plugins(cpp20)]]
// #include "../cpp/Charlie/xxhash.hpp"
#include <Rcpp.h>
using namespace Rcpp;
#include <thread>
#include "../cpp/Charlie/nextPowerOf2.hpp"
#include "../cpp/Charlie/moduloPrimes.hpp"
// #include "../cpp/Charlie/MiniPCG.hpp"
#include <chrono>





// [[Rcpp::export]]
int test(NumericVector tid, // int seed = 42, 
         double scale = 4, int niter = 1000) 
{
  
  // // MiniPCG rng(std::size_t(std::chrono::high_resolution_clock::now()));
  // MiniPCG rng(seed);
  // auto gen64 = [&rng]()->uin64_t
  // {
  //   return (std::size_t(rng()) << std::size_t(32)) + rng();
  // };
  // auto x = (uint64_t*)&tid[0];
  // auto siz = np2(scale * tid.size());
  // std::vector<bool> taken(siz, false);
  // for (int iter = 0; iter < niter; ++iter)
  // {
  //   auto ofs = gen64();
  //   for ()
  // }
  
  
  
  // auto tid = std::this_thread::get_id();
  // auto tidptr = std::size_t(&tid);
  // Rcout << tidptr << "\n";
  // auto siz = Charlie::np2(tid.size()) * scale;
  auto siz = *std::lower_bound(Charlie::modPrimes, 
                               Charlie::modPrimes + 62, 
                               tid.size() * scale);
  // constexpr const std::size_t neg1 = 0 - 1;
  // std::vector<std::size_t> htable(siz, neg1);
  std::vector<bool> taken(siz, false);
  // auto hh = Charlie::xxhash();
  auto hh = std::hash<double>();
  int offset = 0;
  for (; offset < niter; ++offset)
  {
    int i = 0, iend = tid.size();
    for (; i < iend; ++i)
    {
      auto val = tid[i] + offset;
      // auto slot = hh(&val, sizeof(val)) % siz;
      // std::cout << *((uint64_t*)&val) << ", ";
      // std::cout << hh(val) << "\n";
      auto slot = hh(val) % siz;
      if (taken[slot]) break;
      taken[slot] = true;
    }
    if (i >= iend) break;
    // taken.assign(false, taken.size());
    std::fill(taken.begin(), taken.end(), false);
  }
  return offset;
}




#include <Rcpp.h>
using namespace Rcpp;
#include "../Charlie/MiniPCG.hpp"
#include "../Charlie/VecPool2.hpp"
#include "../Charlie/ThreadPool2.hpp"



// Generate a list of vectors of different sizes.
double f(int rseed)
{
  Charlie::MiniPCG rng(rseed);
  int mc = tp::maxCore();
  Rcout << "mc = " << mc << "\n";
  // int mc = 1;
  auto rngs = vp::lend<Charlie::MiniPCG>(0, mc);
  std::uniform_int_distribution<int> U(1, 1000);
  int size = U(rng);
  for (int i = 0, iend = rngs.size(); i < iend; ++i)
    rngs[i].seed(U(rng));
  // for (auto &x: rngs) x.seed(U(rng));
  auto sumv = vp::lend<double>(0, mc);
  std::fill(sumv.begin(), sumv.end(), 0.0);
  // std::atomic<std::size_t> count(2);
  auto maxSizes = vp::lend<int> (0, mc);
  tp::parFor(0, size, [&rngs, &U, //&count, 
             &maxSizes,
             &sumv](std::size_t i, std::size_t t)->bool
  {
    int size1 = U(rngs[t]), size2 = U(rngs[t]);
    // count.fetch_add(size1 + size2);
    maxSizes[t] = std::max(maxSizes[t], size1);
    // count.fetch_add(size1);
    auto x = vp::lend<int>(t, size1, size2);
    for (int u = 0; u < size1; ++u)
    {
      for (int v = 0; v < size2; ++v)
      {
        x[u][v] = U(rngs[t]);
        sumv[t] += x[u][v];
      }
    }
    return false;
  });
  Rcout << "count = " <<    
    std::accumulate(maxSizes.begin(), maxSizes.end(), 0) << "\n";  
  return std::accumulate(sumv.begin(), sumv.end(), 0.0);
}   


// [[Rcpp::export]]
double testVecPool(int rseed, int maxCore)   
{
  vp::activate();
  tp::activate(std::move(maxCore));
  
  
  auto rst = f(rseed);
  std::size_t truCount = 0;
  for (int i = 0, iend = vp::CharlieVpool.size(); i < iend; ++i)
  {
    truCount += vp::CharlieVpool[i].getPool().size();
  }
  Rcout << "truCount = " << truCount << "\n";
    
  
  tp::deactivate();
  vp::deactivate();
  return rst;
}



















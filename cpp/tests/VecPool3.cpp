// [[Rcpp::plugins(cpp17)]]
#include <Rcpp.h>
using namespace Rcpp;
#include "../Charlie/MiniPCG.hpp"
#include "../Charlie/ThreadPool2.hpp"
#include "../Charlie/VecPool3.hpp"


template <typename T> using vec = std::vector<T, vp::VecPool<T> >;
// template <typename T> using vec = std::vector<T, vp::NaiveAlloc<T> >;



 

// Generate a list of vectors of different sizes.
double f(int rseed) 
{
  Charlie::MiniPCG rng(rseed);
  int mc = tp::maxCore();
  auto rngs = vec<Charlie::MiniPCG>(mc);
  std::uniform_int_distribution<int> U(1, 3000);
  int size = U(rng);
  for (int i = 0, iend = rngs.size(); i < iend; ++i)
    rngs[i].seed(U(rng));
  auto sumv = vec<double> (mc, 0);
  auto maxSizes = vec<int>(mc, 0);
  tp::parFor(0, size, [&rngs, &U, //&count, 
             &maxSizes,
             &sumv](std::size_t i, std::size_t t)->bool
             {
               int size1 = U(rngs[t]), size2 = U(rngs[t]);                 
               maxSizes[t] = std::max(maxSizes[t], size1);
               
               
               // ==============================================================
               // Here, if the initialization is done like 
               //   vec<vec<int> > x(2, vec<int>(3) ),
               // then the pool will find vec<int>(3) as the first
               // vector. And because vec<int>(3) is smaller than
               // vec<vec<int>(2), vec<int>(3) cannot be used. Reallocation
               // will occur and thus 1 vector is wasted. But in practice,
               // this wastage should be usually acceptable.
               // ==============================================================
               vec<vec<int> > x(size1);
               for (auto &u: x) u.resize(size2);
               // ==============================================================
               
               
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
  vp::activate<true> vphandle;
  tp::activate tphandle(std::move(maxCore));
  auto rst = f(rseed);
  return rst;
}




// // [[Rcpp::export]]
// double testVecPool ( int rseed, int maxCore)
// {
//   
// }















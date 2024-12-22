// [[Rcpp::plugins(cpp20)]]
#include <Rcpp.h>
using namespace Rcpp;
#include "../Charlie/MiniPCG.hpp"
#include <random>
#include <bitset>


#include "../Charlie/useLinearAlloc.hpp"
// #include "../Charlie/useSTLalloc.hpp"


#define vec Allc::vec
#define hashmap Allc::hashmap
#define str Allc::str


// Generate a list of vectors of different sizes.
std::size_t f(int rseed, int Umax, bool testLinearize) 
{  
  Charlie::MiniPCG rng(rseed);
  std::uniform_int_distribution<int> U(1, Umax);
  int size = U(rng);
  
  
  auto rngs = vec<Charlie::MiniPCG>(size);
  for (int i = 0, iend = rngs.size(); i < iend; ++i)
    rngs[i].seed(U(rng));
  auto mc = tp::maxCore();
  auto sumv = vec<std::size_t> (mc, 0);
  
  
  if (testLinearize) 
    Rcout << "Will test memory linearization. This step could be "
  "much more time consuming for linear memory.\n";
  
  
  tp::parFor(0, size, [&rngs, &U, &sumv, &testLinearize] (
      std::size_t i, std::size_t t)->bool
      {
        
        {
          int size1 = U(rngs[i]);
          vec<vec<std::size_t> > a;
          hashmap<str, int> b;
          
          
          for (int u = 0; u < size1; ++u)
          {
            int size2 = U(rngs[i]);
            a.emplace_back(vec<std::size_t>());
            for (int v = 0; v < size2; ++v)
            {
              a[u].emplace_back( U(rngs[i]));              
              auto s = str(U(rngs[i]), '0');
              // std::cout << "s.size()= " << s.size() <<"\n";
              auto *sp = (uint8_t*)&s[0];
              for (int w = 0, wend = s.size(); w < wend; ++w)
                sp[w] = U(rngs[i]) % 256;
              b[s] = U(rngs[i]);
            }
          }
          
          
          
          
          if (testLinearize )
          {
            std::string s = "Before linearize address = ";
            // std::vector<std::size_t> before;
            for (auto & x: b)
            {
              // before.emplace_back(std::size_t(&x.first[0]));
              // before.emplace_back(std::size_t(&x.second));
              s += std::to_string(std::size_t(&x.first[0])) + ", " + 
                std::to_string(std::size_t(&x.second)) + ", ";
            }
            Allc::linearize ( a , b );
            s += "\nAfter linearize address = ";
            for (auto & x: b)
            {
              s += std::to_string(std::size_t(&x.first[0])) + ", " + 
                std::to_string(std::size_t(&x.second)) + ", ";
            }
            s += "\n\n";
            std::cout << s;
          }
          
          
          // std::cout << "1.65\n";
          
          auto it = b.begin();
          // std::cout << "1.66\n";
          for (int u = 0; u < size1 and it != b.end()
            ; ++u)
          {
            // std::cout << "1.67\n";
            for (int v = 0, vend = a[u].size(); 
                 v < vend and it != b.end()
                   ; ++v, ++it
            )
            {
              // std::cout << "1.68\n";
              sumv[t] += a[u][v] 
              + std::accumulate(
                  it->first.begin(),
                  it->first.end(), std::size_t(0)) +
                    it->second
              ;
            }
          }
        }
        
        
        return false;
      });
  
  
  return std::accumulate(sumv.begin(), sumv.end(), std::size_t(0));
}   


// [[Rcpp::export]]
double test(int rseed, int minBufferSizeMiB = 1, int Umax = 100, 
            int maxCore = 1, bool testLinearize = true)
{
  Allc::activate at(true, std::move(maxCore), minBufferSizeMiB, minBufferSizeMiB);  
  auto rst = f(rseed, Umax, testLinearize);

  
  return rst;
}  




// // [[Rcpp::export]]
// IntegerVector testUnique( IntegerVector x, int minBufferSizeMiB = 100, 
//                           int maxCore = 1)
// {
// #ifdef USE_LINEAR_MEM
//   LinearMem::activate lmhandle(std::move(maxCore), minBufferSizeMiB);
// #endif 
//   
//   
//   vec<int> a(20);
//   for (int i =0; i < 20; ++i)
//     Rcout << a[i] << ' ';
//   Rcout << '\n';
//   
//   
//   auto h = hashset<int>(x.begin(), x.end());
//   IntegerVector rst(h.size());
//   auto it = rst.begin();
//   for (int64_t i =0, iend = rst.size(); i < iend; ++i, ++it)
//     rst[i] = *it;
//   return rst;
// } 


#undef vec 
#undef hashmap
#undef str

















// [[Rcpp::plugins(cpp17)]]
#include <Rcpp.h>
using namespace Rcpp;
#include "../Charlie/Hashmap.hpp"
#include "../Charlie/MiniPCG.hpp"


// [[Rcpp::export]]
void test(int seed, int times, 
              double insertProb = 0.5) 
{
  
  Charlie::MiniPCG rng(seed);
  std::unordered_map<double, double, Charlie::SimpleHash<double>,
                     Charlie::SimpleEqual<double> > truth;
  Charlie::SimpleHashMap<double, double> mine;
  
  
  std::uniform_real_distribution<double> U(0, 1);
  for (int i = 0; i < times; ++i)
  {
    auto p = U(rng);
    if (p < insertProb)
    {
      double k = U(rng), v = U(rng);
      mine.emplace(std::move(k), std::move(v));
      truth.emplace()
    }
    else
    {
      
    }
    
    
    
    
    
    
  }
  
  
  
  
}







































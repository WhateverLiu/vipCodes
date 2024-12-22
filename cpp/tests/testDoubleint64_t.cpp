// [[Rcpp::plugins(cpp20)]]
#include <Rcpp.h>
using namespace Rcpp;
#include <bitset>


// [[Rcpp::export]]
void test(double y) 
{
  // auto y = std::bitset<64>(x);
  for (int i = 0; i < 100; ++i)
  {
    y += i;
    std::cout << std::bitset<64>(y) << "\n";
    y += 1;
    std::cout << std::bitset<64>(y) << "\n";
  }
}


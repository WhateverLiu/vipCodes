#include <Rcpp.h>
using namespace Rcpp;



// [[Rcpp::export]]
int timesTwo(int x) 
{
  std::vector<std::vector<int> > v(x);
  static_assert(std::is_same< std::vector<int>, decltype(v)::value_type >::value) ;
  std::vector<decltype(v)::value_type> u(x);
  return 1;
}



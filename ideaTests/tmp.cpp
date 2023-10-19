#include <Rcpp.h>
using namespace Rcpp;



// [[Rcpp::export]]
void test() {
  
  std::vector<std::vector<char> > x;
  x.emplace_back(std::vector<char>());
  Rcout << x.size() << "\n";
}



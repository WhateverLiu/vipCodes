#include <Rcpp.h>
using namespace Rcpp;
#include "tmph/tmp.hpp" 


// [[Rcpp::export]]
double test(NumericVector x) {
  return g(&x[0], &*x.end());
}


// [[Rcpp::export]] 
double test2(NumericVector x) {
  return std::inner_product(x.begin(), x.end(), x.begin(), 0.0);
}


// [[Rcpp::export]]
void test3() {
  std::srand(std::time());
  Rcout << std::random() << "\n";
}


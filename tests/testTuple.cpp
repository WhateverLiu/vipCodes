// [[Rcpp::plugins(cpp20)]]
#include <Rcpp.h>
using namespace Rcpp;

// This is a simple example of exporting a C++ function to R. You can
// source this function into an R session using the Rcpp::sourceCpp 
// function (or via the Source button on the editor toolbar). Learn
// more about Rcpp at:
//
//   http://www.rcpp.org/
//   http://adv-r.had.co.nz/Rcpp.html
//   http://gallery.rcpp.org/
//

// [[Rcpp::export]]
double test(NumericVector x, NumericVector y) 
{
  std::vector<int> a(x.begin(), x.end()), b(y.begin(), y.end());
  std::tuple<std::vector<int>, std::vector<int> > p(a, b);
  std::fill(std::get<0>(p).begin(), std::get<0>(p).end(), 0);
  std::fill(std::get<1>(p).begin(), std::get<1>(p).end(), 1);
  return std::accumulate(a.begin(), a.end(), 0.0) + 
    std::accumulate(b.begin(), b.end(), 0.0);
    std::accumulate(std::get<0>(p).begin(), std::get<0>(p).end(), 0.0) + 
    std::accumulate(std::get<1>(p).begin(), std::get<1>(p).end(), 0.0);
}


// You can include R code blocks in C++ files processed with sourceCpp
// (useful for testing and development). The R code will be automatically 
// run after the compilation.
//



















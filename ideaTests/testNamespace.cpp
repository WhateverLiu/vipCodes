// [[Rcpp::plugins(cpp20)]]
#include <Rcpp.h>
using namespace Rcpp;
#include <thread>


struct Chthread: public std::thread
{
  std::size_t tid;
  Chthread(std::size_t tid) noexcept: tid(tid) {}
};


namespace Temp
{
std::vector<char> x;
}


double f(int x)
{
  Chthread::
}


// struct A
// {
//   
// };


// [[Rcpp::export]]
double test() {
  unsigned n = std::thread::hardware_concurrency();
  Rcout << n << "\n";
  Temp::x.resize(1000 * 1000);
  auto rst = std::accumulate(Temp::x.begin(), Temp::x.end(), 0.0);
  Rcout << "Temp::x.data() = " << (std::size_t)Temp::x.data() << "\n";
  std::vector<char> y(1000, 3.0);
  Rcout << "y.data() = " << (std::size_t)y.data() << "\n";
  y = std::move(Temp::x);
  Rcout << "Temp::x.size() = " << Temp::x.size() << "\n";
  Rcout << "y.data() = " << (std::size_t)y.data() << "\n";
  std::fill(y.begin(), y.end(), 0.1);
  auto rsty = std::accumulate(y.begin(), y.end(), 0.0);
  return rst + rsty;
}


// You can include R code blocks in C++ files processed with sourceCpp
// (useful for testing and development). The R code will be automatically 
// run after the compilation.
//




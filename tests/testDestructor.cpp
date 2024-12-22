// [[Rcpp::plugins(cpp20)]]
#include <Rcpp.h>
using namespace Rcpp;


struct A
{
  double a, *s;
  A() { a  = 10; s = new double [20]; std::cout << "constructor called.\n"; }
  ~A() { delete [] s; std::cout << "destructor called.\n"; }
};


// [[Rcpp::export]]
void test() 
{
  A a;
  std::cout << &a.a << "\n";
  auto b = std::move(a);
  std::cout << &b.a << "\n";
  std::memset((void*)&a, 0, sizeof(a));
}














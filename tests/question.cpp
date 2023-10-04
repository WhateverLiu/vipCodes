#include <Rcpp.h>
#include <random>
using namespace Rcpp;


#define vec std::vector
struct A
{
  std::mt19937 rng;
  std::uniform_real_distribution<double> U;
  A(){}
  A(int sed) 
  {
    rng.seed(sed); 
    U = std::uniform_real_distribution<double>(0, 100000);
  }
  
  
  template <typename T>
  vec<T> get(std::size_t size)
  {
    vec<T> rst(size);
    for (auto &x: rst) x = U(rng);
    return rst;
  }
  
  
  template <typename T>
  vec<vec<T>> get(std::size_t size1, std::size_t size2)
  {
    vec<vec<T>> rst(size1);
    for (auto &x: rst) get<T> (size2).swap(x);
    return rst;
  } 
  
  
  template <typename T>
  vec<vec<vec<T>>> get(std::size_t size1, std::size_t size2, std::size_t size3)
  {
    vec<vec<vec<T>>> rst(size1);
    for (auto &x: rst) get<T> (size2, size3).swap(x);
    return rst;
  }
  
  
  template <typename T, typename... Args>
  auto magicGet(std::size_t size, Args... args)
  {
    if constexpr (sizeof...(args) == 0)
    {
      vec<T> rst(size);
      for (auto &x: rst) x = U(rng);
      return rst;
    }
    else // The code body must be wrapped inside else {} to ensure the compiler
      // knows they are mutually exclusive.
    {
      vec<decltype(magicGet<T>(args...))> rst(size);
      for (auto &x: rst) magicGet<T>(args...).swap(x);
      return rst;
    }
  }  
  
  
};




// auto u = magicGet(1, 2, 3, 4);
// auto v = magicGet(1, 2, 3, 4, 5);
// u is type vec<vec<vec<vec>>>
// v is type vec<vec<vec<vec<vec>>>>


template <typename X, typename Y, typename Z>
double f(X x, Y y, Z z)
{
  return x * y + z;
}



// [[Rcpp::export]]
void test(int seed) 
{
  
  Rcout << f<int,double>(1.3, 3, 20LL);
  
  
  A a(seed);
  auto x = a.get<float>(3, 2, 5);
  Rcout << (std::size_t)(&x[1][1][1]) << "\n";
  auto y = a.magicGet<float>(3, 2, 5, 7, 8);
  Rcout << std::is_same<vec<vec<vec<vec<vec<float>>>>>, decltype(y)>::value << "\n";
  Rcout << (std::size_t)(&y[1][1][1][1][1]) << "\n";
}





#undef vec

















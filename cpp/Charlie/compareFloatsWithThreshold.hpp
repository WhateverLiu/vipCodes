

// =============================================================================
// Compare floating-point numbers with error threshold.
//   Never assume the comparators in Float are transmissive, i.e. 
//   x == y and y == z => x == z. 
// =============================================================================
template < typename ing, typename num >
struct Float
{
  num val, eps;
  Float ( num val, num eps ): val(val), eps(eps) {}
  ing round() { return std::round(val); }
  ing floor()
  {
    auto valint = std::round(val);
    return std::abs(val - valint) <= eps / 2 * 
      (std::abs(valint) + std::abs(val)) ? valint : std::floor(val);
  }
  ing ceil()
  {
    auto valint = std::round(val);
    return std::abs(val - valint) <= eps / 2 * 
      (std::abs(valint) + std::abs(val)) ? valint : std::ceil(val);
  }
  
  
  friend bool operator <  (Float && x, Float && y) 
  { 
    return y.val - x.val > x.eps / 2 * (std::abs(x.val) + std::abs(y.val)); 
  }
  friend bool operator >  (Float && x, Float && y) 
  { 
    return x.val - y.val > x.eps / 2 * (std::abs(x.val) + std::abs(y.val)); 
  }
  friend bool operator <= (Float && x, Float && y) 
  { 
    return x.val - y.val <= x.eps / 2 * (std::abs(x.val) + std::abs(y.val)); 
  }
  friend bool operator >= (Float && x, Float && y) 
  { 
    return y.val - x.val <= x.eps / 2 * (std::abs(x.val) + std::abs(y.val)); 
  }
  friend bool operator == (Float && x, Float && y) 
  { 
    return std::abs(x.val - y.val) <= 
      x.eps / 2 * (std::abs(x.val) + std::abs(y.val)); 
  }
};


template <typename ing, typename num>
struct CompareFloat
{
  num eps;
  CompareFloat(num eps): eps(eps) {}
  Float<ing, num> operator()(num x) { return Float<ing, num> (x, eps); }
};


// =============================================================================
// // Usage:
// int main()
// {
//   CompareFloat f<int, double>(1e-10);
//   f(3.14) <= f(3.15);
//   f(2.9) > f(7.8);
//   f((3 - 1.9) / 0.3 ).floor();
//   f((3 - 1.9) / 0.3 ).ceil();
// }
// =============================================================================


// // ==========================================================================
// // It has been proved that the above implementation has 0 overhead compared 
// //   to writing it explicitly. Input the following main function in
// //   godbolt.org. The program has the same number of lines of assembly
// //   either by commenting the line with "hi", or the line with "ih."
// // ==========================================================================
// #include <cmath>
// #include <iostream>
// int main()
// {
//   double eps = 1e-10;
//   CompareFloat<int, double> f(eps);
//   double a = rand() - rand(), b = rand() - rand();
//   
//   if (f(a) <= f(b)) std::cout << "hi"; // Comment this line or comment
//   // the other line.
//   
//   if (a - b <= eps / 2 * (std::abs(a) + std::abs(b))) std::cout << "ih";
// }








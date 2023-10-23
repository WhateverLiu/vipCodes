#include "tmp/tmp.hpp"

double g(double *x, double *xend)
{
  return f(x, xend) + std::size_t(x);
}

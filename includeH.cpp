// ============================================================================
# include <setjmp.h>
jmp_buf env;
// longjmp(env, 1); // This line is the breaking point.
// if(setjmp(env)) return List::create(); // Put this line at the beginning of
// the export function.
// ============================================================================
// [[Rcpp::plugins(cpp17)]]
// [[Rcpp::depends(RcppParallel)]]
// # define ARMA_DONT_PRINT_ERRORS
// // [[Rcpp::depends(RcppArmadillo)]]
// # include <RcppArmadillo.h>
# include <Rcpp.h>
# include <RcppParallel.h>
# include <chrono>
# include <fstream>
# include <random>
# include "h/dnyTasking2.hpp"
# include "h/tiktok.hpp"
# include "pcg/pcg_random.hpp"
# define vec std::vector
# define RNG pcg32
using namespace Rcpp;






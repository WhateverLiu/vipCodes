// APIs are all similar to Boost.Python. So, lookup things from that.
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <iostream>
#include "h/meanVarCore.hpp"
namespace py = pybind11;
                      

// =============================================================================
// Take a list of PMFs. A PMF is a list of two numpy flat arrays. The first
// array is value points and the second one is probabilities.
// Compute the means and variances. Return a list of lists of two flat numpy
// arrays, and also zero all elements in the input list.
// The return policy is 'automatic': move for lvalue, copy for rvalue.
// =============================================================================
// [[pycpp::export]]
py::list meanVar ( py::list X ) // There's a bug inside for testing sanitizers.
{
  py::list rst;
  
  
  // for ( py::handle x: X ) // Another way of iterating through X.
  uint64_t firstAddress = 0;
  for ( int k = 0, kend = py::len(X); k < kend; ++k )
  {
    auto lis = py::cast<py::list> (X[k]);
    
    
    // cast<>().request() returns an object containing pointers to the data.
    auto valvec = py::cast<py::array_t<double> >(lis[0]).request();
    auto Pvec   = py::cast<py::array_t<double> >(lis[1]).request();
    int size = valvec.shape[0];
    auto val = (double*)(valvec.ptr);
    auto P   = (double*)(Pvec.ptr);
    
    
    // =========================================================================
    // Showcase the intuitiveness of pybind11/boost.python.
    // =========================================================================
    // auto tmp = py::make_tuple(2); 
    // py::print(tmp);
    
    
    py::array_t<double> mvr(2); // Define shape as (2,)
    double *mvrPtr = (double*)(mvr.request().ptr);
    meanVarCore(val, P, size, mvrPtr[0], mvrPtr[1]);
    
       
    if (k == 0) firstAddress = uint64_t(mvrPtr);
    rst.append(mvr);
  }
  rst.append(firstAddress); 
  return rst;
}


// =============================================================================
// Just another function for computing the Euclidean distance.
// =============================================================================
// [[pycpp::export]]
py::array_t<double> eucD ( py::list X )
{
  // py::list rst;
  py::array_t<double> rst(py::len(X));
  auto rstp = (double*)(rst.request().ptr);
  for ( int k = 0, kend = py::len(X); k < kend; ++k )
  {
    auto lis = py::cast<py::list> (X[k]);
    auto x = py::cast<py::array_t<double> >(lis[0]).request();
    auto y = py::cast<py::array_t<double> >(lis[1]).request();
    int size = x.shape[0];
    auto xp = (double*)(x.ptr);
    auto yp = (double*)(y.ptr);
    double dist = 0;
    for (int i = 0; i < size; ++i)
    {
      double d = xp[i] - yp[i];
      dist += d * d;
    }
    rstp[k] = std::sqrt(dist);
  }
  return rst;
}




 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 


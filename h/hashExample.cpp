#include <Rcpp.h>
using namespace Rcpp;



#define XXH_STATIC_LINKING_ONLY   /* access advanced declarations */
#define XXH_IMPLEMENTATION   /* access definitions */
#include "xxhash.h"


#define vec std::vector
// Return the unique vectors via hashing.


// [[Rcpp::export]]
List uniqueVec(List X) 
{
  
  auto hashFun = [](const vec<int> &v)->std::size_t
  {
    return XXH64(&v[0], sizeof(int) * v.size(), 42);
  };
  
  
  // ===========================================================================
  // This is UNNECESSARY for std::vector! Because the vector class does have ==
  // overloaded!
  // ===========================================================================
  auto equalFun = [](const vec<int> &x, const vec<int> &y)->bool
  {
    if (x.size() != y.size()) return false;
    for (int i = 0, iend = x.size(); i < iend; ++i)
      if (x[i] != y[i]) return false;
    return true;
  };
  
  
  // ===========================================================================
  // unordered_map<keyType, valueType, hashFunType, equalFunType>
  // ===========================================================================
  
  
  // decltype() auto deduces the type of object!
  std::unordered_set<vec<int>, decltype(hashFun), decltype(equalFun)> S(
      13, hashFun, equalFun);
  // Due to custom hashFun and equalFun, the initial bucket count MUST be 
  // specified. In the case where bucket count is not specified, the default
  // bucket count is 11! See example on https://cplusplus.com/reference/unordered_set/unordered_set/load_factor/
  
  
  Rcout << "size = " << S.size() << ", ";
  Rcout << "bucket count = " << S.bucket_count() << "\n";
  
  
  for (int i = 0, iend = X.size(); i < iend; ++i)
  {
    IntegerVector v = X[i];
    S.emplace(vec<int> (v.begin(), v.end()));
  }
  
  
  List rst(S.size());
  int k = 0;
  for (auto it = S.begin(); it != S.end(); ++it, ++k)
  {
    rst[k] = IntegerVector(it->begin(), it->end());
  }
  return rst;
}

#undef vec




/*** R

set.seed(123)
x = lapply(1:10, function(x) sample(sample(3, 1), sample(3, 1), replace = 1))
y = uniqueVec(x)
y



*/






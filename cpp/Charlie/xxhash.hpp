#pragma once


namespace Charlie {

#define XXH_STATIC_LINKING_ONLY   /* access advanced declarations */
#define XXH_IMPLEMENTATION   /* access definitions */
#include "xxhash.h"


inline std::size_t xxhash(const void *x, std::size_t Nbyte, std::size_t seed)
{
  return XXH64(x, Nbyte, seed);
}


inline std::size_t xxhash(const void *x, std::size_t Nbyte)
{
  return XXH3_64bits(x, Nbyte);
}


#undef XXH_IMPLEMENTATION
#undef XXH_STATIC_LINKING_ONLY




}
#pragma once
#include <cstdint>
#include <string> // std::hash


namespace Charlie {


/*
// From https://www.pcg-random.org/download.html
// and https://github.com/imneme/pcg-c-basic/blob/master/pcg_basic.h
// PCG that generates 32-bit unsigned integers.
struct MiniPCG
{
  
  typedef uint32_t result_type; // DO NOT DELETE! WILL BE USED IN std DISTRIBUTION CLASS.
  
  
  std::uint64_t state, inc;
  MiniPCG() { state = 0x853c49e6748fea9b - 42ULL; inc = 0xda3e39cb94b95bdb; }
  
  
  void seed(std::uint64_t s)
  {
    state += s;
    (*this)();
  }
  MiniPCG(std::uint64_t s) { seed(s); }
  
  
  constexpr const static std::uint32_t min() { return 0ul; }
  constexpr const static std::uint32_t max() { return 4294967295ul; } 


  std::uint32_t operator()()
  {
    std::uint64_t oldstate = this->state;
    // Advance internal state
    this->state = oldstate * 6364136223846793005ULL + (this->inc|1);
    // Calculate output function (XSH RR), uses old state for max ILP
    std::uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    std::uint32_t rot = oldstate >> 59u;
    std::uint32_t rst = (xorshifted >> rot) | (xorshifted << ((-rot) & 31u));
    // Rcout << "miniPCG = " << rst << "\n";
    return rst;
  }
};
*/



/**
 * Taken from <https://en.wikipedia.org/wiki/Permuted_congruential_generator>
 * Example code.
 */
struct MiniPCG
{
  
  typedef uint32_t result_type;
  
  
  std::uint64_t state, multiplier, increment;
  
  
  std::uint32_t rotr32(std::uint32_t x, unsigned r)
  {
    return x >> r | x << (-r & 31u);
  }
  
  
  constexpr const static std::uint32_t min() { return 0ul; }
  constexpr const static std::uint32_t max() { return 4294967295ul; }
  
  
  std::uint32_t operator()()
  { 
    std::uint64_t x = state;
    unsigned count = (unsigned)(x >> 59u);		// 59 = 64 - 5
    state = x * multiplier + increment;
    x ^= x >> 18u;								// 18 = (64 - 27)/2
    return rotr32((std::uint32_t)(x >> 27u), count);	// 27 = 32 - 5
  }
  
  
  void reset()
  {
    state = 0x4d595df4d0f33173;
    multiplier = 6364136223846793005ull;
    increment  = 1442695040888963407ull;
  }
  
  
  void seed(std::uint64_t seed)
  { 
    reset();
    state = seed + increment;
    (*this)();
  }
  
  
  MiniPCG() { reset(); }
  MiniPCG(std::uint64_t s){ seed(s); }
  
};




}



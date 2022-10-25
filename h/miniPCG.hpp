#pragma once
#include <cstdint>


// From https://www.pcg-random.org/download.html
// and https://github.com/imneme/pcg-c-basic/blob/master/pcg_basic.h
struct MiniPcg32
{
  
  typedef uint32_t result_type;
  
  
  uint64_t state, inc;
  MiniPcg32() { state = 0x853c49e6748fea9bULL - 42ULL; inc = 0xda3e39cb94b95bdbULL; }
  void seed(uint64_t s)
  {
    state = s + 0x853c49e6748fea9bULL - 42ULL; inc = 0xda3e39cb94b95bdbULL + s;
  }
  MiniPcg32(uint64_t s) { seed(s); }
  
  
  constexpr const static uint32_t min() { return 0ul; }
  constexpr const static uint32_t max() { return 4294967295ul; } 


  uint32_t operator()()
  {
    uint64_t oldstate = this->state;
    // Advance internal state
    this->state = oldstate * 6364136223846793005ULL + (this->inc|1);
    // Calculate output function (XSH RR), uses old state for max ILP
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
  }
};



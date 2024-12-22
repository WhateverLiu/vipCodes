#include <cstdint>
#include <iostream>


struct MiniPCG
{
  std::uint64_t state = 0x4d595df4d0f33173;		// Or something seed-dependent
  constexpr const std::uint64_t multiplier = 6364136223846793005ull;
  constexpr const std::uint64_t increment  = 1442695040888963407ull;	// Or an arbitrary odd constant
  
  
  std::uint32_t rotr32(std::uint32_t x, unsigned r)
  {
    return x >> r | x << (-r & 31u);
  }
  
  
  std::uint32_t operator()()
  { 
    std::uint64_t x = state;
    unsigned count = (unsigned)(x >> 59u);		// 59 = 64 - 5
    state = x * multiplier + increment;
    x ^= x >> 18u;								// 18 = (64 - 27)/2
    return rotr32((std::uint32_t)(x >> 27u), count);	// 27 = 32 - 5
  }
  
  
  void seed(std::uint64_t seed)
  { 
    state = seed + increment;
    (*this)();
  }
  
  
  MiniPCG(){}
  MiniPCG(std::uint64_t s){ seed(s); }
  
};















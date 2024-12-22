#pragma once
#include <chrono>
#include <vector>


/*
// timetype =
// std::chrono::hours,
// std::chrono::minutes,
// std::chrono::seconds,
// std::chrono::milliseconds,
// std::chrono::microseconds,
// std::chrono::nanoseconds
*/


namespace Charlie {


template<typename timetype = std::chrono::microseconds>
struct tiktok
{
  std::vector<std::chrono::time_point<std::chrono::steady_clock> > starts;
  tiktok() { starts.reserve(50); }
  
  
  // Register timestamp.
  std::size_t tik() 
  { 
    starts.emplace_back(std::chrono::steady_clock::now()); 
    return 0; 
  }
  
  
  // Return time passed since registration.
  std::size_t tok()
  {
    std::size_t rst = std::chrono::duration_cast<timetype> (
      std::chrono::steady_clock::now() - starts.back()).count();
    starts.pop_back();
    return rst;
  }
  
  
};


}











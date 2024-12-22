// [[Rcpp::plugins(cpp20)]]
// #include <Rcpp.h>
// using namespace Rcpp;
#include <cstdlib>
#include <iostream>


void* operator new(std::size_t size) {
  std::cout << "Global operator new called, size: " << size << '\n';
  void* ptr = std::malloc(size);
  if (ptr)
    return ptr;
  else
    throw std::bad_alloc{};
}


void operator delete(void* ptr) noexcept {
  std::cout << "Global operator delete called\n";
  std::free(ptr);
} 


// [[Rcpp::export]]
int test() {
  int* p = new int;
  delete p;
  return 0;
}


int main() {
  int* p = new int;
  delete p;
  return 0;
}


/*


 
 
 
 
  
 
*/




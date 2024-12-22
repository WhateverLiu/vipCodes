

// =============================================================================
// Untest anyway.
// =============================================================================
// #include "xxhash.hpp"
// 
// 
// 
// namespace Charlie {
// 
// 
// /**
//  * Brute-force search a perfect hash function. Idea came from
//  * <https://stackoverflow.com/questions/55824130/is-it-possible-to-create-a-minimal-perfect-hash-function-without-a-separate-look/55843687#55843687>
//  */
// struct Phi
// {
//   std::size_t index, hsize;
//   
//   
//   /**
//    * `x[]` is the array of objects to be hashed.
//    */
//   phi(auto *keys, auto size, 
//       std::size_t upscaler = 1,
//       std::size_t Niter = 1000,
//       bool verbose = true)
//   {
//     using ing = decltype(size);
//     using num = std::remove_reference<decltype(*keys)>::type;
//     hsize = upscaler * np2(size);
//     std::vector<bool> b(std::max<unsigned>(1, hsize / 8));
//     std::size_t hsize_1 = hsize - 1;
//     for (; index < Niter; ++index)
//     { 
//       b.assign(b.size(), false);
//       ing i = 0;
//       for (; i < size; ++i) 
//       { 
//         auto mod = xxhash(x[i] + index) & hsize_1;
//         if (b[i]) break;
//         b[i] = true;
//       } 
//       if (i >= size) break;
//     }
//     
//     
//     if (index >= Niter)
//     {
//       std::vector<bool>(0).swap(b);
//       phi(keys, size, upscaler * 2, Niter, verbose);
//     }
//       
//       
//       
//       throw std::runtime_error(
//           "Did not find perfect hash function. "
//           "Try increasing the number of iterations.\n");
//   }
//   
//   
//   /**
//    * Return the necessary size of the lookup table.
//    */
//   std::size_t tableSize() { return hsize; }
//   
//   
//   std::size_t operator()(auto & x)
//   {
//     return xxhash(x + index) & (hsize - 1);
//   }
// };
// 
// 
// }





















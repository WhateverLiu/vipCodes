

/**
 * Find the unique elements in `[x, xend)`. Result is written into the 
 * space `rst[]` is of size at least the number of unique 
 * elements in `[x, xend)`.
 */
template <typename T, typename Hash = std::hash<T>, typename Equa  >
void unique(T* x, T* xend, T* rst)
{
  uint64_t primes[62] = {
    5ull, 11ull, 23ull, 47ull, 97ull, 199ull, 409ull, 823ull, 1741ull,
    3469ull, 6949ull, 14033ull, 28411ull, 57557ull, 116731ull, 236897ull,
    480881ull, 976369ull, 1982627ull, 4026031ull, 8175383ull, 16601593ull,
    33712729ull, 68460391ull, 139022417ull, 282312799ull, 573292817ull,
    1164186217ull, 2364114217ull, 4294967291ull, 8589934583ull, 17179869143ull,
    34359738337ull, 68719476731ull, 137438953447ull, 274877906899ull,
    549755813881ull, 1099511627689ull, 2199023255531ull, 4398046511093ull,
    8796093022151ull, 17592186044399ull, 35184372088777ull, 70368744177643ull,
    140737488355213ull, 281474976710597ull, 562949953421231ull,
    1125899906842597ull, 2251799813685119ull, 4503599627370449ull,
    9007199254740881ull, 18014398509481951ull, 36028797018963913ull,
    72057594037927931ull, 144115188075855859ull, 288230376151711717ull,
    576460752303423433ull, 1152921504606846883ull, 2305843009213693951ull,
    4611686018427387847ull, 9223372036854775783ull, 18446744073709551557ull};
  
  
  auto size = xend - x;
  auto hsize = *std::lower_bound(primes, primes + 62, size);
  auto hf = hashFun();
  vec<T> val; val.reserve(hsize);
  vec<T*> ind(hsize);
  auto xbegin = x;
  for (; x < xend; ++x)
  {
    hf(*x) % hsize
  }
  
  
  
  
  
  
}













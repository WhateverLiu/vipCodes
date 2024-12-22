

struct ThreadIndexLookup
{
  int LOWESTBIT, NBIT;
  std::size_t *begin, *end; // thread id as 64-bit integer.
  uint8_t *u8 = nullptr;
  uint16_t *u16 = nullptr;
  uint32_t *u32 = nullptr;
  uint64_t *u64 = nullptr;
  uint64_t Nthread;
  
  
  ThreadIndexLookup() = default;
  
  
  
  template <typename T>
  T getBlock(std::size_t x, int lowestBit)
  {
    constexpr const nbit = sizeof(T) * 8;
    return (x << (64 - lowestBit - nbit)) >> (64 - nbit);
  }
  
  
  template <typename ing> // ing can be uint8_t, uint16_t
  bool checkBit(int & lowestBit)
  {
    constexpr const int Nbit = sizeof(ing) * 8;
    constexpr const int NmaxUnique = 1 << Nbit;
    std::vector<bool> hit;
    if constexpr (sizeof(ing) <= 2) hit.resize(NmaxUnique);
    std::unordered_set<ing> S;
    lowestBit = 0;
    int lowestBitEnd = 256 - Nbit; 
    bool failed = false;
    for (; lowestBit < lowestBitEnd; ++lowestBit)
    {
      failed = false;
      if constexpr (sizeof ( ing ) <= 2)
      {
        std::fill(hit.begin(), hit.end(), false);
        for (auto x = begin; x < end; ++x)
        {
          auto i = getBlock<ing>(*x, lowestBit);
          if (hit[i]) { failed = true; break; }
          hit[i] = true;
        }
      }
      else
      {
        S.clear();
        for (auto x = begin; x < end; ++x)
        {
          auto i = getBlock<ing>(*x, lowestBit);
          if ( S.find(i) != S.end() ) { failed = true; break; }
          else S.emplace(i);
        } 
      }
      if (!failed) break; 
    }
    return !failed；
  }
  
  
  template <typename ing>
  std::size_t lp(std::thread::id tid)
  {
    uint64_t x;
    std::memcpy(&x, &tid, sizeof(tid));
    return std::find_if(begin, end, [x](const auto & y)->bool
    {
      return getBlock<ing>(x, LOWESTBIT) == y;
    }) - begin; 
  }
  
  
  std::size_t lp8 (std::thread::id tid) { return lp<uint8_t>  (tid); }
  std::size_t lp16(std::thread::id tid) { return lp<uint16_t> (tid); }
  std::size_t lp32(std::thread::id tid) { return lp<uint32_t> (tid); }
  std::size_t lp64(std::thread::id tid) { return lp<uint64_t> (tid); }
  std::size_t (*lookup)(std::thread::id); 
  
  
  template <typename ing>
  void * fillInfo()
  { 
    Nthread = end - begin;
    auto rst = new ing [Nthread];
    int i = 0;
    for (auto x = begin; x < end; ++x, ++i)
      rst[i] = getBlock<ing>(*x, lowestBit);
    LOWESTBIT = lowestBit;
    NBIT = sizeof(ing) * 8;
    
    
    if ( NBIT == 8 ) lookup = lp<uint8_t>;
    else if ( NBIT == 16 ) lookup = lp<uint8_t>;
    else if ( NBIT == 32 ) lookup = lp<uint32_t>;
    else lookup = lp<uint64_t>;
    
    
    return rst;
  }
  
  
  void checkBit ()
  {
    int lowestBit = 0;
    if ( checkBit<uint8_t>(lowestBit) ) 
      u8  = fillInfo<uint8_t>();
    else if ( checkBit<uint16_t>(lowestBit) ) 
      u16 = fillInfo<uint16_t>();
    else if ( checkBit<uint32_t>(lowestBit) ) 
      u32 = fillInfo<uint32_t>();
    else 
      u64 = fillInfo<uint64_t>();
  }
  
  
  
  
  
  
  ThreadIndexLookup(std::thread * begin, std::thread * end)
  {
    static_assert(sizeof(std::thread::id) == 8, 
                  "Size of thread id is not 8 bytes ?!");
    int Nbit = 8;
    for ()
    
    
    
    
  }
};














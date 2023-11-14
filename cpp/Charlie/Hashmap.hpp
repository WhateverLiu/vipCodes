#pragma once
#include "xxhash.hpp"


namespace Charlie {
#define vec std::vector


template <typename Key>
struct SimpleHash
{ 
  bool operator()(const Key &x) { xxhash(&x, sizeof(Key)); }
};
template <typename Key>
struct SimpleEqual
{
  bool operator()(const Key &x, const Key &y) 
  { 
    return std::memcmp(&x, &y, sizeof(Key)) == 0; 
  }
};


template<typename Key, typename Val, 
         typename Hfun = SimpleHash<Key>, 
         typename Efun = SimpleEqual<Key> > 
struct SimpleHashMap
{
  constexpr static const std::size_t nan = std::size_t(0) - 1;
  constexpr static const std::size_t inf = std::size_t(0) - 2;
  constexpr static const std::size_t primes[64] = {
    2ull, 3ull, 5ull, 11ull, 23ull, 47ull, 97ull, 199ull, 409ull, 823ull, 1741ull,
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
  
  
  struct Head { std::size_t begin, last };
  struct Item 
  { 
    std::pair<Key, Val> item; 
    std::size_t next; 
    Item(){}
    Item(item && it, std::size_t nx)
    {
      item = it;
      next = nx;
    }
  };
  
  
  float bucketItemRatio, holePercentThreshold;
  vec<Head> H, Hnew;
  vec<Item> C, Cnew;
  Hfun hf;
  Efun ef;
  std::size_t CNhole;
  VecPool *vp;
  
  
  struct Iterator 
  { 
    Item* it;
    Iterator (){ it = nullptr; }
    Iterator (Item* it): it(it) {}
  };
  
  
  Iterator begin()
  {
    Iterator rst(C.data());
    auto &it = rst.it;
    for (auto end = &*C.end(); it != end and it->next == nan; ++it);
    return rst;
  }
  Iterator end() { return Iterator(&*C.end()); }
  
  
  void operator++(Iterator &iter)
  {
    auto &it = iter.it;
    for (auto end = &*C.end(); it != end and it->next == nan; ++it);
  }
  void operator--(Iterator &iter)
  {
    auto &it = iter.it;
    for (auto end = C.data() - 1; it != end and it->next == nan; --it);
  }
  Iterator operator+(Iterator iter, int64_t k)
  {
    int64_t inc = k > 0 ? 1 : -1;
    for (int64_t i = 0; i != k; i += inc) ++iter;
    return iter;
  }
  Iterator operator-(Iterator iter, int64_t k) { return iter + (-k); }
  
  
  // How to dereference the iterators.
  std::pair<Key, Val>& operator*(Iterator  it) { return it->item; }
  std::pair<Key, Val>& operator->(Iterator it) { return it->item; }
  
  
  std::size_t getAprime(std::size_t size)
  {
    return *std::lower_bound(primes, primes + 64, size);
  }
  
  
  void reset(std::size_t estNitem = 5, 
             float bucketItemRatio = 1.3, 
             float holePercentThreshold = 0.5,
             VecPool *vp = nullptr)
  {
    bucketItemRatio = std::max(1.0f, bucketItemRatio);
    holePercentThreshold = std::min(1.0f, holePercentThreshold);
    this->bucketItemRatio = bucketItemRatio;
    this->holePercentThreshold = holePercentThreshold;
    auto Hsize = getAprime(estNitem * bucketItemRatio);
    this->vp = vp;
    if (vp == nullptr)
    {
      H.resize(Hsize);
      C.resize(Hsize);
    }
    else
    {
      vp->lend<Head>(Hsize).swap(H);
      vp->lend<Head>(0).swap(Hnew);
      vp->lend<Item>(Hsize).swap(C);
      vp->lend<Item>(0).swap(Cnew);
    }
    C.resize(0);
    for (auto &x: H) x.begin = x.last = nan;
    for (auto &x: C) x.next = nan;
    CNhole = 0;
  }
  
  
  ~SimpleHashMapBase()
  {
    if (vp != nullptr)
    {
      vp->recall(Cnew);
      vp->recall(C);
      vp->recall(Hnew);
      vp->recall(H);
    }
  }
  
  
  SimpleHashMapBase(VecPool *vp = nullptr)
  {
    reset(5, 1.3, 0.5, vp);
  }
  SimpleHashMapBase(std::size_t estNitem = 5, 
                    float bucketItemRatio = 1.3, 
                    float holePercentThreshold = 0.5,
                    VecPool *vp = nullptr)
  {
    reset(estNitem, bucketItemRatio, holePercentThreshold, vp);
  }
  
  
  // Return true if chain had been initialized else false.
  // Initialized meaning the bucket has an existing chain.
  // For internal use.
  bool find ( const Key &key, 
              Item *&rst,
              std::size_t& rstPrior, // C[rstPrior].next == the index of rst.
              Head*& hd 
            )
  {
    rstPrior = nan;
    std::size_t h = hf(ky) % H.size();
    hd = &H[h];
    auto i = hd->begin;
    if ( i == nan )
    {
      rst = nullptr;
      return false;
    }
    while ( true )
    {
      if ( eq(ky, C[i].item.first) )
      {
        rst = &C[i];
        return true;
      }
      if (i >= hd->last) break;
      rstPrior = i;
      i = C[i].next;
    } 
    rst = nullptr;
    return true;
  }
  
  
  // User API
  Iterator find ( const Key &key )
  {
    std::pair<Key, Val> *rst; Head* hd; std::size_t rstPrior; // Place holders.
    find(key, rst, rstPrior, tmph);
    return rst == nullptr ? end() : &rst->item;
  }
  
  
  // ===========================================================================
  // The bool component is true if the insertion took place and false if the assignment took place.
  // ===========================================================================
  std::pair<Iterator, bool> insert_or_assign(const Key & ky, Val && vl)
  {
    Item *itemFound; std::size_t rstPrior; Head * headFound; 
    bool initialized = find(ky, itemFound, rstPrior, headFound);
    bool insertionTookPlace = true;
    if (!initialized) // Bucket is empty.
    {
      C.emplace_back(Item(std::pair<Key, Val>(ky, vl), inf));
      headFound->first = C.size() - 1;
      headFound->last = headFound.first;
      itemFound = &C.back();
    }
    else if (itemFound == nullptr) // Bucket is not empty but element is not in chain.
    {
      C.emplace_back(Item(std::pair<Key, Val>(ky, vl), inf));
      auto &currentLast = C[headFound->last];
      headFound->last = C.size() - 1;
      currentLast.next = headFound->last;
      itemFound = &C.back();
    }
    else // Element is in chain
    {
      itemFound->second = vl;
      insertionTookPlace = false;
    }
    return std::pair<Iterator, bool>(Iterator(itemFound), insertionTookPlace);
  }
  
  
  // Return true if the element WAS actually inside the table.
  bool erase(const Key && ky, const bool callDestructor = false)
  {
    Item * itemFound; std::size_t rstPrior; Head * headFound;
    find(ky, itemFound, rstPrior, headFound);
    if ( itemFound == nullptr ) return false;
    if ( headFound->last == headFound->first ) // Only this element is in chain.
      headFound->first = nan;
    // At least 2 elements in chain:
    else if (rstPrior == nan) // The found element is the first element in chain.
      headFound->begin = itemFound->next;
    else if (itemFound - C.data() == headFound->last) // The found element is the last element in chain. 
      headFound->last = rstPrior;
    else
      C[rstPrior].next = itemFound->next;
    CNhole += 1;
    itemFound->next = nan; // Indicate this Item is invalid.
    if (callDestructor) itemFound->item.~std::pair<Key, Val>();
  }
  
  
  // void reorg(const bool downsizeHead = false) // No rehash.
  // {
  //   auto Hsize = H.size();
  //   auto CNfilled = C.size() - CNhole;
  //   auto HsizeNew = getAprime(CNfilled * bucketItemRatio);
  //   if (HsizeNew > Hsize or downsizeHead) H.resize(HsizeNew);
  //   Hnew.resize(HsizeNew);
  //   Cnew.resize(CNfilled);
  //   
  // }
  
};






#undef vec
  
  
  
  
  
  
  
  
  
}
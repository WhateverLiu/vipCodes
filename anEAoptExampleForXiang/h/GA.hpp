# pragma once
#include <random>


namespace Charlie {
// =============================================================================
// Learning rate schedule: LinearLRschedule or ExpLRschedule
// Does not apply to every optimization problem.
// =============================================================================
template<typename ing, typename num>
struct LinearLRschedule
{
  num current, end, delta;
  void set(num start, num end, ing NstepToEnd)
  {
    this->end = end;
    delta = (end - start) / NstepToEnd;
    current = start;
  }
  LinearLRschedule(num start, num end, ing NstepToEnd)
  {
    set(start, end, NstepToEnd);
  }
  LinearLRschedule() {}
  void update()
  {
    current = std::max(end, current + delta);
  }
  num generate() { return current; }  // Read only, thread safe.
};


template<typename ing, typename num>
struct ExpLRschedule
{
  num current, end, delta;
  ExpLRschedule(num start, num end, ing NstepToEnd):
    end(end)
  {
    delta = std::pow(end / start, 1.0 / NstepToEnd);
    current = start;
  }
  ExpLRschedule() {}
  num update() { current = std::max(end, current * delta); }
  num generate() { return current; } // Read only, thread safe.
};
// =============================================================================


template<typename ing, typename GA>
struct GAcmp
{
  GA *x;
  GAcmp() {}
  GAcmp(GA *x): x(x) {}
  bool operator() (ing i, ing j) { return x->getval(i) < x->getval(j); }
};


template<typename ing, typename num, typename GA>
inline void ParaReproduce(
    GA *G, MiniPCG *rngs, ing *id, ing *parent, Charlie::ThreadPool &tp)
{
  tp.parFor(0, G->popuSize() - G->survivalSize(), [&](
      std::size_t objI, std::size_t t)->bool
  {
    G->reproduceTo(parent[objI], id[G->survivalSize() + objI], rngs[objI]);
    return false;    
  }, 1);
}


// ======================================================================================
// The GA class should have the following member functions:
// 1.  void run(ing i): compute the function value of the i_th candidate.

// 2.  num & getval(ing i): return reference to the current function value of the i_th 
//     candidate.

// 3.  void copyTo(ing i, ing j): copy the i_th candidate's content to the j_th candidate.

// 4.  void reproduceTo(ing i, ing j, RNG &rng): Let i_th candidate reproduce a child, 
//     and place it in the container occupied by j. 
//     This amounts to (i) copyTo(i, j), (ii) add noises to candidate j's 
//     parameters, (iii) run(j).

// 5.  ing popuSize(): return population size.

// 6.  ing survivalSize(): return survival population size in each generation.

// 7.  void actionsBetweenGenerations(ing iter): chance to adjust global parameters in GA 
//     after the iter_th generation.

// 8.  std::vector<ing> * corder(): return a pointer to the container for storing 
//     indices of candidates ordered by their function values. The container in the
//     GA class does not need to be initialized.
// ======================================================================================
// ======================================================================================
// Minimization.
// reproduceSelection: deterministic or randomized selection of parents for reproduction.
// 0: deterministic scan. Let every parent produces one child sequentially, and cycle.
// 1: linear prob kernel. 
// 2: inverse kernel.
// Return a numeric vector of vectors. result[i][j] is the function value of the j_th
// best candidate in the i_th generation.
// ======================================================================================
template <typename ing, typename num, typename GA>
std::vector<std::vector<num> > runGAobj(
    GA &initializedG, 
    ing NcandidateToSaveLearningCurve,
    ing maxIter, 
    ing randomSeed, 
    Charlie::ThreadPool &cp, 
    Charlie::VecPool &vp,
    bool verbose = true)
{
  GA &G = initializedG;
  ing popuSize = G.popuSize();
  ing survivalSize = G.survivalSize();
  NcandidateToSaveLearningCurve = std::min(
    survivalSize, NcandidateToSaveLearningCurve);
  auto &id = *G.corder();
  id.resize(popuSize);
  std::iota(id.begin(), id.end(), 0);
  auto parent = vp.lend<ing> (popuSize - survivalSize);
  
  
  auto rngs = vp.lend<MiniPCG>(parent.size());
  for (ing i = 0, iend = rngs.size(); i < iend; ++i)
    rngs[i].seed(randomSeed + 1 + i);
  
  
  auto learningCurve = vp.lend<num> (maxIter, NcandidateToSaveLearningCurve);
  
  
  std::partial_sort(id.begin(), id.begin() + survivalSize, 
                    id.end(), GAcmp<ing, GA>(&G));
  
  
  for (ing k = 0, kend = popuSize - survivalSize; k < kend; )
  {
    for (ing i = 0; i < survivalSize and k < kend; ++i, ++k)
      parent[k] = id[i];
  }
  
  
  for (ing iter = 0; iter < maxIter; ++iter)
  { 
    if (verbose)
    { 
#ifdef Rcpp_hpp 
      Rcpp::Rcout << "Lowest object function value = " << G.getval(id[0]) << "\n";
#else
      std::cout << "Lowest object function value = " << G.getval(id[0]) << "\n";
#endif
    }
    
    
    ParaReproduce<ing, num, GA> (&G, &rngs[0], &id[0], &parent[0], cp);
    
    
    std::partial_sort(id.begin(), id.begin() + G.survivalSize(), 
                      id.end(), GAcmp<ing, GA>(&G));
    
    
    for (ing k = 0; k < NcandidateToSaveLearningCurve; ++k)
      learningCurve[iter][k] = G.getval(id[k]);
    
    
    for (ing k = 0, kend = popuSize - survivalSize; k < kend; )
    {
      for (ing i = 0; i < survivalSize and k < kend; ++i, ++k)
        parent[k] = id[i];
    }
    
    
    G.actionsBetweenGenerations(iter);
  }
  
  
  std::sort(id.begin(), id.end(), GAcmp<ing, GA>(&G));
  
  
  if (verbose) 
  {
#ifdef Rcpp_hpp
    Rcpp::Rcout << "Lowest object function value = " << 
      G.getval(id[0]) << "\n";
#else
    std::cout << "Lowest object function value = " << 
      G.getval(id[0]) << "\n";
#endif
  }
  
  
  vp.recall(rngs);
  vp.recall(parent);
  
  
  return learningCurve;
}




}









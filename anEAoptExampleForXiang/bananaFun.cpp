#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <numeric>
#include <iostream>


#include "h/MiniPCG.hpp" // 弟常用的一个随机数生成器，不深究。
#include "h/isVector.hpp" // 一个内测类是不是STL vector的模板。不深究。
#include "h/mmcp.hpp" // 一个复制内存小函数。不深究。
#include "h/ThreadPool.hpp" // 弟撸的多线程模板。不深究。
#include "h/VecPool.hpp" // 弟撸的内存recycler。不深究。
#include "h/GA.hpp" // 弟的EA模板。可以看看。需要调前5个cpp里的类。


namespace py = pybind11;


#define vec std::vector
#define RNG Charlie::MiniPCG


// 优化如下函数：<https://en.wikipedia.org/wiki/Rosenbrock_function>
// def f(x):
//   S = 0
//   for i in range(len(x) // 2):
//     S += 100 * (x[2 * i] ** 2 - x[2*i + 1]) ** 2 + (x[2*i] -1) ** 2
//   return S
// 弟看了自己写的这个模板，有点复杂，为了最大化优化机会，各个步骤拆得比较细。
// 兄看起累的话，还是自己撸一个吧。
struct BananaFun
{
  int Nsurvival;
  Charlie::LinearLRschedule<int, double> learningR; // Defined in h/GA.hpp.
  // 每一代用的去扰乱当前参数的噪音。一个简单的线性函数。
  
  
  vec<vec<double>> param; // 存参数值。
  vec<double> funval; // 存函数值。
  vec<int> odr;
  
  
  // EA计算器只负责调用这个类里面的member functions.
  
  
  int popuSize() { return funval.size(); } // 必备函数。总人口几多？
  int survivalSize() { return Nsurvival;  } // 必备函数。每一代保留好多？
  
  
  vec<int> *corder() { return &odr; } // 必备函数。返回一个vector指针. 
  // 该vector用于存放人们的id。
  // 优化结束后，vector[0]存的是最优人的id, 
  // vector[vector.size() - 1]是最不优者之id.
  // id 范围是[0, popuSize()-1).
  
  
  void actionsBetweenGenerations(int iter) {} // 必备函数。干一些你想在每一代结束后干的事情。可为空。
  
  
  void run(int i) // 必备函数。告诉EA计算器怎样计算id=i的人的值。
  {
    auto &x = param[i];
    auto &S = funval[i];
    S = 0;
    for (int i = 0, iend = x.size() / 2; i < iend; ++i)
    {
      double tmp = x[2 * i] * x[2 * i] - x[2 * i + 1];
      S += 100 * tmp * tmp + (x[2 * i] - 1) * (x[2 * i] - 1);
    }
  }
  
  
  double & getval (int i) // 必备函数。告诉计算器从哪里读写id=i人的函数值。
  {
    return funval[i];
  }
  
  
  void copyTo(int i, int j) // 必备函数。告诉计算器怎样把人i的所有内容复制到
    // 人j的空间里。
  {
    param[j] = param[i];
    funval[j] = funval[i];
  }
  
  
  // 必备函数。告诉计算器人i是怎样繁衍下一代，并把生出来的东西放进人j的空间里。
  void reproduceTo(int i, int j, RNG &rng)
  {
    copyTo(i, j);
    double learningRate = learningR.generate(); // 内部更新成下一代需要用的噪音。
    std::uniform_real_distribution<double> U(-learningRate, learningRate);
    for (auto &p: param[j]) p += U(rng);
    run(j);
  }
  
  
  BananaFun(){}
  BananaFun(double *initParam, 
            int dim, 
            int popuSize, 
            int survivalSize,
            double iniNoise, 
            double minNoise,
            int maxGen, 
            int NgenerationsTillMinLearningRate):
    Nsurvival(survivalSize)
  {
    param.assign(popuSize, vec<double>(initParam, initParam + dim)); // 初始化。每一个人拥有相同的参数。
    learningR.set(iniNoise, minNoise, NgenerationsTillMinLearningRate);
    funval.resize(popuSize);
    run(0); // 计算人i的值。
    std::fill(funval.begin() + 1, funval.end(), funval.front()); // 把这个值复制给每个人。
  } 
  
  
};



// [[pycpp::export]]
py::dict minimizeBananaFun(
    py::array_t<double> initialParm,
    int popuSize,
    int survivalSize,
    double iniNoise,
    double minNoise,
    int maxGen, 
    int Ngen2minNoise,
    int randomSeed,
    int returnedHistoryLength,
    int maxCore
)
{
  auto initialParmV = initialParm.request();
  auto initialParmPtr = (double*)(initialParmV.ptr);
  int dim = initialParmV.shape[0];
  if (dim % 2 != 0)
  {
    py::print("Dimensionality should be even!");
    return py::list(0);
  }
    
  
  // 生成一个目标函数对象。
  BananaFun objF (
      initialParmPtr, dim, popuSize, survivalSize, iniNoise, minNoise, 
      maxGen, Ngen2minNoise);
  
  
  Charlie::ThreadPool cp(std::move(maxCore));
  Charlie::VecPool vp;
  
  
  Charlie::runGAobj<int, double, BananaFun> (
      objF, returnedHistoryLength, maxGen, randomSeed, cp, vp, true);
  
  
  py::dict rst;
  rst["param"] = py::cast(objF.param[objF.odr[0]]);
  rst["fval"] = objF.funval[objF.odr[0]];
  return rst;
}

















#undef vec
#undef RNG





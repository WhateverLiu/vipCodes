

# LD_PRELOAD="/lib64/libasan.so.5 /lib64/libubsan.so.1"  python


import inspect
import numpy as np
import CharlieSourceCpp as csc
amod = csc.CharlieSourceCpp(
  'bananaFun.cpp', sanitize = False, optFlag = '-Ofast')


howManyCandHistoryToSave = 3 # 保存前几candidates之learning history: function values.
popuSize = 1000 # 人口。
survivalSize = 300 # 样本。
maxGen = 1000 # 一共多少代。
Ngen2minNoise = 700 # 前几代以线性递减learning rate。
maxCore = 1000


initialParm = np.array([10, 11, 12, 13, 14, 15], dtype = np.double)
iniNoise = 2.0 # 最初的噪音。
minNoise = 0.05 # 最终的噪音
randomSeed = 123


# Function optimum: [1, 1, 1, 1, 1, 1]. Opt function value: 0
rst = amod.minimizeBananaFun(
    initialParm,
    popuSize,
    survivalSize,
    iniNoise,
    minNoise,
    maxGen, 
    Ngen2minNoise,
    randomSeed,
    howManyCandHistoryToSave,
    maxCore
)
print(rst)




# 阶梯型下降搜索。
initialParm = np.array([10, 11, 12, 13, 14, 15], dtype = np.double)
iniNoise = 2.0
minNoise = iniNoise / 2
randomSeed = 123
maxGen = 1000 # 一共几代。
Ngen2minNoise = 700 # 前几代以线性递减learning rate。
popuSize = 1000 # 人口。
survivalSize = 300 # 样本。
for k in range(10):
  rst = amod.minimizeBananaFun(
    initialParm,
    popuSize,
    survivalSize,
    iniNoise,
    minNoise,
    maxGen, 
    Ngen2minNoise,
    randomSeed + k,
    howManyCandHistoryToSave,
    maxCore)
  initialParm = np.asarray(rst["param"])
  iniNoise = minNoise
  minNoise = iniNoise / 2

print(rst)
































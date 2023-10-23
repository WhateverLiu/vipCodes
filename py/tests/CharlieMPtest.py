

# Parallel computing with CharlieSourceCpp:

python
import os
import sys
os.chdir('/finance_develop/Charlie/vipCodes/py')
exec(open('CharlieSourceCpp.py').read())
exec(open('CharlieMPlib.py').read())
# exec(open('rfuns.py').read())
import rfuns as r # sys.path.append(dir/of/rfuns.py')


md = CharlieSourceCpp(
  'CharlieSourceCppTests/listOfNumpyArrays.cpp',
  cacheDir = "tempFiles/CharliePycpp",
  sanitize = False, exportModuleOnly = True, rebuild = False)


import numpy as np
def f(siz, commonData):
  lis = []
  for i in range(3):
    val = np.random.uniform(size = (10,))
    p = np.random.uniform(size = (10,))
    p = p / np.sum(p)
    lis.append([val, p])
  rst = md.meanVar(lis)
  return rst


rst = CharliePara(X = [i for i in range(1000)], commonData = None, fun = f, 
  maxNprocess = 15, MPtempDir = 'tempFiles/CharlieMP/C')



















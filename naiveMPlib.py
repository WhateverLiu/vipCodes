

import os
import numpy as np
import shutil
import pickle
import cloudpickle
import subprocess
import datetime
import pytz


################################################################################
# Naive multiprocessing library. It partitions the data and saves them on the
# disk, creates a number of python scripts and invoke shells to execute them.
# Should be easily applicable to distributed systems.
################################################################################
# `function`'s sigature should be :
#
#    function(x, commonData) # commonData will be supplied to para() as well.
#
# where x is the each task's data. 
#
#
# Do not worry about `function`'s dependencies. `cloudpickle` will save all of
# them. `function` can even reference nonlocal data, but this should be 
# frowned upon because `cloudpickle` is slow at saving data like dictionaries.
#
#
# Always ensure that `function` runs in a single thread! For example,
# package `datatable`'s default 16 threads led to a long debugging session.
# ==============================================================================
def para (
  X, commonData, fun, 
  maxNprocess = 15,
  wait = True,
  cleanTempFirst = False,
  keepTempFolder = True, 
  pythonExePath = None
):
  
  
  if not X: return None
  maxNprocess = max(1, min(maxNprocess, len(X)))
  delta = len(X) / maxNprocess
  bounds = sorted(list(set(np.round(np.arange(maxNprocess + 1) * delta).astype(int))))
  if len(bounds) <= 2: return [fun(x, commonData) for x in X]
  
  
  tmpDir = datetime.datetime.now(pytz.timezone("America/New_York")).strftime(
    "%Y-%m-%d-%H-%M-%S")
  tmpDir = "CharlieTempMP-" + tmpDir
  
  
  if cleanTempFirst: 
    for x in os.listdir():
      if "CharlieTempMP-" in x and x.count('-') == 6: 
        shutil.rmtree(x, ignore_errors = True)
  os.makedirs(tmpDir, exist_ok = True)
  
  
  funPath = tmpDir + "/fun.pickle"
  with open(funPath, 'wb') as o: cloudpickle.dump(fun, file = o)
  commonDataPath = tmpDir + "/commonData.pickle"
  with open(commonDataPath, 'wb') as o: pickle.dump(commonData, file = o)
  
  
  inputDir = tmpDir + "/input"
  os.makedirs(inputDir, exist_ok = True)
  startInd = int(wait)
  for i in range(startInd, len(bounds) - 1):
    dat = X[bounds[i]:bounds[i + 1]]
    datPath = inputDir + "/t-" + str(i)
    with open(datPath, 'wb') as o: pickle.dump(dat, file = o)
    
    
  completeDir = tmpDir + "/complete"
  os.makedirs(completeDir, exist_ok = True)
  
  
  scriptDir = tmpDir + "/script"
  os.makedirs(scriptDir, exist_ok = True)
      
      
  for i in range(startInd, len(bounds) - 1):
    codeStr = \
    "import pickle\n" + \
    "with open('"+ tmpDir + "/input/t-" + str(i) + "', 'rb') as o: dat = pickle.load(o)\n" + \
    "with open('"+ tmpDir + "/commonData.pickle', 'rb') as o: commonData = pickle.load(o)\n" + \
    "with open('"+ tmpDir + "/fun.pickle', 'rb') as o: f = pickle.load(o)\n" + \
    "rst = [f(x, commonData) for x in dat]\n" + \
    "with open('"+ tmpDir + "/output/rst-" + str(i) + "-.pickle', 'wb') as o: pickle.dump(rst, o)\n" + \
    "with open('"+ tmpDir + "/complete/f-" + str(i) + "', 'wb') as o: pass\n"
    with open(""+ tmpDir + "/script/script-" + str(i) + ".py", 'w') as o: 
      o.write(codeStr)
  
  
  outDir = tmpDir + "/output"
  os.makedirs(outDir, exist_ok = True)
  
  
  if pythonExePath is None: pythonExePath = "python"
  for i in range(startInd, len(bounds) - 1):
    subprocess.Popen(pythonExePath + " " + tmpDir + "/script/script-" + 
      str(i) + ".py", shell = True)
  
  
  if wait:
    rst = [[fun(x, commonData) for x in X[bounds[0]:bounds[1]]]]
    while len(os.listdir(completeDir)) < len(bounds) - 2: pass
    for i in range(startInd, len(bounds) - 1):
      with open(outDir + '/rst-' + str(i) + '-.pickle', 'rb') as o: d = pickle.load(o)
      rst.append(d)
    if not keepTempFolder: shutil.rmtree(tmpDir, ignore_errors = True)
    return [x for y in rst for x in y]
  
  
  subprocess.Popen(pythonExePath + " " + tmpDir + "/script/script-" + 
    str(0) + ".py", shell = True)
  
  
  # Return a list of file paths.
  return [os.path.abspath(tmpDir + "/output/rst-" + 
            str(i) + "-.pickle").replace("\\", "/") for i in range(
              startInd, len(bounds) - 1)]
  
  
  
  
  

# Example: given an N x P matrix `X` and list `Y` of P x M matrices, multiply `X` 
# and each element in `Y` and sum up the elements.
if False:
  
  
  Y = [np.random.uniform(size = (9, 5)) for _ in range(10000)]
  X = np.random.uniform(size = (100, 9))
  def f(Yi, X): return np.sum(np.matmul(X, Yi))

  
  rst = para(X = Y, commonData = X, fun = f, 
  maxNprocess = 50, wait = True, cleanTempFirst = False, 
  keepTempFolder = True, pythonExePath = None)
    
    
  
  
  













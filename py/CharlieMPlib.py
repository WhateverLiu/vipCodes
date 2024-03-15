import os
import shutil
import pickle
import cloudpickle
import subprocess
import datetime
import pytz
import paramiko
import re
import time


class paraLoadResultUponCompletion:
  def __init__(self, filePaths):
    self.filePaths = filePaths
  def load(self):
    rst = []
    for x in self.filePaths:
      with open(x, 'rb') as o: d = pickle.load(o)
      rst.append(d)
    return rst
    



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
def CharliePara (
  X, commonData, fun, 
  maxNprocess = 15,
  MPtempDir = "../tempFiles/CharlieTempMP/C",
  wait = True,
  pythonExePath = "python3.9"
):
  
  
  if not X: return None
  maxNprocess = max(1, min(maxNprocess, len(X)))
  delta = len(X) / maxNprocess
  bounds = sorted(list(set(np.round(np.arange(maxNprocess + 1) * delta).astype(int))))
  if len(bounds) <= 2: return [fun(x, commonData) for x in X]
  
  
  tmpDir = datetime.datetime.now(pytz.timezone("America/New_York")).strftime(
    "%Y-%m-%d-%H-%M-%S")
  MPtempDir = re.sub('\\\\', '/', os.path.abspath(MPtempDir))
  tmpDir = MPtempDir + '-py-' + tmpDir + '-' + str(os.getpid())
  
  
  os.makedirs(tmpDir, exist_ok = True)
  
  
  funPath = tmpDir + "/fun.pickle"
  with open(funPath, 'wb') as o: cloudpickle.dump(fun, file = o)
  commonDataPath = tmpDir + "/commonData.pickle"
  with open(commonDataPath, 'wb') as o: 
    cloudpickle.dump(commonData, file = o)
  
  
  inputDir = tmpDir + "/input"
  os.makedirs(inputDir, exist_ok = True)
  startInd = int(wait)
  for i in range(startInd, len(bounds) - 1):
    dat = X[bounds[i]:bounds[i + 1]]
    datPath = inputDir + "/t-" + str(i)
    with open(datPath, 'wb') as o: cloudpickle.dump(dat, file = o)
    
    
  completeDir = tmpDir + "/complete"
  os.makedirs(completeDir, exist_ok = True)
  
  
  scriptDir = tmpDir + "/script"
  os.makedirs(scriptDir, exist_ok = True)
      
      
  # Find all modules whose names start with 'CharliePycppModule_', and add
  # its directory.
  modDirs = []
  for x in globals().values():
    if inspect.ismodule(x) and x.__name__[:19] == 'CharliePycppModule_':
      p = os.path.dirname(x.__file__)
      p = p.replace('\\', '/')
      modDirs.append("sys.path.append('" + p + "')")
  modDirs = '\n'.join(modDirs) + '\n\n'
  
  
  for i in range(startInd, len(bounds) - 1):
    codeStr = \
    "import sys, pickle, cloudpickle\n\n" + \
    modDirs + \
    "sys.stderr = open('" + tmpDir + "/log/t-" + str(i) + "-.txt', 'a')\n\n" + \
    "sys.stdout = open('" + tmpDir + "/log/t-" + str(i) + "-.txt', 'a')\n\n" + \
    "with open('"+ tmpDir + "/input/t-" + str(i) + "', 'rb') as o: dat = pickle.load(o)\n\n" + \
    "with open('"+ tmpDir + "/commonData.pickle', 'rb') as o: commonData = pickle.load(o)\n\n" + \
    "with open('"+ tmpDir + "/fun.pickle', 'rb') as o: f = pickle.load(o)\n\n" + \
    "rst = [f(x, commonData) for x in dat]\n\n" + \
    "with open('"+ tmpDir + "/output/rst-" + str(i) + "-.pickle', 'wb') as o: cloudpickle.dump(rst, o)\n\n" + \
    "with open('"+ tmpDir + "/complete/f-" + str(i) + "', 'wb') as o: pass\n\n" + \
    "sys.stdout.close()\n\n" + \
    "sys.stderr.close()\n\n"
    with open(""+ tmpDir + "/script/script-" + str(i) + ".py", 'w') as o: 
      o.write(codeStr)
  
  
  outDir = tmpDir + "/output"
  os.makedirs(outDir, exist_ok = True)
  
  
  logDir = tmpDir + "/log"
  os.makedirs(logDir, exist_ok = True)
  
  
  if pythonExePath is None: pythonExePath = "python"
  for i in range(startInd, len(bounds) - 1):
    # exestr = pythonExePath + " " + tmpDir + "/script/script-" + \
      # str(i) + ".py > " + logDir + "/t-" + str(i) + "-.txt"
    exestr = pythonExePath + " " + tmpDir + "/script/script-" + \
      str(i) + ".py"
    subprocess.Popen(exestr, shell = True)
  
  
  if wait:
    rst = [ [fun(x, commonData) for x in X[bounds[0]:bounds[1]]] ]
    while len(os.listdir(completeDir)) < len(bounds) - 2: pass
    for i in range(startInd, len(bounds) - 1):
      with open(outDir + '/rst-' + str(i) + '-.pickle', 'rb') as o: d = pickle.load(o)
      rst.append(d)
    return [x for y in rst for x in y]
  
  
  subprocess.Popen(pythonExePath + " " + tmpDir + "/script/script-" + 
    str(0) + ".py", shell = True)
  
  
  # Return a list of file paths.
  # ParaNoWaitLoad
  fpaths = [os.path.abspath(tmpDir + "/output/rst-" + 
            str(i) + "-.pickle").replace("\\", "/") for i in range(
              startInd, len(bounds) - 1)]
  return paraLoadResultUponCompletion(fpaths)




# Example: given an N x P matrix `X` and list `Y` of P x M matrices, multiply `X` 
# and each element in `Y` and sum up the elements.
if False:
  import numpy as np
  Y = [np.random.uniform(size = (9, 5)) for _ in range(10000)]
  X = np.random.uniform(size = (100, 9))
  def f(Yi, X): return np.sum(np.matmul(X, Yi))
  rst = para(
    X = Y, commonData = X, fun = f, 
    maxNprocess = 10, wait = False, pythonExePath = None)
    
    
  
  
  



def CharlieParaOnCluster (
  X, commonData, fun, 
  maxNprocess = 15,
  wait = True,
  pythonExePath = "python3.9",
  clusterHeadnodeAddress = "rscgrid139.air-worldwide.com",
  userName = "i56087",
  sshPasswordPath = "pswd/passwd",
  ofilesDir = "Ofiles",
  MPtempDir = "../tempFiles/CharlieTempMP/C",
  jobName = "CH",
  memGBperProcess = 1,
  NthreadPerProcess = 1,
  verbose = True,
  singletonToCluster = False
):
  
  
  if not X: return None
  maxNprocess = max(1, min(maxNprocess, len(X)))
  delta = len(X) / maxNprocess
  bounds = sorted(list(set(np.round(np.arange(maxNprocess + 1) * delta).astype(int))))
  if len(bounds) <= 2 and not singletonToCluster: 
    return [fun(x, commonData) for x in X]
  
  
  tmpDir = datetime.datetime.now(pytz.timezone("America/New_York")).strftime(
    "%Y-%m-%d-%H-%M-%S")
  MPtempDir = re.sub('\\\\', '/', os.path.abspath(MPtempDir))
  tmpDir = MPtempDir + '-py-' + tmpDir + '-' + str(os.getpid())
  
  
  os.makedirs(tmpDir, exist_ok = True)
  ofilesDir = os.path.abspath(ofilesDir)
  os.makedirs(ofilesDir, exist_ok = True)
  
  
  funPath = tmpDir + "/fun.pickle"
  with open(funPath, 'wb') as o: cloudpickle.dump(fun, file = o)
  commonDataPath = tmpDir + "/commonData.pickle"
  with open(commonDataPath, 'wb') as o: cloudpickle.dump(commonData, file = o)
  
  
  inputDir = tmpDir + "/input"
  os.makedirs(inputDir, exist_ok = True)
  for i in range(len(bounds) - 1):
    dat = X[bounds[i]:bounds[i + 1]]
    datPath = inputDir + "/t-" + str(i)
    with open(datPath, 'wb') as o: cloudpickle.dump(dat, file = o)
    
    
  completeDir = tmpDir + "/complete"
  os.makedirs(completeDir, exist_ok = True)
  
  
  scriptDir = tmpDir + "/script"
  os.makedirs(scriptDir, exist_ok = True)
  
  
  # Find all modules whose names start with 'CharliePycppModule_', and add
  # its directory.
  modDirs = []
  for x in globals().values():
    if inspect.ismodule(x) and x.__name__[:19] == 'CharliePycppModule_':
      p = os.path.dirname(x.__file__)
      p = p.replace('\\', '/')
      modDirs.append("sys.path.append('" + p + "')")
  modDirs = '\n'.join(modDirs) + '\n\n'
  
      
  for i in range(len(bounds) - 1):
    codeStr = \
    "import sys, pickle, cloudpickle\n\n" + \
    modDirs + \
    "sys.stderr = open('" + tmpDir + "/log/t-" + str(i) + "-.txt', 'a')\n\n" + \
    "sys.stdout = open('" + tmpDir + "/log/t-" + str(i) + "-.txt', 'a')\n\n" + \
    "with open('"+ tmpDir + "/input/t-" + str(i) + "', 'rb') as o: dat = pickle.load(o)\n\n" + \
    "with open('"+ tmpDir + "/commonData.pickle', 'rb') as o: commonData = pickle.load(o)\n\n" + \
    "with open('"+ tmpDir + "/fun.pickle', 'rb') as o: f = pickle.load(o)\n\n" + \
    "rst = [f(x, commonData) for x in dat]\n\n" + \
    "with open('"+ tmpDir + "/output/rst-" + str(i) + "-.pickle', 'wb') as o: cloudpickle.dump(rst, o)\n\n" + \
    "with open('"+ tmpDir + "/complete/f-" + str(i) + "', 'wb') as o: pass\n\n" + \
    "sys.stderr.close()\n\n" + \
    "sys.stdout.close()\n\n"
    with open(""+ tmpDir + "/script/s-" + str(i) + "-.py", 'w') as o: 
      o.write(codeStr)
  
  
  outDir = tmpDir + "/output"
  os.makedirs(outDir, exist_ok = True)
  
  
  logDir = tmpDir + "/log"
  os.makedirs(logDir, exist_ok = True)
  
  
  if pythonExePath is None: pythonExePath = "python"
      
      
  qsubStrs = []
  workDir = os.getcwd()
  for i in range(len(bounds) - 1):
    qsubStr = "echo 'cd " + workDir + "; " + pythonExePath + " " + tmpDir + \
      "/script/s-" + str(i) + "-.py" + "' | " + \
      "qsub -N '" + jobName + "-" + str(i) + "' -o " + \
      ofilesDir + " -l h_vmem=" + str(memGBperProcess) + "G -pe threads " + \
      str(NthreadPerProcess) + " -j y"
    qsubStrs.append(qsubStr)
  
  
  # load(sshPasswordPath)
  client = paramiko.client.SSHClient()
  client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
  with open(sshPasswordPath, 'r') as p:
    client.connect(
      clusterHeadnodeAddress, username = userName, password = p.read())
    
    
  if verbose: print("Submitting ", len(qsubStrs), " jobs\n")
  for x in qsubStrs:
    if verbose: print(x, "\n")
    while True:
      try: 
        client.exec_command(x)
        time.sleep(0.06)
        break
      except: continue
  client.close()
  
  
  if wait:
    while len(os.listdir(completeDir)) < len(bounds) - 1: pass
    for i in range(len(bounds) - 1):
      with open(outDir + '/rst-' + str(i) + '-.pickle', 'rb') as o: d = pickle.load(o)
      rst.append(d)
    return [x for y in rst for x in y]
  
  
  # Return a list of file paths.
  # ParaNoWaitLoad
  fpaths = [os.path.abspath(tmpDir + "/output/rst-" + str(i) + "-.pickle").replace(
    "\\", "/") for i in range(len(bounds) - 1)]
  return paraLoadResultUponCompletion(fpaths)
  


# ==============================================================================
# Example: given an N x P matrix `X` and list `Y` of P x M matrices, multiply `X` 
# and each element in `Y` and sum up the elements.
# ==============================================================================
if False:
  
  
  import numpy as np
  Y = [np.random.uniform(size = (9, 5)) for _ in range(10000)]
  X = np.random.uniform(size = (100, 9))
  def f(Yi, X): return np.sum(np.matmul(X, Yi))
  
  
  tmp = paraOnCluster (
    X = Y, commonData = X, fun = f, 
    maxNprocess = 10,
    wait = False,
    pythonExePath = "python3.9",
    clusterHeadnodeAddress = "rscgrid139.air-worldwide.com",
    userName = "i56087",
    sshPasswordPath = "pswd/passwd",
    ofilesDir = "Ofiles",
    MPtempDir = "CharlieTempMP/C",
    jobName = "Charlie",
    memGBperProcess = 1,
    NthreadPerProcess = 1,
    verbose = True)
  
  
  
  
  






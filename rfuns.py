

import math
import numpy as np
import scipy.stats as ss
import matplotlib.pyplot as plt
import pandas as pd
import inspect, gc, sys, os, pathlib, cloudpickle, shutil, re
import datatable
import matplotlib.font_manager as pltFontManager
import datetime
import pytz


myTimeZone = timezone = pytz.timezone("America/New_York")


plt.rcParams["pdf.fonttype"] = 42
tmp = "Times New Roman"
if tmp not in pltFontManager.get_font_names(): tmp = 'C059'
plt.rcParams["font.family"] = tmp
del tmp
plt.rcParams['axes.spines.bottom'] = True
plt.rcParams['axes.spines.left'] = True
plt.rcParams['axes.spines.right'] = False
plt.rcParams['axes.spines.top'] = False
# plt.rcParams["figure.figsize"] = (10, 10 * 0.618)


letters = ('a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 
'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z')
LETTERS = ('A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 
'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z')


def valrange(x):
  return np.array([np.min(x), np.max(x)])


def sample(x, k = None, replace = True, p = None):
  '''
  return np.random.choice(x, k, replace, p)
  '''
  b = type(x) is tuple and k is None
  if b: k = np.prod(x)
  if k is None:
    if type(x) is int: k = x
    elif type(x) is np.ndarray and x.shape == (1,): 
      x = k = x[0]
  rst = np.random.choice(x, k, replace, p)
  if b: rst.shape = x
  return rst
  

def runif(shape, low = 0, high = 1):
  '''
  return np.random.uniform(low, high, shape)
  '''
  return np.random.uniform(low, high, shape)


def rnorm(shape, mu = 0, sd = 1):
  '''
  return np.random.normal(mu, sd, shape)
  '''
  return np.random.normal(mu, sd, shape)


def pnorm(q, mu = 0, sd = 1):
  '''
  return ss.norm.cdf(x = q, loc = mu, scale = sd)
  '''
  return ss.norm.cdf(x = q, loc = mu, scale = sd)


def qnorm(p, mu = 0, sd = 1):
  '''
  return ss.norm.ppf(p, loc = mu, scale = sd)
  '''
  return ss.norm.ppf(p, loc = mu, scale = sd)


def seq(begin, end = None, size = None, by = None):
  '''
  if end is None:
    return np.arange(size) * by + begin
  return np.linspace(begin, end, size)
  '''
  if end is None:
    return np.arange(size) * by + begin
  return np.linspace(begin, end, size)


def plot(x = None, y = None, Type = 'p', close = True):
  if close: plt.close()
  ylen = len(y) if type(y) is list else np.prod(y.shape)
  if x is None: x = np.arange(0, ylen)
  if Type == 'p': plt.scatter(x, y)
  else: plt.plot(x, y)
  plt.show()


def hist(x, breaks = None, close = True):
  if close: plt.close()
  plt.hist(x, bins = breaks)
  plt.show()


def lines(x, y = None, close = False):
  if close: plt.close()
  xlen = len(x) if type(x) is list else np.prod(x.shape)
  if y is None: y = np.arange(0, xlen)
  plt.plot(x, y)
  plt.show()


def lapply(X, fun):
  '''
  Apply `fun` to every element in `X`.
  
  If `X` is a list, apply `fun` to every list element, and return a list.
  
  If `X` is a numpy array, try applying `fun` to every element in `X`, and 
  returning a numpy array of the same shape. If this does not work, return
  a list.
  
  If `X` is a pd.DataFrame, apply `fun` to every column of `X`. What comes
  out depends on pd's behavior.
  
  typeX = type(X)
  if typeX is list: return [fun(x) for x in X]
  if typeX is np.ndarray:
    shape = X.shape
    X.shape = (np.prod(shape),)
    try: rst = np.vectorize(fun)(X)
    except: rst = [fun(x) for x in X]
    X.shape = shape
    return rst
  if typeX is pd.DataFrame:
    return X.apply(fun, axis = 0, raw = True)
  return None
  '''
  typeX = type(X)
  if typeX is list: return [fun(x) for x in X]
  if typeX is np.ndarray:
    shape = X.shape
    X.shape = (np.prod(shape),)
    try: rst = np.vectorize(fun)(X)
    except: rst = [fun(x) for x in X]
    X.shape = shape
    return rst
  if typeX is pd.DataFrame:
    return X.apply(fun, axis = 0, raw = True)
  return None
  

if False:
  tmp = pd.DataFrame(np.random.uniform(0, 1, (3, 4)))
  def fun(x): return x + 1
  tmp2 = lapply(tmp, fun)


def apply(X, axis = 0, fun = lambda x: None):
  '''
  `axis` = 0: apply `fun` to every column of matrix/pd.DataFrame `X`.
  
  if X is np.ndarray:
    return np.apply_along_axis(fun, axis, X)
  return X.apply(fun, axis, raw = True)
  '''
  if type(X) is np.ndarray:
    return np.apply_along_axis(fun, axis, X)
  return X.apply(fun, axis, raw = True)


def mapply(fun, *Xs):
  '''
  return [fun(*x) for x in zip(Xs)]
  '''
  return [fun(*x) for x in zip(*Xs)]


def rowSums(X): 
  '''
  return apply(X, 1, np.sum)
  '''
  return apply(X, 1, np.sum)


def colSums(X): 
  '''
  return apply(X, 0, np.sum)
  '''
  return apply(X, 0, np.sum)


def rowMeans(X): 
  '''
  return apply(X, 1, np.mean)
  '''
  return apply(X, 1, np.mean)


def colMeans(X): 
  '''
  return apply(X, 0, np.mean)
  '''
  return apply(X, 0, np.mean)


def rowSDs(X, unbiased = True): 
  '''
  if unbiased:
    def sd(x): return np.std(x, ddof = 1)
  else: sd = np.std
  return apply(X, 1, sd)
  '''
  if unbiased:
    def sd(x): return np.std(x, ddof = 1)
  else: sd = np.std
  return apply(X, 1, sd)


def colSDs(X, unbiased = True): 
  '''
  if unbiased:
    def sd(x): return np.std(x, ddof = 1)
  else: sd = np.std
  return apply(X, 0, sd)
  '''
  if unbiased:
    def sd(x): return np.std(x, ddof = 1)
  else: sd = np.std
  return apply(X, 0, sd)


def eleAllNumpy(l):
  if sum(tuple(type(u) is not np.ndarray for u in l)) == 0:
    return np.asarray(tuple(u for y in l for u in y))
  return None


def unlist(x, recursive = True, to_numpy = True, untuple = True):
  '''
  Assume x could only contains nested lists, scalers, or numpy arrays.
  '''  
  if to_numpy:
    result = eleAllNumpy(x)
    if result is not None: return result
    
    
  def islist(u):
    if untuple: return type(u) in (list, tuple)
    return type(u) is list
    
    
  result = []
  if recursive:
    for u in x:
      if islist(u):
        result.extend(unlist(u, to_numpy = to_numpy))
      else: result.append(u)
    
    
    if to_numpy:
      tmpRst = eleAllNumpy(result)
      if tmpRst is not None: result = tmpRst
      else:
        tmpRst = []
        for u in result:
          if type(u) is np.ndarray: tmpRst.extend(list(u))
          else: tmpRst.append(u)
        result = np.asarray(tmpRst)
    
      
  else:
    for u in x:
      if islist(u): result.extend(u)
      else: result.append(u)
      
      
  return result


# aggregate, match, rm, unlink, copyFiles, listFiles,
# dirCreate, renameFiles, exists, fileExists,
# moveFiles, save, load, sink, basename, dirname, fileExt,
# normPath, strsplit, grepl, gsub, readLines, writeLines
# findCodeLine, union, intersect, setdiff, rbind, cbind, c, t
  

def match(x, y, method = "dt"):
  '''
  x and y are two numpy 1d arrays containing only finite values.  
  
  method = 'dt': use datatable
  method = 'pandas': use pandas
  method = 'numpy': use numpy
  method = 'dict': use hashing.
  '''
  if method == 'dt': # Use datatable
    xdf = datatable.Frame({'val': x})
    ydf = datatable.Frame({'val': y, 'ind': np.arange(y.shape[0]) })[
      :, datatable.min(datatable.f.ind), datatable.by(datatable.f.val)]
    ydf.key = 'val'
    rst = xdf[:, :, datatable.join(ydf)]['ind'].to_numpy()
    return rst.filled(-1 - y.shape[0]).ravel()
  
  
  if method == 'pandas': # Use pandas dataframe.
    xdf = pd.DataFrame({'val': x})
    ydf = pd.DataFrame({'val': y, 'ind': np.arange(y.shape[0]) }).groupby(
      ['val']).min()
    joined = xdf.join(ydf, on = 'val', lsuffix = '_x', rsuffix = '_y')
    rst = joined['ind'].to_numpy()
    rst[np.isnan(rst)] = -1 - y.shape[0]
    return rst.astype(int)
  
  
  rst = np.zeros(x.shape[0], dtype = np.int32) - (y.shape[0] + 1)
  if method == 'numpy':
    yorder = y.argsort()
    ysorted = y[yorder]
    ind = np.searchsorted(ysorted, x)
    outofBound = ind >= y.shape[0]
    ind[outofBound] = 0
    eq = ysorted[ind] == x
    eq[outofBound] = False
    rst[eq] = yorder[ind][eq]
  else: # Hashing.
    D = dict(zip(y[::-1], np.arange(y.shape[0] - 1, -1, -1)))
    for i, u in enumerate(x):
      val = D.get(u)
      if val is not None: rst[i] = val
  return rst




# Test match()
if False:
  import datatable
  import pandas
  import time
  import numpy as np
  
  
  N = int(1e9)
  k = int(1e7)
  x = np.random.choice(N, k)
  y = np.random.choice(N, k)
  timeCosts = {}
  
  
  st = time.time()
  ind = match(x, y, "dt")
  timeCosts['datatable'] = time.time() - st
  np.all(x[ind >= 0] == y[ind[ind >= 0]])
  
  
  st = time.time()
  ind = match(x, y, "pandas")
  timeCosts['pandas'] = time.time() - st
  np.all(x[ind >= 0] == y[ind[ind >= 0]])
  
  
  st = time.time()
  ind = match(x, y, "numpy")
  timeCosts['numpy'] = time.time() - st
  np.all(x[ind >= 0] == y[ind[ind >= 0]])
  
  
  st = time.time()
  ind = match(x, y, "hashing")
  timeCosts['hashing'] = time.time() - st
  np.all(x[ind >= 0] == y[ind[ind >= 0]])




def rm(lis):
  '''
  Remove variables named in the list from the global environment.
  
  gl = globals()
  for x in lis: del gl[x]
  '''
  gl = globals()
  for x in lis: del gl[x]


def rmAll(removeFunctions = False, removeModules = False):
  '''
  gl = globals()
  for x in gl:
    if inspect.isbuiltin(gl[x]) continue
    if (not includeModules) and inspect.ismodule(gl[x]): continue
    if (not includeFunctions) and inspect.isfunction(gl[x]): continue
    del gl[x]
  '''
  gl = globals()
  for x in tuple(gl.keys()):
    if inspect.isbuiltin(gl[x]): continue
    if (not removeModules) and inspect.ismodule(gl[x]): continue
    if (not removeFunctions) and inspect.isfunction(gl[x]): continue
    del gl[x]
    

# ==============================================================================
# File manipulations.
# ==============================================================================
def vecit(p): return (p,) if type(p) is str else p


def getwd(backslash2forward = True):
  '''
  if not backSlashToFoward: return os.getcwd()
  return re.sub('\\\\', '/', os.getcwd())
  '''
  if not backslash2forward: return os.getcwd()
  return re.sub('\\\\', '/', os.getcwd())


def basename(path):
  '''
  rst = [os.path.basename(x) for x in vecit(path)]
  if type(path) is str: return rst[0]
  return rst
  '''
  rst = [os.path.basename(x) for x in vecit(path)]
  if type(path) is str: return rst[0]
  return rst


def dirname(path, backslash2forward = True):
  '''
  rst = [os.path.dirname(x) for x in vecit(path)]
  if backslash2forward:
    for i in range(len(rst)): rst[i] = re.sub('\\\\', '/', rst[i])
  if type(path) is str: return rst[0]
  return rst
  '''
  rst = [os.path.dirname(x) for x in vecit(path)]
  if backslash2forward:
    for i in range(len(rst)): rst[i] = re.sub('\\\\', '/', rst[i])
  if type(path) is str: return rst[0]
  return rst


def file_ext(filename, withDot = False):
  '''
  rst = [pathlib.Path(x).suffix for x in vecit(filename)]
  if not withDot:
    for i in range(len(rst)): rst[i] = rst[i][1:]
  if type(filename) is str: return rst[0]
  return rst
  '''
  rst = [pathlib.Path(x).suffix for x in vecit(filename)]
  if not withDot:
    for i in range(len(rst)): rst[i] = rst[i][1:]
  if type(filename) is str: return rst[0]
  return rst


def normPath(p, backslash2forward = True):
  '''
  rst = [os.path.normpath(x) for x in vecit(p)]
  for i in range(len(rst)):
    rst[i] = re.sub('\\\\', '/', rst[i])
  if type(p) is str: return rst[0]
  return rst
  '''
  rst = [os.path.normpath(x) for x in vecit(p)]
  for i in range(len(rst)):
    rst[i] = re.sub('\\\\', '/', rst[i])
  if type(p) is str: return rst[0]
  return rst


def dircreate(lis, existOK = True):
  '''
  paths = vecit(lis)
  for x in paths: os.makedirs(x, exist_ok = existOK)
  '''
  paths = vecit(lis)
  for x in paths: os.makedirs(x, exist_ok = existOK)


def listfiles(path = '.', recursive = False, 
fullpath = False, normalizePath = False):
  '''
  path_ = vecit(path)
  if recursive:
    rst = [dirpath + '/' + f
      for p in path_ for dirpath, _, filenames in os.walk(p) for f in filenames]
  else: 
    rst = [x for p in path_ for x in os.listdir(p)]    
  if fullpath:
    for i in range(len(rst)): rst[i] = os.path.abspath(rst[i])
  if normalizePath:
    for i in range(len(rst)): rst[i] = os.path.normpath(rst[i])
  return rst
  '''
  path_ = vecit(path)
  if recursive:
    rst = [dirpath + '/' + f
      for p in path_ for dirpath, _, filenames in os.walk(p) for f in filenames]
  else: 
    rst = [p + '/' + x for p in path_ for x in os.listdir(p)]
  if fullpath:
    for i in range(len(rst)): rst[i] = os.path.abspath(rst[i])
  if normalizePath:
    for i in range(len(rst)): rst[i] = os.path.normpath(rst[i])
  return rst
    

def getTimeString():
  '''
  return datetime.datetime.now(myTimeZone).strftime("%Y-%m-%d-%H-%M-%S")
  '''
  return datetime.datetime.now(myTimeZone).strftime("%Y-%m-%d-%H-%M-%S")


def unlink(lis, recycleBin = None, deletePermanently = False):
  '''
  rebin = recycleBin
  if recycleBin is None and not deletePermanently:
    os.makedirs('../recycleBin', exist_ok = True)
    rebin = '../recycleBin'
    filesInBin = set(basename(listfiles(rebin)))
  flist = vecit(lis)
  for x in flist:
    if not deletePermanently:
      tmpfname = rebin + '/' + os.path.basename(x) if x not in \
      filesInBin else rebin + '/' + os.path.basename(x) + '-' + getTimeString()
      shutil.move(x, tmpfname)
    else:
      try: os.remove(x)
      except:
        try: shutil.rmtree(x)
        except: pass
  '''
  rebin = recycleBin
  if recycleBin is None and not deletePermanently:
    os.makedirs('../recycleBin', exist_ok = True)
    rebin = '../recycleBin'
    filesInBin = set(basename(listfiles(rebin)))
  flist = vecit(lis)
  for x in flist:
    if not deletePermanently:
      tmpfname = rebin + '/' + os.path.basename(x) if x not in filesInBin \
        else rebin + '/' + os.path.basename(x) + '-' + getTimeString()
      shutil.move(x, tmpfname)
    else:
      try: os.remove(x)
      except:
        try: shutil.rmtree(x)
        except: pass
    

def filecopy(froms, tos, overwrite = True):
  '''
  Copy files/dirs in `froms` into directories specified in `tos`.
  
  fs, ts = vecit(froms), vecit(tos)
  if len(ts) > 1 and len(ts) != len(fs):
    sys.exit("Number of multiple desinations != number of sources.")
  if len(ts) == 1: ts = tuple(ts[0] for x in fs)
  for x, y in zip(fs, ts):
    if os.path.isdir(x): 
      tmpext = ''.join( sample(c(np.arange(10), letters, LETTERS), 32))
      finalDir = y + '/' + basename(x)
      tmpdir = finalDir + '-' + tmpext
      shutil.copytree(x, tmpdir)
      if overwrite and os.path.exists(finalDir): 
        shutil.rmtree(finalDir)
      shutil.move(tmpdir, finalDir)
    else:
      if overwrite: shutil.copy(x, y) # shutil.copy() overwrites automatically.
  '''
  fs, ts = vecit(froms), vecit(tos)
  if len(ts) > 1 and len(ts) != len(fs):
    sys.exit("Number of multiple desinations != number of sources.")
  if len(ts) == 1: ts = tuple(ts[0] for x in fs)
  for x, y in zip(fs, ts):
    if os.path.isdir(x): 
      tmpext = ''.join( sample(c(np.arange(10), letters, LETTERS), 32))
      finalDir = y + '/' + basename(x)
      tmpdir = finalDir + '-' + tmpext
      shutil.copytree(x, tmpdir)
      if overwrite and os.path.exists(finalDir): 
        shutil.rmtree(finalDir)
      shutil.move(tmpdir, finalDir)
    else:
      if overwrite: shutil.copy(x, y) # shutil.copy() overwrites automatically.
  

def filemove(froms, tos):
  '''
  Move files or directories into directories
  
  froms: a file name as a string or a list of files/directory names
  tos: a directory name as a string or a list of directory names.
  If `tos` is a string, it will be wrapped in as [tos]
  If len(tos) > 1, then len(tos) should be == len(froms).
  
  fs, ts = vecit(froms), vecit(tos)
  if len(ts) > 1 and len(ts) != len(fs):
    sys.exit("Number of multiple desinations != number of sources.")
  if len(ts) == 1: ts = tuple(ts[0] for x in fs)
  for i in range(len(ts)):
    shutil.move(fs[i], ts[i] + '/' + basename(fs[i]))
  '''
  fs, ts = vecit(froms), vecit(tos)
  if len(ts) > 1 and len(ts) != len(fs):
    sys.exit("Number of multiple desinations != number of sources.")
  if len(ts) == 1: ts = tuple(ts[0] for x in fs)
  for i in range(len(ts)):
    shutil.move(fs[i], ts[i] + '/' + basename(fs[i]))


def filerename(froms, tos):
  '''
  fs, ts = vecit(froms), vecit(tos)
  if len(fs) != len(ts): 
    sys.exit("Number of source files != number of destination files")
  for i in range(len(fs)): shutil.move(fs[i], ts[i])
  '''
  fs, ts = vecit(froms), vecit(tos)
  if len(fs) != len(ts): 
    sys.exit("Number of source files != number of destination files")
  for i in range(len(fs)): shutil.move(fs[i], ts[i])
  



# ==============================================================================
# Data wrangling.
# ==============================================================================
def rbind(*args):
  '''
  rbind data frames, series, numpy arrays.
  
  if sum((type(x) is not np.ndarray for x in args)) == 0:
    return np.vstack(args)  
  return pd.concat(tuple(pd.DataFrame(x) for x in args), 
    ignore_index = False, axis = 0)
  '''
  if sum((type(x) is not np.ndarray for x in args)) == 0:
    return np.vstack(args)  
  return pd.concat(tuple(pd.DataFrame(x) for x in args), 
    ignore_index = False, axis = 0)




def cbind(*args):
  '''
  cbind data frames, series, numpy arrays.
  
  if sum((type(x) is not np.ndarray for x in args)) == 0:
    return np.hstack(args)
  return pd.concat(tuple(pd.DataFrame(x) for x in args), 
    ignore_index = False, axis = 1)
  '''
  if sum((type(x) is not np.ndarray for x in args)) == 0:
    return np.hstack(args)
  return pd.concat(tuple(pd.DataFrame(x) for x in args), 
    ignore_index = False, axis = 1)
    

def t(X, returnView = False, returnNumpy = True):
  '''
  X can be a pd.DataFrame or a 2-d numpy array.
  
  if returnView: return X.T
  return X.transpose()
  '''
  if returnView: return X.T
  return X.transpose()


def c(*args):
  '''
    return unlist(args)
  '''
  return unlist(args)


def unique(x, useDatatable = True):
  '''
  x can be pd.Series, list, numpy, etc..
  pd.Series also has member function unique() that uses hashing, but
  they did it in a naive fashion and is often much slower than np.unique
  for small or moderate size data.
  
  return np.unique(x)
  '''
  if useDatatable:
    y = x.to_numpy() if hasattr(x, 'to_numpy') else x
    return datatable.Frame(
      {'y': y})[:, datatable.min(datatable.f.y), datatable.by(
        datatable.f.y)]['y'].to_numpy().ravel()
  return np.unique(x)


def intersect(x, y):
  '''
  x, y can be different types in pd.Series, list, numpy, etc..
  
  return np.intersect(x, y)
  '''
  return np.intersect(x, y)


def union(x, y):
  '''
  x, y can be different types in pd.Series, list, numpy, etc..
  
  return np.union1d(x, y)
  '''
  return np.union1d(x, y)


def setdiff(x, y):
  '''
  x can be pd.Series, list, numpy, etc..
  
  return np.setdiff1d(x, y)
  '''
  return np.setdiff1d(x, y)


def save(D, file):
  '''
  D is a dictionary of pairs of the variable name and the variable itself.
  
  if D is not dict: sys.exit("Input is not a dictionary")
  with open(file, 'wb') as f: cloudpickle.dump(D, f)
  '''
  if type(D) is not dict: sys.exit("Input is not a dictionary")
  with open(file, 'wb') as f: cloudpickle.dump(D, f)


def load(file, namespace = globals()):
  '''
  Load variables into the global environment by default.
  
  with open(file, 'rb') as f: D = cloudpickle.load(f)
  for x in D: namespace[x] = D[x]
  '''
  with open(file, 'rb') as f: D = cloudpickle.load(f)
  for x in D: namespace[x] = D[x]




def writeLines(contentLines, con, option = 'w'):
  '''
  def writeLines(contentLines, con, option = 'w'):
    f = open(con, option)
    if contentLines is str:
      f.writelines(contentLines + '\n')
    else:
      f.writelines([s + '\n' for s in contentLines])
    f.close()
  '''
  f = open(con, option)
  if type(contentLines) is str:
    f.writelines(contentLines + '\n')
  else:
    f.writelines([s + '\n' for s in contentLines])
  f.close()


def readLines(filename):
  '''
  def readLines(filename):
    f = open(filename)
    rst = f.read().splitlines()
    f.close()
    return rst
  '''
  f = open(filename)
  rst = f.read().splitlines()
  f.close()
  return rst


def gsub(pattern, replacement, x):
  '''
  gsub(pattern, replacement, x):
    if type(x) is str: return re.sub(pattern, replacement, x)
    return [re.sub(pattern, replacement, s) for s in x]
  '''
  if type(x) is str: return re.sub(pattern, replacement, x)
  return [re.sub(pattern, replacement, s) for s in x]


def grepl(pattern, x):
  '''
  grepl(pattern, x):
    if type(x) is str: return pattern in x
    return [pattern in s for s in x]
  '''
  if type(x) is str: return pattern in x
  return [pattern in s for s in x]


def strsplit(x, sep = ' '):
  '''
  strsplit(x, sep = ' '):
    if x is str: return x.split(sep)
    return [s.split(sep) for s in x]
  '''
  if type(x) is str: return x.split(sep)
  return [s.split(sep) for s in x]


class Sink:
  '''
  a = runif(100)
  sink = Sink('tmp.txt')
  print(a)
  sink()

  def __init__(self, filename):
    self.stdout = sys.stdout
    self.stderr = sys.stderr
    self.file = open(filename, 'w')
    sys.stdout = self.file
    sys.stderr = self.file
  def __call__(self):
    self.file.close()
    sys.stdout = self.stdout
    sys.stderr = self.stderr
  '''
  def __init__(self, filename):
    self.stdout = sys.stdout
    self.stderr = sys.stderr
    self.file = open(filename, 'w')
    sys.stdout = self.file
    sys.stderr = self.file
  def close(self):
    self.file.close()
    sys.stdout = self.stdout
    sys.stderr = self.stderr


def sink(filename = None):
  gl = globals()
  if filename is not None:
    gl["sinkObject_703016678_501108285"] = Sink(filename)
  else: gl["sinkObject_703016678_501108285"].close()




if False:
  a = runif(100)
  sink = Sink('tmp.txt')
  print(a)
  sink()


# if __init__ == 'main':
#   import numpy as np
#   import scipy.stats as ss
#   import matplotlib.pyplot as plt
#   import pandas as pd
#   import inspect
#   import gc
#   import sys
#   import os
#   import pathlib
#   import cloudpickle
#   import shutil
#   import re





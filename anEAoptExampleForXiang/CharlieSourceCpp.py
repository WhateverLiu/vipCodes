import os
import pybind11
import cloudpickle
import shutil
import platform
import re
import time
import importlib
import pathlib
import subprocess
import inspect
import sys
from sysconfig import get_paths


def getAllHfilesAndMtime(cplr, cppPath, beingSanitized):
  cmd = cplr + ' -MM ' + cppPath
  if beingSanitized: cmd = "unset LD_PRELOAD; " + cmd
  fs = subprocess.check_output(cmd, shell = True).decode('utf-8')
  if platform.system() == "Linux": 
    fs = fs.replace('\n', '').replace('\\', '').split(' ')[1:]
  else: 
    fs = fs.replace('\\', '/').replace('\r', '\n').replace('\n', '').split(' ')[1:]
  fs = [os.path.abspath(x).replace("\\", "/") for x in fs if x != '' and x != '/']
  fmtime = [os.path.getmtime(f) for f in fs]
  return dict(zip(fs, fmtime))




# Load in a dll module built via pybind11
def unloadModule(moduleFullName):
  md = os.path.basename(os.path.splitext(moduleFullName)[0]) # Module name without ext.
  dr = os.path.dirname(moduleFullName).replace("\\","/")
  try: sys.modules.pop(md)
  except: pass
  for x in list(globals().keys()):
    try:
      if globals()[x].__file__ == moduleFullName:
        funNames = [x[0] for x in inspect.getmembers(globals()[x], inspect.isbuiltin)]
        for f in funNames: 
          if f in globals(): del globals()[f]
        del globals()[x]
    except: pass




# Load in a dll module built via pybind11
def loadModule(moduleFullName, loadAllFunsIntoGlobals = True):
  md = os.path.basename(os.path.splitext(moduleFullName)[0]) # Module name without ext.
  dr = os.path.dirname(moduleFullName).replace("\\","/")
  sys.path.append(dr)
  thelib = importlib.import_module(md)
  if loadAllFunsIntoGlobals:
    funNames = [x[0] for x in inspect.getmembers(thelib, inspect.isbuiltin)]
    for x in funNames: globals()[x] = getattr(thelib, x)
  sys.path.pop()
  return thelib 






# ==============================================================================
# To debug with sanitizers, restart python with command in Linux:
#   LD_PRELOAD="/lib64/libasan.so.5  /lib64/libubsan.so.1"  python
#
# /lib64/libasan.so.5 and /lib64/libubsan.so.1 are found by either inspecting
#   `ldd theLibraryYouBuilt.so`, or by checking 
#   `file $(gcc -print-file-name=libasan.so)`.
# 
# Currently sanitizers are only available on Linux.
#
# `rebuild = False` will not trigger a rebuild if the source file that contains
#   // [[pycpp::export]] has no change. It behaves just like Rcpp. So enforce
#   rebuild if anything is changed in some #included files.
#
# Almost never set 'exportModuleOnly' to False! Otherwise during 
# multiprocessing, functions from module cannot be pickled. 
# This is in contrast with R. Always use a different name to be assigned
# to the return value, i.e., md1 = CharlieSourceCpp('file1.cpp'),
# md2 = CharlieSourceCpp('file2.cpp')...
# ==============================================================================
def CharlieSourceCpp(
  
  cppFile, # C++源代码路径。
  
  cacheDir = '../tempFiles/CharliePycpp', # 临时文件夹路径。没事先建立的话函数内部自动生成一个
  
  compilerPath = 'g++', # 编译器路径。Linux/Apple都多半默认加载了系统路径，
  # 于是无需全路径。Windows就不一样，全路径整起。
  
  optFlag = '-Ofast', 
  # 代码优化该多猛，-O1, -O2, -O3, -Ofast, ... 查查吧。弟最多用的是-O2或-Ofast。
  #   -O3理论上要比-O2更猛，但由于优化过度，有时会生成体积更大的二进制，导致综
  #   合速度反而不如-O2. 
  # -Ofast为了提速会违反一些IEEE的浮点运算法则。如果浮点运算中途产生
  #   Inf, NaN等等，用-Ofast死得梆硬 --- 程序不会崩溃，但出来的数字没法
  #   知道对错。
  # 如果事先100%确信程序不会中途产生极值，-Ofast结果会基本一样。“基本”是指，比如
  #   x = 3.14 * 3.14 * 3.14 * 3.14, -Ofast会把它优化成两道乘法
  #   x = (3.14 * 3.14) * (3.14 * 3.14)。结果会跟三连乘有微小区别。
  # -Ofast 大数组运算5x起步。游戏设计全用它。
  
  flags = '-std=gnu++17 -shared -DNDEBUG -Wall -fpic -m64 -march=native -mfpmath=sse -msse2 -mstackrealign',
  # 另一些弟比较在意的编译指令。兄不用深究。默认这些就行。但这个是Linux g++的，
  # 不晓得通用于clang (苹果)不。另一个文件夹里面弟会测测，撸撸
  
  pythonHfileFolder = re.sub('\\\\', '/', get_paths()['include']),
  # Python头文件路径。无需深究。
  
  pybindInclude = re.sub('\\\\', '/', os.path.dirname(pybind11.__file__) + '/include'),
  # pybind11头文件路径。无需深究。
  
  extraIncludePaths = [],
  # 链接外部C++ library路径。目前无需深究。
  
  dllLinkFilePaths = [],
  # 链接外部二进制。目前无需深究。
  
  sanitize = False,
  # 是否debug内存。目前无需深究，但用处极大。弟认为，90%的C++初学者放弃学C++的
  # 原因，就是纠错特难。一旦内存出错，整个垮完。加断点测，很麻烦且完全不可靠，
  # 因为二进制执行顺序不一定是C++代码顺序。另外如果数据内存错误感染到代码内存，
  # 症状会彻底反逻辑，让人想死。以前的工具诸如gdb, valgrind要不用起来麻烦，
  # 要不就会把程序速度减到极慢，查稍微大一点的代码很不现实。sanitizer是google 
  # 2014年才写出来的，对C++ community发展意义重大。自从用了它，弟写C++不再抑郁。
  
  sanitizerFlags = '-g -fno-omit-frame-pointer -fsanitize=address,undefined',
  # 内存纠错的一些旗帜。目前无需深究。
  
  verbose = True,
  # 跑时是否打印过程。推荐默认是。
  
  rebuild = False,
  # 是否重新编译。默认否，即代码变了才重新编译。
  
  dryRun = False,
  # 是否只干跑不编译。默认否。
  
  exportModuleOnly = True, 
  # 是否只返回module. 推荐默认是。否则把函数直接赶进Python环境，极不推荐。
  
  moduleName = None
  # 给一个module的名字。推荐默认无，即用户自定义一个变量名接着该函数返回值。
):
  
  if platform.system() == "Windows" and sanitize:
    print('Sanitizers are not supported by Windows')
    return
  
  
  rebuild |= sanitize 
  cppFullPath = os.path.abspath(cppFile)
  cppFileBasename = os.path.basename(cppFile)
  cppFileDirname = os.path.dirname(cppFullPath)
  os.makedirs(cacheDir, exist_ok = True)
  cacheDirFullPath = os.path.abspath(cacheDir)
  historyFile = cacheDirFullPath + '/history.pickle'
  if platform.system() == "Windows":
    parentFolder = os.path.dirname(pythonHfileFolder)
    dllLinkFilePaths.append(parentFolder + '/' + os.path.basename(parentFolder))
    
  
  cppFileCodes = open(cppFile).read().splitlines()
  funNames = []
  if verbose:
    print(
      "A line of '// [[pycpp::export]]' in the code implies the next line declares " + \
      "a function to be exported.")
  for i in range(1, len(cppFileCodes)):
    if cppFileCodes[i - 1] == '// [[pycpp::export]]':
      x = cppFileCodes[i]
      # ========================================================================
      # Find the first '('. Then scan towards the left. The first nonspace
      # char is the last char of the function name. The last char that is letter
      # or _ or number is the first char of the function name.
      # ========================================================================
      for i in range(len(x)):
        if x[i] == '(': break
      for i in range(i - 1, -1, -1):
        if x[i] != ' ': break
      endCharInd = i + 1 
      for i in range(i - 1, -1, -1):
        a = ord(x[i]) 
        if not(48 <= a <= 57 or 97 <= a <= 122 or 65 <= a <= 90 or a == 95):
          break
      beginCharInd = i + 1
      funNames.append(x[beginCharInd:endCharInd])
  
  
  # ============================================================================
  # Load history if there exists a history file.
  # ============================================================================
  H = {}
  try: H = cloudpickle.load(open(historyFile, 'rb'))
  except: H = {}
  
  
  currentTimeStamps = getAllHfilesAndMtime(compilerPath, cppFullPath, sanitize)
  
  
  if cppFullPath in H:
    noChange = currentTimeStamps == H[cppFullPath]['lastMtimes']
    libIsThere = os.path.exists(H[cppFullPath]['libpath'])
    exportCppIsThere = os.path.exists(H[cppFullPath]['exportCppPath'])
    if (not rebuild) and noChange and exportCppIsThere and sanitize == H[cppFullPath]['sanitize']:
      print('Source files have no changes. No rebuild.')
      unloadModule(H[cppFullPath]['libpath'])
      return loadModule(H[cppFullPath]['libpath'], 
        loadAllFunsIntoGlobals = not exportModuleOnly)
  
  
  if cppFullPath not in H:
    destFullPath = cacheDirFullPath + '/C-' + str(len(H) + 1)
  else:
    unloadModule(H[cppFullPath]['libpath'])
    destFullPath = os.path.dirname(H[cppFullPath]['libpath'])
    

  os.makedirs(destFullPath, exist_ok = True)
  
  
  if moduleName is None:
    moduleName = re.sub('[.]', 'd', 'm' + str(time.time())) # if moduleName is None else moduleName
    if sanitize: moduleName += 'debug'
  moduleName = 'CharliePycppModule_' + moduleName
  
  
  exportedCpp = destFullPath + '/exported.cpp'
  codeLines = [
    '#include "' + cppFullPath + '"', '', '', 
    'PYBIND11_MODULE(' + moduleName + ', m) {', 
    '  m.doc() = "' + moduleName + '";']
  for f in funNames:
    codeLines.append(
      '  m.def("' + f + '", &' + f + ', "A function named ' + f + '.");')
  codeLines.append('}')
  open(exportedCpp, 'w').writelines('\n'.join(codeLines))
  
  
# if sanitize: flags += ' -g '
  flags += ' ' + optFlag + ' '
  cmd = compilerPath + ' ' + flags + ' -I"' + pythonHfileFolder + '" -I"' + \
    pybindInclude + '"'
  if extraIncludePaths:
    cmd += ' -I"'.join(extraIncludePaths) + '"'
  linkCmd = []
  for f in dllLinkFilePaths:
    dn = os.path.dirname(f)
    bn = os.path.basename(f)
    if verbose:
      if bn[:3] == 'lib' or '.' in bn: print(
        'Dynamic linking library name has "lib" or ',
        '[.]. You may need to remove them.',
        sep = ''
        )
    linkCmd.append(' -L"' + dn + '" -l' + bn + ' ')
  linkCmd = ' '.join(linkCmd)
  cmd += linkCmd
  if sanitize: cmd += ' ' + sanitizerFlags + ' '
    
  
  moduleFullName = destFullPath + '/' + moduleName
  if platform.system() == "Windows": moduleFullName += '.pyd'
  else: moduleFullName += '.so'
  
  
  cmd += ' ' + exportedCpp + '  -o ' + moduleFullName
  
  
  if dryRun: return cmd
  if verbose: print(cmd)

  
  if sanitize: tmp = os.system("unset LD_PRELOAD; " + cmd)
  else: tmp = os.system(cmd)
  thelib = loadModule(moduleFullName, loadAllFunsIntoGlobals = not exportModuleOnly)
  
  
  H[cppFullPath] = {}
  H[cppFullPath]['lastMtimes'] = currentTimeStamps
  H[cppFullPath]['exportCppPath'] = exportedCpp
  H[cppFullPath]['libpath'] = moduleFullName
  H[cppFullPath]['sanitize'] = sanitize
  with open(historyFile, 'wb') as f: cloudpickle.dump(H, f)
  
  
  return thelib
























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
  cwd = os.getcwd().replace("\\","/")
  os.chdir(dr)
  thelib = importlib.import_module(md)
  if loadAllFunsIntoGlobals:
    funNames = [x[0] for x in inspect.getmembers(thelib, inspect.isbuiltin)]
    for x in funNames: globals()[x] = getattr(thelib, x)
  os.chdir(cwd)
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
  cppFile,
  cacheDir = '../tempFiles/CharliePycpp',
  compilerPath = 'g++',
  flags = '-std=gnu++17 -shared -DNDEBUG -O2 -Wall -fpic -m64 -march=native -mfpmath=sse -msse2 -mstackrealign',
  pythonHfileFolder = re.sub('\\\\', '/', get_paths()['include']),
  pybindInclude = re.sub('\\\\', '/', os.path.dirname(pybind11.__file__) + '/include'),
  extraIncludePaths = [],
  dllLinkFilePaths = [],
  sanitize = False,
  sanitizerFlags = '-fno-omit-frame-pointer -fsanitize=address,undefined',
  verbose = True,
  rebuild = False,
  dryRun = False,
  exportModuleOnly = True, 
  moduleName = None
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
  
  
  if sanitize: flags += ' -g '
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
























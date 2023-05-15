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
from sysconfig import get_paths


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
# ==============================================================================
def sourceCppCharlie(
  cppFile,
  cacheDir = '../tempFiles/CharliePycpp',
  compilerPath = 'g++',
  flags = '-std=gnu++17 -shared -Ofast -Wall -fpic -m64 -march=native',
  pythonHfileFolder = re.sub('\\\\', '/', get_paths()['include']),
  pybindInclude = re.sub('\\\\', '/', os.path.dirname(pybind11.__file__) + '/include'),
  extraIncludePaths = [],
  dllLinkFilePaths = [],
  sanitize = False,
  sanitizerFlags = '-fsanitize=address,undefined',
  verbose = True,
  rebuild = False,
  dryRun = False,
  exportModuleOnly = False,
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
      "A line of '// [[pycpp::export]]' in the code implies the next line declares ",
      "a function to be exported.",
      sep = "")
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
  if os.path.isfile(historyFile):
    H = cloudpickle.load(open(historyFile, 'rb'))
  else: H = {}
  
  
  if cppFullPath in H:
    tmpFolder = H[cppFullPath]
    if os.path.exists(tmpFolder):
      if os.path.exists(tmpFolder + '/' + cppFileBasename):
        existingFile = open(tmpFolder + '/' + cppFileBasename).read()
        currentFile = open(cppFile).read()
        if (not rebuild) and (existingFile == currentFile):
          print('Source file has no changes. No rebuild')
          extname = '.pyd' if platform.system() == "Windows" else '.so'
          for f in os.listdir(tmpFolder):
            # ==================================================================
            # The cache dir should always have only 1 dynamic library: a file
            # with extension of '.so' or '.pyd'.
            # ==================================================================
            if pathlib.Path(f).suffix == extname:
              curDir = os.getcwd()
              os.chdir(tmpFolder)
              md = os.path.basename(os.path.splitext(f)[0])
              thelib = importlib.import_module(md)
              if not exportModuleOnly:
                for x in funNames: globals()[x] = getattr(thelib, x)
              os.chdir(curDir)
              break
          return thelib;
  else:
    H[cppFullPath] = cacheDirFullPath + '/C-' + str(len(H) + 1)
  destFullPath = H[cppFullPath]
  cloudpickle.dump(
    H, open(cacheDirFullPath + '/history.pickle', 'wb'))
  
  
  try: shutil.rmtree(destFullPath)
  except: pass
  os.makedirs(destFullPath, exist_ok = True)
  shutil.copy(cppFullPath, destFullPath + '/' + cppFileBasename)
  
  
  if sanitize:
    if moduleName is None: moduleName = "debug"
  else:
    moduleName = re.sub('[.]', 'd', 'm' + str(time.time())) if moduleName is None else moduleName
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
  if sanitize:
    cmd += ' -fno-omit-frame-pointer ' + sanitizerFlags
    
    
  cmd += ' ' + exportedCpp + '  -o ' + destFullPath + '/' + moduleName
  
  
  if platform.system() == "Windows": cmd += '.pyd'
  else: cmd += '.so'
  
  
  if dryRun: return cmd
  if verbose: print(cmd)
  try:
    if sanitize: tmp = os.system("unset LD_PRELOAD; " + cmd)
    else: tmp = os.system(cmd)
    curDir = os.getcwd()
    os.chdir(destFullPath)
    ntried = 0 
    while ntried < 10:
      try:
        ntried += 1
        if verbose: print('Load module..')
        thelib = importlib.import_module(moduleName)
        if not exportModuleOnly:
          for x in funNames: globals()[x] = getattr(thelib, x)
        if verbose: print('Success.')
        break
      except: continue
    os.chdir(curDir)
  except: pass
  return thelib




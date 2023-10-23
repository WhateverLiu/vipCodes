

# Whether the two enviornments that contain mtimes are equal.
mtimeEnvEqual <- function(e1, e2)
{
  if (length(e1) != length(e2)) return(F)
  if (!all(names(e1) == names(e2))) return(F)
  for (n in names(e1))
  {
    if (e1[[n]] != e2[[n]]) return(F)
  }; T
}


getRcppRelatedIncludePath <- function(pkgName)
{
  ps = .libPaths()
  for (p in ps)
  {
    rst = paste0(p, '/', pkgName, '/include')
    if (dir.exists(rst)) break
  }; rst
}


# cplr is the full path to the compiler.
# cppPath is the full path to the C++ script.
getAllHfilesAndMtime <- function(cplr, cppPath, beingSanitized)
{
  cmd = paste0(cplr, ' -MM ', cppPath)
  if (beingSanitized) cmd = paste0('unset LD_PRELOAD; ', cmd)
  fs = system(cmd, intern = T)
  fs = gsub(' |[\\]', '', fs)
  file.mtime(fs)
}


# gcc12 is here: /opt/rh/gcc-toolset-12/root/bin/g++
# Without any provision, gcc will search the following directory: run cpp -v 
#   to show. It includes (i) /usr/lib/gcc/x86_64-redhat-linux/8/include,
#   (ii) /usr/local/include, (iii) /usr/include
sourceCppCharlie <- function(
    
    cppFile,
    
    cacheDir = '../tempFiles/CharlieRcpp',
    
    compilerPath = Sys.which('g++'),
    
    flags = '-std=gnu++17 -shared -DNDEBUG -O2 -Wall -fpic -m64 -march=native -mfpmath=sse -msse2 -mstackrealign',
    
    includePaths = c(
      R.home('include'), 
      getRcppRelatedIncludePath('Rcpp'),
      getRcppRelatedIncludePath('RcppEigen')
    ),
    
    dllLinkFilePaths = c(
      # R.home('lib')
    ),
    
    sanitize = FALSE,
    sanitizerFlags = '-fno-omit-frame-pointer -fsanitize=address,undefined',
    verbose = TRUE,
    rebuild = FALSE
)
{
  # All file paths are absolute paths.
  cppFile = normalizePath(cppFile, winslash = '/')
  dir.create(cacheDir, recursive = T, showWarnings = F)
  cacheDir = normalizePath(cacheDir, winslash = '/')
  compilerPath = normalizePath(Sys.which('g++'), winslash = '/')
  includePaths = sapply(includePaths, function(x) normalizePath(x, winslash = '/'))
  dllLinkFilePaths = sapply(dllLinkFilePaths, function(x) normalizePath(x, winslash = '/'))
  
  
  
  H = new.env()
  Hfile = paste0(cacheDir, "/history.RData")
  if (file.exists(Hfile)) load(Hfile)
  currentTimeStamps = getAllHfilesAndMtime(compilerPath, cppFile, sanitize)
  if ( !is.null( H[[cppFile]] ) )
  {
    noChange = mtimeEnvEqual(H[[cppFile]][["lastMtimes"]], currentTimeStamps)
    libIsThere = file.exists(H[[cppFile]][["libpath"]])
    exportCppIsThere = file.exists(H[[cppFile]][['exportCppPath']])
    exportRscriptIsThere = file.exists(H[[cppFile]][['exportRpath']])
    if (!rebuild & noChange & libIsThere & exportCppIsThere & H[[cppFile]][['sanitize']] == sanitize )
    {
      try(dyn.unload(H[[cppFile]][["libpath"]]))
      dyn.load(H[[cppFile]][["libpath"]])
      source(H[[cppFile]][['exportRpath']])
      return(invisible(NULL))
    }
    
    
    # Try unloading dll and remove the directory.
    try({
      dyn.unload(H[[cppFile]][["libpath"]])
      unlink(dirname(H[[cppFile]][["libpath"]]), recursive = T)
      eval(parse(text = paste0('rm("', cppFile, '", envir = H)')))
      })
  }
  
  
  pkgName = paste0('C', length(H) + 1L)
  
  # cat(pkgName, cacheDir, cppFile, "\n")
  
  unlink(paste0(cacheDir, '/', pkgName), recursive = T)
  
  
  # eval(expr = suppressMessages(Rcpp::Rcpp.package.skeleton(
  #   name = pkgName, path = cacheDir, cpp_files = cppFile, example_code = F)),
  #   envir = globalenv())
  
  
  suppressMessages(Rcpp::Rcpp.package.skeleton(
    name = pkgName, path = cacheDir, cpp_files = cppFile, example_code = F, 
    environment = environment()))
  
  
  
  # return(list(pkgName, cacheDir, cppFile))
  
  # print("hello")
  
  pkgPath = paste0(cacheDir, '/', pkgName)
  file.rename(from = paste0(pkgPath, '/R/RcppExports.R'), 
              to = paste0(pkgPath, '/RcppExports.R'))
  file.rename(from = paste0(pkgPath, '/src/RcppExports.cpp'), 
              to = paste0(pkgPath, '/RcppExports.cpp'))
  unlink(paste0(pkgPath, '/', setdiff(
    list.files(pkgPath), c("RcppExports.cpp", "RcppExports.R"))), recursive = T)
  
  
  options(digits.secs = 6)
  dllPath = paste0('m', gsub('-|[.]|:| ', '_', paste0((Sys.time()), '')))
  dllPath = paste0(pkgPath, '/', dllPath)
  dllPath = paste0(dllPath, .Platform$dynlib.ext)
  generatedCpp = readLines(paste0(pkgPath, '/RcppExports.cpp'))
  generatedCpp = generatedCpp[-(which(grepl(
    'static const R_CallMethodDef', generatedCpp)):length(generatedCpp))]
  cppFile2compile = c(paste0('#include "', cppFile, '"'), generatedCpp)
  
  
  cppFile2compilePath = paste0(pkgPath, "/toCompile.cpp")
  writeLines(cppFile2compile, con = cppFile2compilePath)
  if (length(includePaths) != 0)
    includePaths = paste0(paste0('-I"', includePaths, '"'), collapse = ' ')
  else includePaths = ''
  if (length(dllLinkFilePaths) != 0)
  {
    Lpaths = dirname(dllLinkFilePaths)
    lnames = tools::file_path_sans_ext(basename(dllLinkFilePaths))
    Lpaths = paste0(paste0('-L"', Lpaths, '"'), collapse = ' ')
    lnames = paste0(paste0('-l', lnames), collapse = ' ')
  } else 
  {
    Lpaths = ''
    lnames = ''
  }
  
  
  if (sanitize) flags = paste0(flags, ' ', sanitizerFlags)
  cmd = paste0(compilerPath, ' ', 
               flags, ' ', 
               includePaths, ' ',
               Lpaths, ' ',
               lnames)
  cmd = paste0(cmd, ' ', cppFile2compilePath, ' -o ', dllPath) 
  if (sanitize) cmd = paste0("unset LD_PRELOAD; ", cmd)
  if (verbose) message(cmd)
  system(cmd)
  
  
  # dllInfo = dyn.load(dllPath, local = F)
  fenv = new.env()
  source(paste0(pkgPath, '/RcppExports.R'), local = fenv)
  
  
  
  
  H[[cppFile]] = new.env()
  H[[cppFile]][['lastMtimes']] = currentTimeStamps
  H[[cppFile]][['libpath']] = dllPath
  H[[cppFile]][['sanitize']] = sanitize
  H[[cppFile]][['exportCppPath']] = paste0(pkgPath, '/RcppExports.cpp')
  H[[cppFile]][['exportRpath']] = paste0(pkgPath, '/RcppExports.R')
  save(H, file = paste0(cacheDir, '/history.RData'))
}




# Tests.
if (F)
{
  
  setwd('/finance_develop/Charlie/vipCodes/ideaTests')
  source('/finance_develop/Charlie/vipCodes/R/sourceCppCharlie.R')
  tmp = sourceCppCharlie('sanitizerTest.cpp')
  
  
  dyn.unload('/finance_develop/Charlie/vipCodes/tempFiles/CharlieRcpp/C1/m2023_10_22_14_55_11_010094.so')
  dllinfo = dyn.load('/finance_develop/Charlie/vipCodes/tempFiles/CharlieRcpp/C1/m2023_10_22_14_55_11_010094.so')
  
  
  test2 = function(x)
  {
    .Call(getNativeSymbolInfo('_C1_test', dllinfo)$address, x)
  }
  
  
  test2 = Rcpp:::sourceCppFunction(
    func = function(x){}, F, dll = dllinfo, symbol = '_C1_test')
  
  
  test2 = function(x)
  {
    .Call(`_C1_test`, x, PACKAGE = '/finance_develop/Charlie/vipCodes/tempFiles/CharlieRcpp/C1/m2023_10_22_14_55_11_010094.so')
  }
  test2(runif(10))
  
  
  
  
  .Call('_C1_test', runif(10))
  
  
  unlink("/finance_develop/Charlie/vipCodes/tempFiles/CharlieRcpp", 
         recursive = T)
  
  
}


















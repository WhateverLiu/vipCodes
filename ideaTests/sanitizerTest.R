

# Rcpp::compileAttributes(".")

LD_PRELOAD="/lib64/libasan.so.5 /lib64/libubsan.so.1"  R


setwd('/finance_develop/Charlie/vipCodes/ideaTests')
f = "sanitizerTest.cpp"
dir.create("tempFiles", showWarnings = F)
context = .Call("sourceCppContext", PACKAGE = "Rcpp", f, NULL, F, "tempFiles", .Platform)
generatedCpp = c('#include "sanitizerTest.cpp"', context$generatedCpp)
writeLines(generatedCpp, con = "sanitizerTestExport.cpp")
file.edit("sanitizerTestExport.cpp")


if (F)
{
  
  
  unlink("sanitizerTest.so")
  compileCmd = ' g++ -std=gnu++17  -I"/usr/include/R"  -I"/usr/lib64/R/library/Rcpp/include" -I"/finance_develop/Charlie/vipCodes/ideaTests" -I/usr/local/include  -DNDEBUG  -fpic  -O3 -Wall -DNDEBUG -mfpmath=sse -msse2 -mstackrealign  -shared    /finance_develop/Charlie/vipCodes/ideaTests/apkg/src/RcppExports.cpp  -o  sanitizerTest.so '
  system(compileCmd)
  dllInfo = dyn.load('sanitizerTest.so')
  f = .Call('_apkg_test', runif(10)); f
  dyn.unload('sanitizerTest.so')
  
  
}


# Compile without sanitizers.
if (F)
{
  compileCmd = ' g++ -std=gnu++17  -I"/usr/include/R"  -I"/usr/lib64/R/library/Rcpp/include" -I"/finance_develop/Charlie/vipCodes/ideaTests" -I/usr/local/include  -DNDEBUG  -fpic  -O3 -Wall -DNDEBUG -mfpmath=sse -msse2 -mstackrealign  -shared  -L/usr/lib64/R/lib  -lR  sanitizerTestExport.cpp  -o  sanitizerTest.so '
  system(compileCmd)
  dllInfo = dyn.load('sanitizerTest.so')  
  f = .Call('sourceCpp_1_test', runif(10))
  
  
  
}


# compileCmd = 'g++ -std=gnu++17  -I"/usr/include/R"  -I"/usr/lib64/R/library/Rcpp/include" -I"/finance_develop/Charlie/vipCodes/ideaTests" -I/usr/local/include  -fpic -O3 -g -Wall -DNDEBUG -mfpmath=sse -msse2 -mstackrealign  -shared -L/usr/lib64/R/lib  -lR  sanitizerTest.cpp  -o  sanitizerTest.so  '


compileCmd = ' g++ -std=gnu++17  -I"/usr/include/R"  -I"/usr/lib64/R/library/Rcpp/include" -I"/finance_develop/Charlie/vipCodes/ideaTests" -I/usr/local/include  -fno-omit-frame-pointer -DNDEBUG -fsanitize=address,undefined  -fpic -O3 -g -Wall -DNDEBUG -mfpmath=sse -msse2 -mstackrealign  -shared -L/usr/lib64/R/lib  -lR  sanitizerTestExport.cpp  -o  sanitizerTest.so '


system(paste0("unset LD_PRELOAD; ", compileCmd))


dllInfo = dyn.load('sanitizerTest.so')
test = Rcpp:::sourceCppFunction(function(x) {}, FALSE, dllInfo, 'sourceCpp_1_test')
rm(dllInfo)


tmp = runif(100)
test(tmp)





Rcpp::sourceCpp('sanitizerTest.cpp', verbose = 1, rebuild = 1, cacheDir = "tempFiles")



depends = Rcpp:::.getSourceCppDependencies(
  context$depends, f)
Rcpp:::.validatePackages(depends, context$cppSourceFilename)
envRestore <- .setupBuildEnvironment(depends, context$plugins, 
                                     file)


source('rcppSourceCpp.R')
tmp2 = rcppSourceCpp(
    file = f, code = NULL, env = globalenv(), embeddedR = TRUE,
    rebuild = FALSE, cacheDir = getOption("rcpp.cache.dir", tempdir()), 
    cleanupCacheDir = FALSE, showOutput = verbose, verbose = getOption("verbose"), 
    dryRun = FALSE, windowsDebugDLL = FALSE, echo = TRUE)





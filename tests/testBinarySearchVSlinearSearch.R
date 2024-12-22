

source('R/CharlieSourceCpp.R')


CharlieSourceCpp (file = "tests/testBinarySearchVSlinearSearch.cpp", 
                  env = globalenv(), 
                  compilerPath = c("/home/i56087/opt/gcc-13/bin/g++",
                                   Sys.which("g++")), optFlag = "-Ofast", 
                  cppStd = "-std=gnu++20", 
          flags = "-shared -DNDEBUG -Wall -fpic -m64 -march=native -mfpmath=sse -msse2 -mstackrealign", 
          includePaths = c(R.home("include"), getRcppRelatedIncludePath("Rcpp")), 
          dllLinkFilePaths = c(), sanitize = FALSE, 
          sanitizerFlags = "-g -fno-omit-frame-pointer -fsanitize=address,undefined", 
          rebuild = 1, cacheDir = "../tempFiles", verbose = TRUE) 


testInR(123, 100)









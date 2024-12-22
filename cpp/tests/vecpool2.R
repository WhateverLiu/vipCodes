

LD_PRELOAD="/lib64/libasan.so.5 /lib64/libubsan.so.1"  R


source("R/CharlieSourceCpp.R")
CharlieSourceCpp('cpp/tests/vecpool2.cpp', 
                 compilerPath = Sys.which("g++"),
                 cppStd = '-std=gnu++17',
                 sanitize = T, 
                 optFlag = '-O0')
tmp = testVecPool( 123, 1000) 













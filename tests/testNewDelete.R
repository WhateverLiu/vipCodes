

source("R/CharlieSourceCpp.R")
CharlieSourceCpp('tests/testNewDelete.cpp', 
                 compilerPath = "/home/i56087/opt/gcc-13/bin/g++",
                 cppStd = '-std=gnu++20',
                 sanitize = F, 
                 rebuild = F,
                 optFlag = '-O0')







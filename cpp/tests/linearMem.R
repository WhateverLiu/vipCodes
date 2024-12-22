# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/i56087/opt/gcc-13/lib64
# $LD_LIBRARY_PATH


cd  /finance_develop/Charlie/vipCodes


# LD_PRELOAD="/lib64/libasan.so.5 /lib64/libubsan.so.1"  R
LD_PRELOAD="/home/i56087/opt/gcc-13/lib64/libasan.so      /home/i56087/opt/gcc-13/lib64/libubsan.so" R


source("R/CharlieSourceCpp.R")
CharlieSourceCpp('cpp/tests/linearMem.cpp', 
                 # compilerPath = "/home/i56087/opt/gcc-13/bin/g++",
                 # cppStd = '-std=gnu++20',
                 sanitize = 1, 
                 rebuild = 1,
                 optFlag = '-O0'
                 # flags = "-shared -DNDEBUG -Wall -fpic -m64 -march=native -mfpmath=sse -msse2 -mstackrealign"
                 )


# sed = sample(123, 1); sed
sed = 123
system.time({
rst = test(rseed = sed, minBufferSizeMiB = 64, Umax = 10, 
           maxCore = 3, testLinearize = 0)
}); rst
# 3425802131











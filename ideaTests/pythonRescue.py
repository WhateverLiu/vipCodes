

import os
os.system(' g++ -std=gnu++17  -I"/usr/include/R"  -I"/usr/lib64/R/library/Rcpp/include" -I"/finance_develop/Charlie/vipCodes/ideaTests" -I/usr/local/include  -fno-omit-frame-pointer -DNDEBUG -fsanitize=address,undefined  -fpic -O3 -g -Wall -DNDEBUG -mfpmath=sse -msse2 -mstackrealign  -shared -L/usr/lib64/R/lib  -lR  sanitizerTestExport.cpp  -o  sanitizerTest.so ')









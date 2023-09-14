


# R code to test:
Rcpp::sourceCpp('tests/testThreadPool.cpp', verbose = T)
tmp2 = sample(1000L, 3e7, replace= T) 
system.time({cppRst = paraSummation(tmp2, maxCore = 100, grainSize = 100)})
system.time({truth = sum((tmp2 %% 31L + tmp2 %% 131L + tmp2 %% 73L + 
                            tmp2 %% 37L + tmp2 %% 41L) %% 7L)})
cppRst - truth













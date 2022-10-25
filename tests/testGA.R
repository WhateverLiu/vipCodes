

Rcpp::sourceCpp("tests/testGA.cpp")
initxy = runif(2, -10, 10)
tmp = testGA(initxy = initxy, initNoise = 1, minNoise = 1e-5, 
             popuSize = 100, survivalSize = 30, maxGen = 100, 
             Ngen2minNoise = 70, reproduceSelection = "", 
             randomSeed = 42, maxCore = 7)














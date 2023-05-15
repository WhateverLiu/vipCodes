

# ==============================================================================  
# In debugging mode, start Python with sanitizers in a terminal. 
# Unavailable on Windows.
# ==============================================================================  
LD_PRELOAD="/lib64/libasan.so.5 /lib64/libubsan.so.1"  python
# ==============================================================================


# ==============================================================================  
# Normal mode.
# ==============================================================================  
python
# ==============================================================================


exec(open('py/sourceCppCharlie.py').read())


tmp = sourceCppCharlie(
  'cpp/listOfNumpyArrays.cpp', sanitize = True, 
  exportModuleOnly = False, rebuild = True)


import numpy as np
lis = []
for i in range(3):
  val = np.random.uniform(size = (10,))
  p = np.random.uniform(size = (10,))
  p = p / np.sum(p)
  lis.append([val, p])


rst = meanVar(lis); rst


mvr = []
for x in lis:
  m = np.sum(x[0] * x[1])
  mvr.append(np.array([m, np.sum(x[0] * x[0] * x[1]) - m * m]))


# Compare results and check of the return value is a reference to the
# object created in C++ code. Yes.
mvr; rst; rst[0].ctypes.data


# Run the other function in the source file.
eucDrst = eucD(lis); eucDrst









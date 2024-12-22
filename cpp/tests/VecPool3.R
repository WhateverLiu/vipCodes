# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/i56087/opt/gcc-13/lib64
# $LD_LIBRARY_PATH


# LD_PRELOAD="/lib64/libasan.so.5 /lib64/libubsan.so.1"  R
LD_PRELOAD="/home/i56087/opt/gcc-13/lib64/libasan.so.8   /home/i56087/opt/gcc-13/lib64/libubsan.so.1"  R


source("R/CharlieSourceCpp.R")
CharlieSourceCpp('cpp/tests/VecPool3.cpp', 
                 # compilerPath = "/home/i56087/opt/gcc-13/bin/g++",
                 # cppStd = '-std=gnu++20',
                 sanitize = F, 
                 rebuild = T,
                 optFlag = '-Ofast')


useMutex = list()
for (i in 1:30)
{
  useMutex[[i]] = system.time({
    testVecPool(123, 1)
  })
}


useSpinlock = list()
for (i in 1:30)
{
  useSpinlock[[i]] = system.time({
    testVecPool(123, 1)
  })
}


rowMeans(as.data.frame(useMutex))
rowMeans(as.data.frame(useSpinlock))



























cd /home/i56087
sudo git clone https://gcc.gnu.org/git/gcc.git gcc-source
sudo cd gcc-source/
sudo git checkout remotes/origin/releases/gcc-12
sudo ./contrib/download_prerequisites
sudo mkdir gcc-13-build
sudo ../gcc-source/configure --prefix=$HOME/opt/gcc-13 --enable-languages=c,c++,fortran,go --disable-multilib
sudo make -j16
sudo make install






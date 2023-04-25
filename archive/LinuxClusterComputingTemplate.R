# minNobsInCell = 1 ~ 20
# Randomize grid systems. 1 scale. 0.02 ~ 1km, 


# args = commandArgs(trailingOnly = TRUE)
# i = as.integer(args[1])


if(T)
{
  
  
  # It has been tested. Adding small random noise to zero claims does not help.
  dir.create("//RSGrid/finance_develop/Charlie/SVChristian005", showWarnings = F)
  dir.create("//RSGrid/finance_develop/Charlie/SVChristian005/data", showWarnings = F)
  dir.create("//RSGrid/finance_develop/Charlie/SVChristian005/R", showWarnings = F)
  dir.create("//RSGrid/finance_develop/Charlie/SVChristian005/ofiles", showWarnings = F)
  dir.create("//RSGrid/finance_develop/Charlie/SVChristian005/result", showWarnings = F)
  file.copy(from = "R/rfuns.R", to = "//RSGrid/finance_develop/Charlie/SVChristian005/R", overwrite = T)
  file.copy(from = "R/SVChristianLinux.R", to = "//RSGrid/finance_develop/Charlie/SVChristian005/R", overwrite = T)
  
  
  # Data preparation to Linux cluster.
  if(T)
  {
    
    
    load("data/ChristianSV.Rdata")
    dat = ChristianSV
    source('R/rfuns.r')
    binSize = unique(c(seq(10L, 100L, by = 2L), seq(100L, 3200L, by = 100L))) / 100
    set.seed(42)
    tmp = lapply(binSize, function(x)
    {
      Nsamples = max(x ^ 2, 100) # 100 samples (randomizations of lon, lat starts) for each.
      delta = x / 120
      tmp = latinHypercubeSample(lowerBounds = c(-180 - delta / 2, -90 - delta / 2),
                                 upperBounds = c(-180 + delta / 2, -90 + delta / 2),
                                 random = T, Nsamples = Nsamples)
      tmp = data.frame(tmp, binSize = rep(x, nrow(tmp)))
      colnames(tmp)[1:2] = c("lonStart", "latStart"); tmp
    })
    tmp = reconstructData(tmp)
    # minNobsInCell from 1 to 20
    tmp = lapply(1:20, function(x) data.frame(minNobsInCell = x, tmp))
    tmp = reconstructData(tmp); gc()
    tmp2 = keyALGs::splitList(1:nrow(tmp), N = 1000)
    tmp = lapply(tmp2, function(x) tmp[x,])
    computeList = tmp
    mdrIntervalBounds = seq(0, 1 + 1e-10, by = 0.005)
    iccFun = "REDICCrandomizeLonLatStart"
    
    
    save(dat, mdrIntervalBounds, computeList, iccFun, file = "alldata.Rdata", version = 2)
    keyALGs::win2lnxZipCopy(winFiles = "alldata.Rdata", lnxDir = "//RSGrid/finance_develop/Charlie/SVChristian005/data", removeZipOnWin = T, passwdPath = "passwd.Rdata")
    unlink("alldata.Rdata", recursive = T)
    
    
  }
  
  
  # Write submission scripts.
  dir.create("scripts", showWarnings = F) # temp folder.
  for(i in 1L:length(computeList))
  {
    # cat(i, "")
    s = list()
    s[[length(s) + 1]] = '#!/bin/bash'
    # s[[length(s) + 1]] = '#$ -N "SVChristian005SpatialCorrOneBin"'
    s[[length(s) + 1]] = paste0('#$ -N "SVChristian005SpatialCorrOneBin', i, '"')
    s[[length(s) + 1]] = '#$ -o /finance_develop/Charlie/SVChristian005/ofiles'
    s[[length(s) + 1]] = '#$ -l h_rt=00:30:00'
    s[[length(s) + 1]] = '#$ -l h_vmem=2G'
    s[[length(s) + 1]] = '#$ -j y'
    s[[length(s) + 1]] = '#$ -q finance.q'
    s[[length(s) + 1]] = '#$ -P finance'
    s[[length(s) + 1]] = '#$ -pe threads 1'
    s[[length(s) + 1]] = paste0('source /etc/profile; module load gcc-7.3; module load R-4.0.2; cd /finance_develop/Charlie/SVChristian005; Rscript R/SVChristianLinux.R ', i)
    write(paste0(unlist(s), collapse = "\n"), file = paste0("scripts/q", i, ".sh"))
  }
  keyALGs::win2lnxZipCopy(winFiles = "scripts", lnxDir = "//RSGrid/finance_develop/Charlie/SVChristian005")
  unlink("scripts", recursive = T)
  
  
  qsublist = list()
  for(i in 1:length(computeList))
  {
    qsub = paste0("q", i, ".sh")
    qsub = paste0(paste0("dos2unix ", qsub), paste0("; qsub ", qsub))
    qsublist[[i]] = qsub
  }
  qsublist = unlist(qsublist)
  qsubAll = paste0("cd /finance_develop/Charlie/SVChristian005/scripts; ", paste0(qsublist, collapse = "; "))
  
  
  # Submit to Linux cluster.
  if(T)
  {
    load("data/passwd.Rdata")
    session = ssh::ssh_connect("rscgrid1finance1", passwd = passwd)
    rm(passwd); gc()
    # ssh::ssh_exec_wait(session, command = qsubAll)
    tmp = ssh::ssh_exec_internal(session, command = qsubAll)
    ssh::ssh_disconnect(session) 
  }
  
  
  
  
  # Collect results.
  if(T)
  {
    
    
    keyALGs::lnx2winZipCopy(lnxFiles = "//RSGrid/finance_develop/Charlie/SVChristian004/result", winDir = ".")
    resultsPaths = list.files("result", full.names = T)
    results = list()
    for(i in 1:length(resultsPaths))
    {
      cat(i, "")
      load(resultsPaths[[i]])
      results[[i]] = rst
    }
    unlink('result', recursive = T)
    
    
    source("R/rfuns.R")
    rst = reconstructData(results); rm(results); gc()
    save(rst, file = "data/etcREDICC.Rdata")
    
    
    
  }
  
}


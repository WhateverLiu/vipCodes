


#' Naive distributed computing
#' 
#' Naive multiprocessing on shared disk architecture.
#' 
#' @param X  A list or a vector. \code{X[[i]]} is the data demanded by the \code{i}th task.
#' 
#' @param commonData  An R object of any type.
#' 
#' @param fun  An R function with signature \code{fun(X[[i]], commonData)}.
#' 
#' @param maxNprocess  Maximum number of processes to spawn. Default 15.
#' 
#' @param wait  Should the function wait till all processes complete. Default \code{TRUE}.
#' 
#' @param cleanTempFirst  Should the temporary files be cleaned first. Default \code{TRUE}.
#' 
#' @param keepTempFolder  Should the temporary files be kept after execution. Default \code{TRUE}.
#' 
#' @param RscriptExePath  Path to \code{Rscript} executable. If \code{NULL}, 
#' assume \code{Rscript} is executable in the operating system command prompt.
#' 
#' @return A list or a vector of character strings. 
#' 
#' \code{wait = FALSE} returns a list. The \code{i}th element is the result
#' from executing \code{fun} on \code{X[[i]]} and \code{commonData}.
#' 
#' \code{wait = TRUE} returns a vector of character string. The \code{i}th
#' element is the full file path to the would-be-saved result from executing 
#' \code{fun} on \code{X[[i]]} and \code{commonData}. Every file is a 
#' \code{.Rdata} which contains an R object named \code{rst}.
#' 
#' @details  The function will
#' \itemize{
#' 
#' \item Partition \code{X} into \code{P <= maxNprocess}
#' sub-lists and save them separately on disk.
#' 
#' \item Save \code{commonData}, all the loaded library names, and all the
#' function objects in the current R session on disk.
#' 
#' \item Generate \code{P} R scripts, and invoke \code{P}
#' \code{Rscript} subprocesses to run those scripts.
#' }
#' 
#' The function will identify if a function is compiled and linked using 
#' \code{Rcpp}, and track and load the corresponding \code{.dll} or \code{.so} 
#' files in subprocesses.
#' 
#' @example inst/examples/para.R
#'
para <- function(X, commonData, fun, 
                 maxNprocess = 15L, 
                 wait = TRUE,
                 cleanTempFirst = FALSE,
                 keepTempFolder = TRUE, 
                 RscriptExePath = NULL)
{
  if (length(X) == 0) return(NULL)
  
  
  maxNprocess = max(1L, min(maxNprocess, length(X)))
  blocks = unique(as.integer(round(seq(1L, length(X) + 1L, len = maxNprocess + 1L))))
  maxNprocess = min(maxNprocess, length(blocks) - 1L)
  if (maxNprocess <= 1L)
  {
    message("One core finishes the job.\n")
    return(lapply(X, function(x) fun(x, commonData)))
  }
  
  
  if (cleanTempFirst)
  {
    fnames = list.files()
    todelete = fnames[grepl("CharlieTempMP-", fnames)]
    unlink(todelete, recursive = T)
  }
  
  
  tmpDir = paste0(strsplit(
    as.character(Sys.time()), split = "[:]| ")[[1]], collapse = '-')
  tmpDir = paste0('CharlieTempMP-', tmpDir, '-', paste0(strsplit(
    Sys.timezone(), split = '/')[[1]], collapse = '-'))
  dir.create(tmpDir, showWarnings = F)
  
  
  funNames = ls()[sapply(ls(), function(x)
  {
    eval(parse(text = paste0("is.function(", x, ")")))
  })]
  
  
  sourceCppRcodePath = unlist(lapply(getLoadedDLLs(), function(x)
  {
    x = x[["path"]]
    if (!grepl("sourceCpp_", basename(x))) return (NULL)
    folder = dirname(x)
    filesInFolder = list.files(folder, full.names = T)
    filesInFolderExt = tools::file_ext(filesInFolder)
    
    
    # Check if there are exactly one .R and one .cpp file in the same directory.
    if (sum(filesInFolderExt == "cpp" |  filesInFolderExt == "R") != 2L) return(NULL)
    filesInFolder[filesInFolderExt == "R"]
  }))
  if (length(sourceCppRcodePath) != 0) names(sourceCppRcodePath) = NULL
  
  
  loadedLibNames = (.packages())
  
  
  eval(parse(text = paste0(
    "save(", paste0(funNames, collapse = ", "),
    ", loadedLibNames, commonData, ",
    paste0("file = '", tmpDir, "/commonData.Rdata')"))))
  
  
  datDir = paste0(tmpDir, "/input")
  dir.create(datDir, showWarnings = F)
  
  
  Xresv = X
  startInd = 1L + wait # If wait, do not save the first task on disk, but 
  # execute it in the current session.
  for (i in startInd:(length(blocks) - 1L))
  {
    X = Xresv[blocks[i]:(blocks[i + 1] - 1L)]
    fname = paste0(datDir, "/t-", i, "-.Rdata")
    save(X, file = fname)
  }
  X = Xresv; rm(Xresv)
  
  
  rstDir = paste0(tmpDir, "/output")
  dir.create(rstDir, showWarnings = F)
  
  
  completeDir = paste0(tmpDir, "/complete")
  dir.create(completeDir, showWarnings = F)
  
  
  codeDir = paste0(tmpDir, "/script")
  dir.create(codeDir, showWarnings = F)
 
  
  for (i in startInd:(length(blocks) - 1L))
  {
    s = list()
    s[[length(s) + 1]] = paste0("load('", tmpDir, "/input/t-", i, "-.Rdata')")
    s[[length(s) + 1]] = paste0("load('", tmpDir, "/commonData.Rdata')")
    if (length(sourceCppRcodePath) != 0)
    {
      for (x in sourceCppRcodePath) s[[length(s) + 1]] = paste0("source('", x, "')")
    }
    s[[length(s) + 1]] = "for (x in loadedLibNames) eval(parse(text = paste0('library(', x, ')')))"
    s[[length(s) + 1]] = "rst = lapply(X, function(x) fun(x, commonData))"
    s[[length(s) + 1]] = paste0("save(rst, file = '", tmpDir, "/output/rst-", i, "-.Rdata')")
    s[[length(s) + 1]] = paste0("write('', file = '", tmpDir, "/complete/f-", i, "')")
    writeLines(unlist(s), con = paste0(tmpDir, "/script/s-", i, "-.R"))
  }
  
  
  if (is.null(RscriptExePath)) RscriptExePath = "Rscript"
  for (i in startInd:(length(blocks) - 1L))
  {
    system(paste0(RscriptExePath, " ", tmpDir, "/script/s-", i, "-.R"), wait = F)
  }
  
  
  if (wait)
  {
    rst = lapply(X[blocks[1]:(blocks[2] - 1L)], function(x) fun(x, commonData))
    
    
    nfile = length(blocks) - 2L
    while (length(list.files(paste0(tmpDir, "/complete"))) < nfile) {}
    
    
    result = list(rst)
    for (i in 2:(length(blocks) - 1L))
    {
      load(paste0(tmpDir, "/output/rst-", i, "-.Rdata"))
      result[[i]] = rst
    }
    
    
    if (!keepTempFolder) unlink(tmpDir, recursive = T)
    return (unlist(result, recursive = F))
  }
  
  
  # If not to wait, files have already been saved on the disk.
  system(paste0(RscriptExePath, " ", tmpDir, "/script/s-", 1, "-.R"), wait = F)
  paste0(normalizePath(tmpDir, winslash = '/'), "/output/rst-", 
         1:(length(blocks) - 1L), "-.Rdata")
}




# Test
if (F)
{
  # One motivation behind para() is to allow multiprocessing over Rcpp functions
  # compiled and linked in the current R session, without the necessity of
  # tracking and recompilation of those functions in every other process.
  cppFile = "
// [[Rcpp::plugins(cpp17)]]
#include <Rcpp.h>
using namespace Rcpp;
// [[Rcpp::export]]
double stu(NumericVector x) { return std::accumulate(x.begin(), x.end(), 0.0); }
"
  writeLines(cppFile, con = "tmpCpp.cpp")
  Rcpp::sourceCpp("tmpCpp.cpp")
  
  
  # ============================================================================
  # Example I:
  # ============================================================================
  X = lapply(1:10000, function(x) runif(1000))
  y = runif(1000)
  f = function(x, y) { stu(x) + sum(x * y) }
  
  
  rstTruth = lapply(X, function(x) f(x, y))
  
  
  rstPara = para(X, commonData = y, fun = f, maxNprocess = 100, wait = TRUE, 
                 cleanTempFirst = TRUE, keepTempFolder = TRUE,
                 RscriptExePath = NULL)
  
  
  # Compare results:
  cat("Max difference =", max(abs(unlist(rstTruth) - unlist(rstPara))), "\n")
  
  
  
  
  # ============================================================================
  # Example II:
  # ============================================================================
  g = function(x, y) { stu(x) } # Function should always have 2 arguments.
  rstParaNoWait = para(X, commonData = NULL, fun = g, maxNprocess = 30, 
                       wait = FALSE, cleanTempFirst = TRUE, 
                       keepTempFolder = TRUE, RscriptExePath = NULL)
  cat("Results will be in files:\n\n"); for(u in rstParaNoWait) cat(u, "\n")
  
  
  
  
  
}











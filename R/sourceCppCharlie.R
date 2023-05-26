


rcppSourceCpp = function (
  file = "", code = NULL, env = globalenv(), embeddedR = TRUE,
  rebuild = FALSE, cacheDir = getOption("rcpp.cache.dir", tempdir()),
  cleanupCacheDir = FALSE, showOutput = verbose, verbose = getOption("verbose"),
  dryRun = FALSE, windowsDebugDLL = FALSE, echo = TRUE) 
{
  
  
  cacheDir <- path.expand(cacheDir)
  cacheDir <- Rcpp:::.sourceCppPlatformCacheDir(cacheDir)
  cacheDir <- normalizePath(cacheDir)
  if (!missing(code)) {
    rWorkingDir <- getwd()
    file <- tempfile(fileext = ".cpp", tmpdir = cacheDir)
    con <- file(file, open = "w")
    writeLines(code, con)
    close(con)
  }
  else {
    rWorkingDir <- normalizePath(dirname(file))
  }
  file <- normalizePath(file, winslash = "/")
  if (!tools::file_ext(file) %in% c("cc", "cpp")) {
    stop("The filename '", basename(file), "' does not have an ", 
         "extension of .cc or .cpp so cannot be compiled.")
  }
  if (.Platform$OS.type == "windows") {
    if (grepl(" ", basename(file), fixed = TRUE)) {
      stop("The filename '", basename(file), "' contains spaces. This ", 
           "is not permitted.")
    }
  }
  else {
    if (windowsDebugDLL) {
      if (verbose) {
        message("The 'windowsDebugDLL' toggle is ignored on ", 
                "non-Windows platforms.")
      }
      windowsDebugDLL <- FALSE
    }
  }
  context <- .Call("sourceCppContext", PACKAGE = "Rcpp", file, 
                   code, rebuild, cacheDir, .Platform)
  
  
  if (context$buildRequired || rebuild) {
    if (verbose) Rcpp:::.printVerboseOutput(context)
    succeeded <- FALSE
    output <- NULL
    depends <- Rcpp:::.getSourceCppDependencies(context$depends, 
                                         file)
    Rcpp:::.validatePackages(depends, context$cppSourceFilename)
    envRestore <- Rcpp:::.setupBuildEnvironment(depends, context$plugins, file)
    cwd <- getwd()
    setwd(context$buildDirectory)
    fromCode <- !missing(code)
    
    
    if (!Rcpp:::.callBuildHook(context$cppSourcePath, fromCode, 
                        showOutput)) {
      Rcpp:::.restoreEnvironment(envRestore)
      setwd(cwd)
      return(invisible(NULL))
    }
    
    
    on.exit({
      if (!succeeded) Rcpp:::.showBuildFailureDiagnostics()
      Rcpp:::.callBuildCompleteHook(succeeded, output)
      setwd(cwd)
      Rcpp:::.restoreEnvironment(envRestore)
    })
    
    
    if (file.exists(context$previousDynlibPath)) {
      try(silent = TRUE, dyn.unload(context$previousDynlibPath))
      file.remove(context$previousDynlibPath)
    }
    
    r <- paste(R.home("bin"), "R", sep = .Platform$file.sep)
    lib <- context$dynlibFilename
    deps <- context$cppDependencySourcePaths
    src <- context$cppSourceFilename
    args <- c(r, "CMD", "SHLIB", if (windowsDebugDLL) "-d", 
              if (rebuild) "--preclean", if (dryRun) "--dry-run", 
              "-o", shQuote(lib), 
              if (length(deps)) 
                paste(shQuote(deps), collapse = " "), shQuote(src))
    cmd <- paste(args, collapse = " ")
    if (showOutput) cat(cmd, "\n")
    # result <- suppressWarnings(system(cmd, intern = !showOutput))
    # cat("result <- suppressWarnings: ", result, "\n")
    result <- suppressWarnings(system(cmd, intern = TRUE))
    # cat("result: ", result, "\n")
    
    
    if (!showOutput) {
      output <- result
      attributes(output) <- NULL
      status <- attr(result, "status")
      if (!is.null(status)) {
        cat(result, sep = "\n")
        succeeded <- FALSE
        stop("Error ", status, " occurred building shared library.")
      }
      else if (!file.exists(context$dynlibFilename)) {
        cat(result, sep = "\n")
        succeeded <- FALSE
        stop("Error occurred building shared library.")
      }
      else {
        succeeded <- TRUE
      }
    }
    else if (!identical(as.character(result), "0")) {
      succeeded <- FALSE
      stop("Error ", result, " occurred building shared library.")
    }
    else {
      succeeded <- TRUE
    }
  }
  else {
    cwd <- getwd()
    on.exit({
      setwd(cwd)
    })
    if (verbose) 
      cat("\nNo rebuild required (use rebuild = TRUE to ", 
          "force a rebuild)\n\n", sep = "")
  }
  
  
  if (dryRun) 
  {
    # return(invisible(NULL))
    cat("?????????????????????????????result = ", result, "\n")
    return(result)
  }
  
    
  if (length(context$exportedFunctions) > 0 || length(context$modules) > 
      0) {
    exports <- c(context$exportedFunctions, context$modules)
    removeObjs <- exports[exports %in% ls(envir = env, all.names = T)]
    remove(list = removeObjs, envir = env)
    scriptPath <- file.path(context$buildDirectory, context$rSourceFilename)
    source(scriptPath, local = env)
  }
  else if (getOption("rcpp.warnNoExports", default = TRUE)) {
    warning("No Rcpp::export attributes or RCPP_MODULE declarations ", 
            "found in source")
  }
  if (embeddedR && (length(context$embeddedR) > 0)) {
    srcConn <- textConnection(context$embeddedR)
    setwd(rWorkingDir)
    source(file = srcConn, local = env, echo = echo)
  }
  if (cleanupCacheDir) 
    Rcpp:::cleanupSourceCppCache(cacheDir, context$cppSourcePath,
                                 context$buildDirectory)
  invisible(list(functions = context$exportedFunctions, modules = context$modules, 
                 cppSourcePath = context$cppSourcePath, buildDirectory = context$buildDirectory))
}




# ==============================================================================
# Copied Rcpp::sourceCpp(), added some ingredients, truncated much just for
# dry run.
# ==============================================================================
cppDryRun <- function (file = "", env = globalenv(), rebuild = FALSE, 
                       cacheDir = getOption("rcpp.cache.dir", tempdir()), 
                       cleanupCacheDir = FALSE, showOutput = verbose, 
                       verbose = getOption("verbose"), 
                       windowsDebugDLL = FALSE) 
{
  dryRun = TRUE
  code = NULL
  
  
  cacheDir <- path.expand(cacheDir)
  cacheDir <- Rcpp:::.sourceCppPlatformCacheDir(cacheDir)
  cacheDir <- normalizePath(cacheDir, winslash = '/')
  rWorkingDir <- normalizePath(dirname(file), winslash = '/')
  file <- normalizePath(file, winslash = "/")
  if (!tools::file_ext(file) %in% c("cc", "cpp")) {
    stop("The filename '", basename(file), "' does not have an ", 
         "extension of .cc or .cpp so cannot be compiled.")
  }
  
  
  if (.Platform$OS.type == "windows") {
    if (grepl(" ", basename(file), fixed = TRUE)) {
      stop("The filename '", basename(file), "' contains spaces. This ", 
           "is not permitted.")
    }
  }
  else {
    if (windowsDebugDLL) {
      if (verbose) {
        message("The 'windowsDebugDLL' toggle is ignored on ", 
                "non-Windows platforms.")
      }
      windowsDebugDLL <- FALSE
    }
  }
  
  
  context <- .Call("sourceCppContext", PACKAGE = "Rcpp", file, 
                   code, rebuild, cacheDir, .Platform)
  
  
  if (context$buildRequired || rebuild) {
    if (verbose) 
      Rcpp:::.printVerboseOutput(context)
    succeeded <- FALSE
    output <- NULL
    depends <- Rcpp:::.getSourceCppDependencies(
      context$depends, file)
    Rcpp:::.validatePackages(depends, context$cppSourceFilename)
    envRestore <- Rcpp:::.setupBuildEnvironment(depends, context$plugins, 
                                                file)
    cwd <- getwd()
    setwd(context$buildDirectory)
    fromCode <- !missing(code)
    if (!Rcpp:::.callBuildHook(context$cppSourcePath, fromCode, 
                               showOutput)) {
      Rcpp:::.restoreEnvironment(envRestore)
      setwd(cwd)
      return(invisible(NULL))
    }
    on.exit({
      if (!succeeded) Rcpp:::.showBuildFailureDiagnostics()
      Rcpp:::.callBuildCompleteHook(succeeded, output)
      setwd(cwd)
      Rcpp:::.restoreEnvironment(envRestore)
    })
    if (file.exists(context$previousDynlibPath)) {
      try(silent = TRUE, dyn.unload(context$previousDynlibPath))
      file.remove(context$previousDynlibPath)
    }
    r <- paste(R.home("bin"), "R", sep = .Platform$file.sep)
    lib <- context$dynlibFilename
    deps <- context$cppDependencySourcePaths
    src <- context$cppSourceFilename
    
    
    args <- c(r, "CMD", "SHLIB", if (windowsDebugDLL) "-d", 
              if (rebuild) "--preclean", if (dryRun) "--dry-run", 
              "-o", shQuote(lib), if (length(deps)) 
                paste(shQuote(deps), collapse = " "), shQuote(src))
    cmd <- paste(args, collapse = " ")
    result = suppressWarnings(system(cmd, intern = T))
    
    
    setwd(cwd)
    buildDir = context$buildDirectory
    
  }
  else {
    if (verbose) 
      cat("\nNo rebuild required (use rebuild = TRUE to ", 
          "force a rebuild)\n\n", sep = "")
    result = NULL
    buildDir = context$buildDirectory
  }
  
  
  list(cmd = result, buildDir = buildDir)
}




sourceCppCharlie = function(file = "", env = globalenv(), rebuild = FALSE, 
                            # cacheDir = getOption("rcpp.cache.dir", tempdir()),
                            cacheDir = '../dlls',
                            cleanupCacheDir = FALSE, showOutput = verbose, 
                            verbose = getOption("verbose"), 
                            windowsDebugDLL = FALSE, 
                            compileOptionsToBeRemoved = c(
                              "-Werror=format-security -Wp,-D_FORTIFY_SOURCE=2 -Wp,-D_GLIBCXX_ASSERTIONS -fexceptions -fstack-protector-strong -grecord-gcc-switches -specs=/usr/lib/rpm/redhat/redhat-hardened-cc1 -specs=/usr/lib/rpm/redhat/redhat-annobin-cc1", 
                              "-fasynchronous-unwind-tables -fstack-clash-protection -fcf-protection",
                              "-Wl,-z,relro  -Wl,-z,now -specs=/usr/lib/rpm/redhat/redhat-hardened-ld"
                              , " -g "
                            ), 
                            Oflag = "-Ofast",
                            rebuildUsingRcppIfFail = TRUE)
{
  
  dir.create(cacheDir, showWarnings = F, recursive = T)
  rebuild = as.logical(rebuild)
  cleanupCacheDir = as.logical(cleanupCacheDir)
  verbose = as.logical(verbose)
  windowsDebugDLL = as.logical(windowsDebugDLL)
  
  
  fileFullPath = normalizePath(file, winslash = '/')
  makeWouldUse = cppDryRun(file, env, rebuild, cacheDir, cleanupCacheDir, 
                           showOutput, verbose, windowsDebugDLL)
  
  
  buildDir = makeWouldUse$buildDir
  makeWouldUse = makeWouldUse$cmd
  
  
  if (is.null(makeWouldUse))
  {
    cwd = getwd()
    setwd(buildDir)
    source(paste0(basename(file), ".R"))
    setwd(cwd)
    return(invisible(NULL))
  }
  
  
  cmd = makeWouldUse[(which(
    makeWouldUse == "make would use") + 1L):length(makeWouldUse)]
  
  
  allOflags = paste0(c("-O0", "-O1", "-O2", "-O3", "-Ofast", "-Os"), 
                     collapse = "|")
  
  
  if (.Platform$OS.type == 'windows')
  {
    compileCmd = cmd[1]
    linkCmd = cmd[!grepl("echo ", cmd) & grepl("g[+][+] ", cmd) & 
                    grepl(" -shared ", cmd) & grepl(" -L", cmd) & grepl(" -lR", cmd)]
    print(linkCmd)
    linkCmd = linkCmd[[length(linkCmd)]]
    linkCmdSeped = strsplit(linkCmd, split = ' ')[[1]]
    linkCmdSeped = linkCmdSeped[tools::file_ext(linkCmdSeped) != 'def']
    linkCmdSeped = linkCmdSeped[!linkCmdSeped %in% c('\\', ';', '')]
    linkCmd = paste0(linkCmdSeped, collapse = ' ')
    cmd = c(compileCmd, linkCmd)
    
    
  } else
  {
    
    cmd = paste0(cmd, collapse = "\n")  
    if (length(compileOptionsToBeRemoved)!= 0)
    {
      tmp = paste0(compileOptionsToBeRemoved, collapse = "|")
      cmd = gsub(tmp, " ", cmd)
    }
    
    
  }
  
  
  cmd = gsub(allOflags, Oflag, cmd)
  cwd = getwd()
  setwd(buildDir)
  
  
  clcmd = function()
  {
    if (.Platform$OS.type == "windows")
    {
      if (verbose) cat(cmd[1], "\n", cmd[2], "\n")
      sysRst = system(cmd[1])
      if (sysRst != 0) return(FALSE)
      sysRst = system(cmd[2])
      if (sysRst != 0) return(FALSE)
    } else 
    {
      if (verbose) cat(cmd, "\n")
      sysRst = system(cmd)
      if (sysRst != 0) return(FALSE)
    }
    source(paste0(basename(file), ".R"))
    setwd(cwd)
    TRUE
  }
  
  
  success = clcmd()
  if (!success)
  {
    setwd(cwd)
    message("Custom C++ build failed. Rebuild it using Rcpp::sourceCpp().")
    Rcpp::sourceCpp(fileFullPath, verbose = !verbose, cacheDir = cacheDir,
                    rebuild = rebuild)
  }
  
  
  if (.Platform$OS.type == "windows")
    list(compileCmd = compileCmd, linkCmd = linkCmd, buildDir = buildDir)
  else
    list(cmd = cmd, buildDir = buildDir)
}




# Test.
if (F)
{
  
  
  curDir = "C:/Users/i56087/Desktop/tsunamiML/Charlie"
  setwd(curDir)
  source('R/sourceCppCharlie.R')
  
  
  info = sourceCppCharlie(
    "src/trainWeightedEnet009.cpp", verbose = T, rebuild = 1,
    Oflag = '-O3', cacheDir = '.', rebuildUsingRcppIfFail = F)
  
  
  setwd(buildDir)
  
  
}


















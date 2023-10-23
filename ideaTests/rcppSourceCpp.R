
rcppSourceCpp = function (file = "", code = NULL, env = globalenv(), embeddedR = TRUE, 
          rebuild = FALSE, cacheDir = getOption("rcpp.cache.dir", tempdir()), 
          cleanupCacheDir = FALSE, showOutput = verbose, verbose = getOption("verbose"), 
          dryRun = FALSE, windowsDebugDLL = FALSE, echo = TRUE) 
{
  cacheDir <- path.expand(cacheDir)
  cacheDir <- .sourceCppPlatformCacheDir(cacheDir)
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
    if (verbose) 
      .printVerboseOutput(context)
    succeeded <- FALSE
    output <- NULL
    depends <- .getSourceCppDependencies(context$depends, 
                                         file)
    .validatePackages(depends, context$cppSourceFilename)
    envRestore <- .setupBuildEnvironment(depends, context$plugins, 
                                         file)
    cwd <- getwd()
    setwd(context$buildDirectory)
    fromCode <- !missing(code)
    if (!.callBuildHook(context$cppSourcePath, fromCode, 
                        showOutput)) {
      .restoreEnvironment(envRestore)
      setwd(cwd)
      return(invisible(NULL))
    }
    on.exit({
      if (!succeeded) .showBuildFailureDiagnostics()
      .callBuildCompleteHook(succeeded, output)
      setwd(cwd)
      .restoreEnvironment(envRestore)
    })
    if (file.exists(context$previousDynlibPath)) {
      try(silent = TRUE, dyn.unload(context$previousDynlibPath))
      file.remove(context$previousDynlibPath)
    }
    r <- file.path(R.home("bin"), "R")
    lib <- context$dynlibFilename
    deps <- context$cppDependencySourcePaths
    src <- context$cppSourceFilename
    args <- c("CMD", "SHLIB", if (windowsDebugDLL) "-d", 
              if (rebuild) "--preclean", if (dryRun) "--dry-run", 
              "-o", shQuote(lib), if (length(deps)) paste(shQuote(deps), 
                                                          collapse = " "), shQuote(src))
    if (showOutput) 
      cat(paste(c(r, args), collapse = " "), "\n")
    so <- if (showOutput) 
      ""
    else TRUE
    result <- suppressWarnings(system2(r, args, stdout = so, 
                                       stderr = so))
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
    return(invisible(NULL))
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
    cleanupSourceCppCache(cacheDir, context$cppSourcePath, 
                          context$buildDirectory)
  invisible(list(functions = context$exportedFunctions, modules = context$modules, 
                 cppSourcePath = context$cppSourcePath, buildDirectory = context$buildDirectory))
}








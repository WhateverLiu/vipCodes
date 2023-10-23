function (pkgdir = ".", verbose = getOption("verbose")) 
{
  pkgdir <- normalizePath(pkgdir, winslash = "/")
  descFile <- file.path(pkgdir, "DESCRIPTION")
  if (!file.exists(descFile)) 
    stop("pkgdir must refer to the directory containing an R package")
  pkgDesc <- read.dcf(descFile)[1, ]
  pkgname = .readPkgDescField(pkgDesc, "Package")
  depends <- c(.readPkgDescField(pkgDesc, "Depends", character()), 
               .readPkgDescField(pkgDesc, "Imports", character()), .readPkgDescField(pkgDesc, 
                                                                                     "LinkingTo", character()))
  depends <- unique(.splitDepends(depends))
  depends <- depends[depends != "R"]
  namespaceFile <- file.path(pkgdir, "NAMESPACE")
  if (!file.exists(namespaceFile)) 
    stop("pkgdir must refer to the directory containing an R package")
  pkgNamespace <- readLines(namespaceFile, warn = FALSE)
  registration <- any(grepl("^\\s*useDynLib.*\\.registration\\s*=\\s*TRUE.*$", 
                            pkgNamespace))
  srcDir <- file.path(pkgdir, "src")
  if (!file.exists(srcDir)) 
    return(FALSE)
  rDir <- file.path(pkgdir, "R")
  if (!file.exists(rDir)) 
    dir.create(rDir)
  unlink(file.path(rDir, "RcppExports.R"))
  cppFiles <- list.files(srcDir, pattern = "\\.((c(c|pp)?)|(h(pp)?))$", 
                         ignore.case = TRUE)
  cppFiles <- setdiff(cppFiles, "RcppExports.cpp")
  locale <- Sys.getlocale(category = "LC_COLLATE")
  Sys.setlocale(category = "LC_COLLATE", locale = "C")
  cppFiles <- sort(cppFiles)
  Sys.setlocale(category = "LC_COLLATE", locale = locale)
  cppFileBasenames <- tools::file_path_sans_ext(cppFiles)
  cppFiles <- file.path(srcDir, cppFiles)
  cppFiles <- normalizePath(cppFiles, winslash = "/")
  linkingTo <- .readPkgDescField(pkgDesc, "LinkingTo")
  includes <- .linkingToIncludes(linkingTo, TRUE)
  typesHeader <- c(paste0(pkgname, "_types.h"), paste0(pkgname, 
                                                       "_types.hpp"))
  pkgHeader <- c(paste0(pkgname, ".h"), typesHeader)
  pkgHeaderPath <- file.path(pkgdir, "inst", "include", pkgHeader)
  pkgHeader <- pkgHeader[file.exists(pkgHeaderPath)]
  if (length(pkgHeader) > 0) {
    pkgInclude <- paste("#include \"../inst/include/", pkgHeader, 
                        "\"", sep = "")
    includes <- c(pkgInclude, includes)
  }
  pkgHeader <- typesHeader
  pkgHeaderPath <- file.path(pkgdir, "src", pkgHeader)
  pkgHeader <- pkgHeader[file.exists(pkgHeaderPath)]
  if (length(pkgHeader) > 0) 
    includes <- c(paste0("#include \"", pkgHeader, "\""), 
                  includes)
  invisible(.Call("compileAttributes", PACKAGE = "Rcpp", pkgdir, 
                  pkgname, depends, registration, cppFiles, cppFileBasenames, 
                  includes, verbose, .Platform))
}





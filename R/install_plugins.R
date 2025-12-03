#' List available Vamp plugins for download
#'
#' @param platform Platform to filter by: "win32", "osx", or "linux" (default: current OS)
#' @return A data frame with plugin names, descriptions, and download URLs
#' @export
#' @importFrom utils download.file unzip untar
list_available_plugins <- function(platform = NULL) {
  
  # Determine platform if not specified
  if (is.null(platform)) {
    os <- Sys.info()["sysname"]
    platform <- switch(os,
                      "Windows" = "win32",
                      "Darwin" = "osx",
                      "linux")
  }
  
  tryCatch({
    # Get RDF index
    index_url <- "https://www.vamp-plugins.org/rdf/plugins/index.txt"
    rdf_urls <- readLines(index_url, warn = FALSE)
    
    names <- character()
    descriptions <- character()
    download_pages <- character()
    
    # Parse a subset of RDF files for popular plugins
    popular_plugins <- c("qm-vamp-plugins", "bbc-vamp-plugins", "pyin", 
                         "silvet", "match-vamp-plugin", "nnls-chroma")
    
    for (plugin_id in popular_plugins) {
      rdf_url <- paste0("https://www.vamp-plugins.org/rdf/plugins/", plugin_id)
      
      tryCatch({
        rdf_content <- paste(readLines(rdf_url, warn = FALSE), collapse = "\n")
        
        # Extract dc:title
        title_match <- regexpr('dc:title[[:space:]]+"([^"]+)"', rdf_content, perl = TRUE)
        if (title_match > 0) {
          title <- regmatches(rdf_content, title_match)
          title <- gsub('dc:title[[:space:]]+"([^"]+)"', '\\1', title)
        } else {
          title <- plugin_id
        }
        
        # Extract dc:description
        desc_match <- regexpr('dc:description[[:space:]]+"""([^"]+)"""', rdf_content, perl = TRUE)
        if (desc_match > 0) {
          desc <- regmatches(rdf_content, desc_match)
          desc <- gsub('dc:description[[:space:]]+"""([^"]+)"""', '\\1', desc)
          desc <- gsub("\\n", " ", desc)
        } else {
          desc <- ""
        }
        
        # Extract doap:download-page
        download_match <- regexpr('doap:download-page[[:space:]]+<([^>]+)>', rdf_content, perl = TRUE)
        if (download_match > 0) {
          download_url <- regmatches(rdf_content, download_match)
          download_url <- gsub('doap:download-page[[:space:]]+<([^>]+)>', '\\1', download_url)
        } else {
          download_url <- ""
        }
        
        if (download_url != "") {
          names <- c(names, plugin_id)
          descriptions <- c(descriptions, desc)
          download_pages <- c(download_pages, download_url)
        }
        
      }, error = function(e) {
        # Skip plugins that fail to parse
      })
    }
    
    # Add aubio plugins with direct download
    names <- c(names, "vamp-aubio-plugins")
    descriptions <- c(descriptions, "Audio analysis plugins from Aubio library (onset, pitch, tempo detection)")
    download_pages <- c(download_pages, 
                       switch(platform,
                             "win32" = "https://aubio.org/bin/vamp-aubio-plugins/0.5.1/vamp-aubio-plugins-0.5.1-win32.zip",
                             "osx" = "https://aubio.org/bin/vamp-aubio-plugins/0.5.1/vamp-aubio-plugins-0.5.1-osx-universal.zip",
                             "https://aubio.org/bin/vamp-aubio-plugins/0.5.1/vamp-aubio-plugins-0.5.1-linux-amd64.zip"))
    
    if (length(names) == 0) {
      stop("No plugins found")
    }
    
    plugins <- data.frame(
      name = names,
      description = descriptions,
      download_url = download_pages,
      stringsAsFactors = FALSE
    )
    
    return(plugins)
    
  }, error = function(e) {
    message("Could not fetch plugins from RDF: ", e$message)
    
    # Fallback to known working URLs
    plugins <- data.frame(
      name = c("vamp-aubio-plugins"),
      description = c("Audio analysis plugins from Aubio library"),
      download_url = c(switch(platform,
                             "win32" = "https://aubio.org/bin/vamp-aubio-plugins/0.5.1/vamp-aubio-plugins-0.5.1-win32.zip",
                             "osx" = "https://aubio.org/bin/vamp-aubio-plugins/0.5.1/vamp-aubio-plugins-0.5.1-osx-universal.zip",
                             "https://aubio.org/bin/vamp-aubio-plugins/0.5.1/vamp-aubio-plugins-0.5.1-linux-amd64.zip")),
      stringsAsFactors = FALSE
    )
    
    return(plugins)
  })
}

#' Get the appropriate Vamp plugin directory for the current OS
#'
#' @param user_dir If TRUE, use user-specific directory; if FALSE, use system directory
#' @return Path to the Vamp plugin directory
#' @export
#' @note On Windows, the system directory (C:\\Program Files\\Vamp Plugins) may require administrator rights.
#'       User directory is recommended for non-admin installations.
get_vamp_plugin_dir <- function(user_dir = TRUE) {
  os <- Sys.info()["sysname"]
  
  if (os == "Windows") {
    # Note: The Vamp host SDK looks for plugins in specific locations
    # By default it searches: C:\Program Files\Vamp Plugins
    # User can set VAMP_PATH environment variable to add custom directories
    if (user_dir) {
      # Use user profile directory which doesn't require admin
      dir <- normalizePath(file.path(Sys.getenv("USERPROFILE"), "Vamp Plugins"), mustWork = FALSE, winslash = "\\")
    } else {
      dir <- normalizePath(file.path(Sys.getenv("ProgramFiles"), "Vamp Plugins"), mustWork = FALSE, winslash = "\\")
    }
  } else if (os == "Darwin") {
    if (user_dir) {
      dir <- path.expand("~/Library/Audio/Plug-Ins/Vamp")
    } else {
      dir <- "/Library/Audio/Plug-Ins/Vamp"
    }
  } else {
    # Linux
    if (user_dir) {
      dir <- path.expand("~/.vamp")
    } else {
      dir <- "/usr/local/lib/vamp"
    }
  }
  
  return(dir)
}

#' Download and install Vamp plugins
#'
#' @param plugin_names Character vector of plugin names to install (from list_available_plugins()), or NULL to install first available
#' @param user_dir If TRUE, install to user-specific directory; if FALSE, install system-wide (may require admin)
#' @param verbose If TRUE, print progress messages
#' @return Invisibly returns a logical vector indicating success for each plugin
#' @export
#' @examples
#' \dontrun{
#' # List available plugins
#' list_available_plugins()
#' 
#' # Install specific plugin to user directory (no admin required)
#' install_vamp_plugins("vamp-aubio-plugins", user_dir = TRUE)
#' 
#' # Install to system directory (may require admin rights)
#' install_vamp_plugins("vamp-aubio-plugins", user_dir = FALSE)
#' }
install_vamp_plugins <- function(plugin_names = NULL, user_dir = TRUE, verbose = TRUE) {
  
  # Get available plugins
  available <- list_available_plugins()
  
  # If no plugin names specified, use the first available (likely Plugin Pack)
  if (is.null(plugin_names)) {
    if (verbose) message("No specific plugins requested. Using first available option...")
    plugin_names <- available$name[1]
  }
  
  # Validate plugin names
  invalid <- plugin_names[!plugin_names %in% available$name]
  if (length(invalid) > 0) {
    stop("Unknown plugins: ", paste(invalid, collapse = ", "), 
         "\nUse list_available_plugins() to see available options")
  }
  
  # Get OS-specific info
  os <- Sys.info()["sysname"]
  plugin_dir <- get_vamp_plugin_dir(user_dir)
  
  # Create plugin directory if it doesn't exist
  if (!dir.exists(plugin_dir)) {
    if (verbose) message("Creating directory: ", plugin_dir)
    tryCatch({
      dir.create(plugin_dir, recursive = TRUE, showWarnings = FALSE)
    }, error = function(e) {
      stop("Failed to create plugin directory '", plugin_dir, "': ", e$message, 
           "\n  Try using user_dir=", !user_dir, " or run with administrator privileges.")
    })
  }
  
  # Set VAMP_PATH environment variable to include both directories
  if (os == "Windows") {
    user_path <- normalizePath(file.path(Sys.getenv("USERPROFILE"), "Vamp Plugins"), mustWork = FALSE, winslash = "\\")
    system_path <- normalizePath(file.path(Sys.getenv("ProgramFiles"), "Vamp Plugins"), mustWork = FALSE, winslash = "\\")
    vamp_path <- paste(user_path, system_path, sep = ";")
    Sys.setenv(VAMP_PATH = vamp_path)
    if (verbose && user_dir) {
      message("Note: VAMP_PATH set to include both user and system directories")
      message("  User dir: ", user_path)
      message("  System dir: ", system_path)
    }
  }
  
  results <- logical(length(plugin_names))
  names(results) <- plugin_names
  
  for (i in seq_along(plugin_names)) {
    plugin_name <- plugin_names[i]
    plugin_info <- available[available$name == plugin_name, ]
    url <- plugin_info$download_url
    
    if (verbose) {
      message("\n[", i, "/", length(plugin_names), "] Downloading ", plugin_name, "...")
      message("  URL: ", url)
    }
    
    # Check if it's an installer (exe/dmg) or archive (zip/tar.gz)
    is_installer <- grepl("\\.(exe|dmg)$", url, ignore.case = TRUE)
    
    if (is_installer) {
      if (verbose) {
        message("  This is an installer package.")
        message("  Downloading to: ", getwd())
      }
      
      # For installers, download to current directory
      filename <- basename(url)
      download_path <- file.path(getwd(), filename)
      
      tryCatch({
        download.file(url, download_path, mode = "wb", quiet = !verbose)
        results[i] <- TRUE
        if (verbose) {
          message("  SUCCESS: Downloaded ", filename)
          message("  Please run the installer manually: ", download_path)
        }
      }, error = function(e) {
        results[i] <- FALSE
        if (verbose) message("  ERROR: Failed to download - ", e$message)
      })
      
    } else {
      # For archives, download and extract
      temp_file <- tempfile(fileext = ifelse(grepl("\\.zip$", url), ".zip", ".tar.gz"))
      
      tryCatch({
        download.file(url, temp_file, mode = "wb", quiet = !verbose)
        
        if (verbose) message("  Extracting to: ", plugin_dir)
        
        # Extract
        if (grepl("\\.zip$", url)) {
          # Extract to temp directory first
          temp_extract <- tempfile("vamp_extract")
          dir.create(temp_extract)
          unzip(temp_file, exdir = temp_extract, overwrite = TRUE)
          
          # Find DLL/SO files and move them to plugin_dir
          plugin_files <- list.files(temp_extract, pattern = "\\.(dll|so|dylib|cat|n3)$", 
                                    recursive = TRUE, full.names = TRUE)
          
          if (length(plugin_files) > 0) {
            for (pf in plugin_files) {
              file.copy(pf, file.path(plugin_dir, basename(pf)), overwrite = TRUE)
            }
            if (verbose) message("  Copied ", length(plugin_files), " file(s)")
          } else {
            # No plugin files found, just copy everything
            file.copy(list.files(temp_extract, full.names = TRUE), 
                     plugin_dir, recursive = TRUE, overwrite = TRUE)
          }
          
          unlink(temp_extract, recursive = TRUE)
        } else {
          untar(temp_file, exdir = plugin_dir)
        }
        
        results[i] <- TRUE
        if (verbose) message("  SUCCESS: ", plugin_name, " installed")
        
      }, error = function(e) {
        results[i] <- FALSE
        if (verbose) message("  ERROR: Failed to install ", plugin_name, " - ", e$message)
      }, finally = {
        if (file.exists(temp_file)) unlink(temp_file)
      })
    }
  }
  
  if (verbose) {
    message("\n=== Installation Summary ===")
    if (!all(grepl("\\.(exe|dmg)$", available$download_url[available$name %in% plugin_names]))) {
      message("Installed to: ", plugin_dir)
    }
    message("Success: ", sum(results), "/", length(results))
    if (any(!results)) {
      message("Failed: ", paste(names(results)[!results], collapse = ", "))
    }
    message("\nVerify installation with: vampPlugins()")
  }
  
  invisible(results)
}

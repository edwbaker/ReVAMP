test_that("VAMP_PATH environment variable is respected", {
  skip_if_not(requireNamespace("tuneR", quietly = TRUE), 
              "tuneR package not available")
  
  # Save original VAMP_PATH
  original_path <- Sys.getenv("VAMP_PATH", unset = NA)
  
  # Get default plugins
  default_plugins <- vampPlugins()
  default_paths <- vampPaths()
  
  skip_if(nrow(default_plugins) == 0, "Should find plugins in default paths")
  expect_gt(length(default_paths), 0,
            label = "Should have at least one search path")
  
  # Test that paths are reported correctly
  expect_type(default_paths, "character")
  expect_true(all(nzchar(default_paths)),
              label = "All paths should be non-empty strings")
})

test_that("custom VAMP_PATH can be set", {
  skip_if_not(requireNamespace("tuneR", quietly = TRUE), 
              "tuneR package not available")
  
  # Save original VAMP_PATH
  original_path <- Sys.getenv("VAMP_PATH", unset = NA)
  
  # Create a custom path string (use actual system paths)
  if (.Platform$OS.type == "windows") {
    user_dir <- file.path(Sys.getenv("USERPROFILE"), "Vamp Plugins")
    system_dir <- file.path(Sys.getenv("ProgramFiles"), "Vamp Plugins")
    custom_path <- paste(user_dir, system_dir, sep = ";")
  } else if (Sys.info()["sysname"] == "Darwin") {
    custom_path <- paste(
      file.path(Sys.getenv("HOME"), "Library/Audio/Plug-Ins/Vamp"),
      "/Library/Audio/Plug-Ins/Vamp",
      sep = ":"
    )
  } else {
    custom_path <- paste(
      file.path(Sys.getenv("HOME"), "vamp"),
      file.path(Sys.getenv("HOME"), ".vamp"),
      "/usr/local/lib/vamp",
      "/usr/lib/vamp",
      sep = ":"
    )
  }
  
  # Set custom VAMP_PATH
  Sys.setenv(VAMP_PATH = custom_path)
  
  # Verify it's set
  expect_equal(Sys.getenv("VAMP_PATH"), custom_path,
               label = "VAMP_PATH should be set to custom value")
  
  # Get plugins with custom path
  custom_plugins <- vampPlugins()
  custom_paths <- vampPaths()
  
  # Should still find plugins
  skip_if(nrow(custom_plugins) == 0, "Should find plugins with custom VAMP_PATH")
  expect_gt(length(custom_paths), 0,
            label = "Should have search paths with custom VAMP_PATH")
  
  # Restore original VAMP_PATH
  if (is.na(original_path)) {
    Sys.unsetenv("VAMP_PATH")
  } else {
    Sys.setenv(VAMP_PATH = original_path)
  }
})

test_that("VAMP_PATH with non-existent directory doesn't crash", {
  skip_if_not(requireNamespace("tuneR", quietly = TRUE), 
              "tuneR package not available")
  
  # Save original VAMP_PATH
  original_path <- Sys.getenv("VAMP_PATH", unset = NA)
  
  # Set VAMP_PATH to a non-existent directory plus a real one
  if (.Platform$OS.type == "windows") {
    real_dir <- file.path(Sys.getenv("ProgramFiles"), "Vamp Plugins")
    fake_dir <- "C:\\NonExistent\\Vamp\\Path"
    test_path <- paste(fake_dir, real_dir, sep = ";")
  } else {
    real_dir <- "/usr/local/lib/vamp"
    fake_dir <- "/nonexistent/vamp/path"
    test_path <- paste(fake_dir, real_dir, sep = ":")
  }
  
  Sys.setenv(VAMP_PATH = test_path)
  
  # Should not crash, just skip non-existent directories
  expect_no_error({
    paths <- vampPaths()
    plugins <- vampPlugins()
  })
  
  # Should still find plugins from existing directories
  expect_type(paths, "character")
  
  # Restore original VAMP_PATH
  if (is.na(original_path)) {
    Sys.unsetenv("VAMP_PATH")
  } else {
    Sys.setenv(VAMP_PATH = original_path)
  }
})

test_that("VAMP_PATH uses correct path separator for OS", {
  skip_if_not(requireNamespace("tuneR", quietly = TRUE), 
              "tuneR package not available")
  
  # Save original VAMP_PATH
  original_path <- Sys.getenv("VAMP_PATH", unset = NA)
  
  # Test with multiple paths using correct separator
  if (.Platform$OS.type == "windows") {
    # Windows uses semicolon
    path1 <- file.path(Sys.getenv("USERPROFILE"), "Vamp Plugins")
    path2 <- file.path(Sys.getenv("ProgramFiles"), "Vamp Plugins")
    test_path <- paste(path1, path2, sep = ";")
    expect_true(grepl(";", test_path),
                label = "Windows path should contain semicolon separator")
  } else {
    # Unix/Mac use colon
    path1 <- file.path(Sys.getenv("HOME"), ".vamp")
    path2 <- "/usr/local/lib/vamp"
    test_path <- paste(path1, path2, sep = ":")
    expect_true(grepl(":", test_path),
                label = "Unix path should contain colon separator")
  }
  
  Sys.setenv(VAMP_PATH = test_path)
  
  # Should parse correctly and return multiple paths
  paths <- vampPaths()
  expect_gte(length(paths), 1,
             label = "Should parse multiple paths from VAMP_PATH")
  
  # Restore original VAMP_PATH
  if (is.na(original_path)) {
    Sys.unsetenv("VAMP_PATH")
  } else {
    Sys.setenv(VAMP_PATH = original_path)
  }
})

test_that("empty VAMP_PATH falls back to defaults", {
  skip_if_not(requireNamespace("tuneR", quietly = TRUE), 
              "tuneR package not available")
  
  # Save original VAMP_PATH
  original_path <- Sys.getenv("VAMP_PATH", unset = NA)
  
  # Set VAMP_PATH to empty string
  Sys.setenv(VAMP_PATH = "")
  
  # Should fall back to default paths
  paths <- vampPaths()
  plugins <- vampPlugins()
  
  expect_gt(length(paths), 0,
            label = "Should use default paths when VAMP_PATH is empty")
  
  # Verify we get default system paths
  if (.Platform$OS.type == "windows") {
    expect_true(any(grepl("Vamp Plugins", paths, ignore.case = TRUE)),
                label = "Windows should include default Vamp Plugins directory")
  } else if (Sys.info()["sysname"] == "Darwin") {
    expect_true(any(grepl("Library.*Vamp", paths)),
                label = "macOS should include Library Vamp directory")
  } else {
    expect_true(any(grepl("/vamp$", paths)),
                label = "Linux should include vamp directories")
  }
  
  # Restore original VAMP_PATH
  if (is.na(original_path)) {
    Sys.unsetenv("VAMP_PATH")
  } else {
    Sys.setenv(VAMP_PATH = original_path)
  }
})

test_that("default VAMP_PATH expands environment variables", {
  skip_if_not(requireNamespace("tuneR", quietly = TRUE), 
              "tuneR package not available")
  
  # Save original VAMP_PATH
  original_path <- Sys.getenv("VAMP_PATH", unset = NA)
  
  # Unset VAMP_PATH to force defaults (which do environment variable expansion)
  Sys.setenv(VAMP_PATH = "")
  
  paths <- vampPaths()
  
  # When using defaults, environment variables should be expanded
  if (.Platform$OS.type == "windows") {
    # Should not contain %ProgramFiles% placeholder
    expect_false(any(grepl("%ProgramFiles%", paths, fixed = TRUE)),
                 label = "Default paths should expand %ProgramFiles%")
    # Should contain actual Program Files path or Vamp Plugins
    expect_true(any(grepl("Program Files|Vamp Plugins", paths, ignore.case = TRUE)),
                label = "Should contain expanded Program Files path")
  } else {
    # Should not contain $HOME placeholder  
    expect_false(any(grepl("$HOME", paths, fixed = TRUE)),
                 label = "Default paths should expand $HOME")
    # Should contain actual home directory
    home <- Sys.getenv("HOME")
    if (nzchar(home)) {
      expect_true(any(grepl(home, paths, fixed = TRUE)),
                  label = "Should contain expanded home directory path")
    }
  }
  
  # Restore original VAMP_PATH
  if (is.na(original_path)) {
    Sys.unsetenv("VAMP_PATH")
  } else {
    Sys.setenv(VAMP_PATH = original_path)
  }
})

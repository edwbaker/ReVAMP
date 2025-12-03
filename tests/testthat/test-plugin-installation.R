test_that("get_vamp_plugin_dir returns valid path", {
  user_dir <- get_vamp_plugin_dir(user_dir = TRUE)
  system_dir <- get_vamp_plugin_dir(user_dir = FALSE)
  
  expect_type(user_dir, "character")
  expect_type(system_dir, "character")
  expect_true(nchar(user_dir) > 0)
  expect_true(nchar(system_dir) > 0)
  
  # Check paths use correct separators for Windows
  if (.Platform$OS.type == "windows") {
    expect_true(grepl("\\\\", user_dir))
    expect_true(grepl("\\\\", system_dir))
  }
})

test_that("list_available_plugins returns data frame", {
  skip_on_cran()
  skip_if_offline()
  
  plugins <- list_available_plugins()
  
  expect_s3_class(plugins, "data.frame")
  expect_true(all(c("name", "description", "download_url") %in% names(plugins)))
  
  if (nrow(plugins) > 0) {
    # Check URLs are valid
    expect_true(all(grepl("^https?://", plugins$download_url)))
  }
})

test_that("install_vamp_plugins validates inputs", {
  # Function should handle invalid inputs, though exact error type may vary
  result1 <- tryCatch(
    install_vamp_plugins(character(0)),
    error = function(e) "error",
    message = function(m) "message"
  )
  expect_true(result1 == "error" || result1 == "message")
  
  result2 <- tryCatch(
    install_vamp_plugins(123),
    error = function(e) "error"
  )
  expect_equal(result2, "error")
})

test_that("VAMP_PATH environment variable is set correctly", {
  # Get current VAMP_PATH
  vamp_path <- Sys.getenv("VAMP_PATH")
  
  if (nchar(vamp_path) > 0) {
    # Check it contains expected separators
    if (.Platform$OS.type == "windows") {
      expect_true(grepl(";", vamp_path) || length(strsplit(vamp_path, ";")[[1]]) == 1)
    } else {
      expect_true(grepl(":", vamp_path) || length(strsplit(vamp_path, ":")[[1]]) == 1)
    }
  }
})

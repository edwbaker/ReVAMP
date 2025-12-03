test_that("vampInfo returns valid SDK information", {
  info <- vampInfo()
  
  expect_type(info, "list")
  expect_named(info, c("API version", "SDK version"))
  expect_equal(info[[1]], 2)
  expect_type(info[[2]], "character")
  expect_true(grepl("^[0-9]+(\\.[0-9]+)*$", info[[2]]))  # Version format check
})

test_that("vampPaths returns character vector", {
  paths <- vampPaths()
  
  expect_type(paths, "character")
  expect_true(length(paths) > 0)
})

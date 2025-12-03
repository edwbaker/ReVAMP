test_that("vampPlugins returns a data frame", {
  skip_if_not(length(vampPaths()) > 0, "No Vamp plugin paths available")
  
  plugins <- vampPlugins()
  
  expect_s3_class(plugins, "data.frame")
  expect_true(nrow(plugins) >= 0)
  
  if (nrow(plugins) > 0) {
    # Check for key columns
    expect_true("library" %in% names(plugins))
    expect_true("id" %in% names(plugins))
  }
})

test_that("vampPlugins library names are correctly parsed", {
  skip_if_not(length(vampPaths()) > 0, "No Vamp plugin paths available")
  
  plugins <- vampPlugins()
  
  if (nrow(plugins) > 0) {
    # Library names should not contain colons (parsing issue indicator)
    expect_false(any(grepl(":", plugins$library)))
    
    # Each row should have a library name
    expect_true(all(nchar(plugins$library) > 0))
  }
})

test_that("vampParams returns plugin parameter information", {
  skip_if_not(length(vampPaths()) > 0, "No Vamp plugin paths available")
  
  plugins <- vampPlugins()
  skip_if(nrow(plugins) == 0, "No plugins available")
  
  # Test with first available plugin - use id column
  first_key <- plugins$id[1]
  params <- vampParams(first_key)
  
  expect_s3_class(params, "data.frame")
  # Params might be empty if plugin has no parameters
  expect_true(nrow(params) >= 0)
  
  if (nrow(params) > 0) {
    expect_true(all(c("identifier", "name", "description", 
                      "unit", "min_value", "max_value", 
                      "default_value", "quantized") %in% names(params)))
  }
})

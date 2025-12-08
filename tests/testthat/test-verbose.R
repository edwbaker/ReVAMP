library(tuneR)

# Helper function to create test audio
create_test_wave <- function(duration = 0.5, sample_rate = 44100) {
  t <- seq(0, duration, length.out = duration * sample_rate)
  signal <- sin(2 * pi * 440 * t)
  signal_int <- as.integer(signal * 32767)
  Wave(left = signal_int, samp.rate = sample_rate, bit = 16)
}

test_that("verbose parameter controls diagnostic output", {
  skip_if_not(nzchar(Sys.which("Rscript")))
  
  # Get a simple plugin to test (BBC Energy - always available)
  plugins <- vampPlugins()
  test_plugin <- plugins[plugins$name == "Energy", "id"][1]
  skip_if(length(test_plugin) == 0 || is.na(test_plugin))
  
  # Create test audio
  wave <- create_test_wave(duration = 0.5)
  
  # Capture output with verbose=FALSE (default)
  output_quiet <- capture.output({
    result_quiet <- runPlugin(wave, test_plugin, verbose = FALSE)
  }, type = "message")
  
  # Capture output with verbose=TRUE
  output_verbose <- capture.output({
    result_verbose <- runPlugin(wave, test_plugin, verbose = TRUE)
  }, type = "message")
  
  # verbose=FALSE should produce no diagnostic output
  expect_length(output_quiet, 0)
  
  # verbose=TRUE should produce diagnostic output
  expect_true(length(output_verbose) > 0)
  
  # Check for expected diagnostic messages
  output_text <- paste(output_verbose, collapse = "\n")
  expect_match(output_text, "Running plugin", ignore.case = TRUE)
  expect_match(output_text, "Using block size", ignore.case = TRUE)
  
  # Results should be identical regardless of verbose setting
  expect_equal(dim(result_quiet), dim(result_verbose))
  expect_equal(colnames(result_quiet), colnames(result_verbose))
})

test_that("verbose works with custom block/step sizes", {
  skip_if_not(nzchar(Sys.which("Rscript")))
  
  plugins <- vampPlugins()
  test_plugin <- plugins[plugins$name == "Energy", "id"][1]
  skip_if(length(test_plugin) == 0 || is.na(test_plugin))
  
  wave <- create_test_wave(duration = 0.5)
  
  # Test with custom block and step sizes
  output_verbose <- capture.output({
    result <- runPlugin(wave, test_plugin, blockSize = 2048, stepSize = 512, verbose = TRUE)
  }, type = "message")
  
  output_text <- paste(output_verbose, collapse = "\n")
  
  # Should mention the custom block and step sizes
  expect_match(output_text, "block size.*2048", ignore.case = TRUE)
  expect_match(output_text, "step size.*512", ignore.case = TRUE)
  
  # Test quiet mode with same settings
  output_quiet <- capture.output({
    result_quiet <- runPlugin(wave, test_plugin, blockSize = 2048, stepSize = 512, verbose = FALSE)
  }, type = "message")
  
  expect_length(output_quiet, 0)
  expect_equal(dim(result), dim(result_quiet))
})

test_that("verbose default is FALSE", {
  skip_if_not(nzchar(Sys.which("Rscript")))
  
  plugins <- vampPlugins()
  test_plugin <- plugins[plugins$name == "Energy", "id"][1]
  skip_if(length(test_plugin) == 0 || is.na(test_plugin))
  
  wave <- create_test_wave(duration = 0.5)
  
  # When verbose is not specified, should default to FALSE (quiet)
  output_default <- capture.output({
    result_default <- runPlugin(wave, test_plugin)
  }, type = "message")
  
  expect_length(output_default, 0)
  
  # Should be identical to explicitly setting verbose=FALSE
  output_explicit <- capture.output({
    result_explicit <- runPlugin(wave, test_plugin, verbose = FALSE)
  }, type = "message")
  
  expect_equal(output_default, output_explicit)
  expect_equal(dim(result_default), dim(result_explicit))
})

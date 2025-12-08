test_that("runPlugin uses plugin's preferred block and step sizes by default", {
  skip_if_not(requireNamespace("tuneR", quietly = TRUE), 
              "tuneR package not available")
  
  # Create a simple test signal
  sample_rate <- 44100
  duration <- 0.5  # seconds
  samples <- round(sample_rate * duration)
  
  # Generate a sine wave
  freq <- 440  # Hz
  t <- seq(0, duration, length.out = samples)
  signal <- sin(2 * pi * freq * t)
  
  # Create Wave object
  wave <- tuneR::Wave(
    left = as.integer(signal * 32767),
    samp.rate = sample_rate,
    bit = 16
  )
  
  # Get plugin info to see default block/step sizes
  plugins <- vampPlugins()
  
  # Use vamp-example-plugins:amplitudefollower which has known defaults
  amp_plugin <- plugins[plugins$id == "vamp-example-plugins:amplitudefollower", ]
  
  if (nrow(amp_plugin) == 0) {
    skip("vamp-example-plugins:amplitudefollower not available")
  }
  
  # Run plugin with defaults - should use plugin's preferred sizes
  result <- runPlugin(
    key = "vamp-example-plugins:amplitudefollower",
    wave = wave
  )
  
  expect_type(result, "list")
  expect_true("amplitude" %in% names(result))
  
  # Check that we got output with reasonable number of frames
  # With default settings, we should get approximately duration * (sample_rate / step_size) frames
  amp_data <- result$amplitude
  expect_gt(nrow(amp_data), 0)
})

test_that("runPlugin respects custom blockSize parameter", {
  skip_if_not(requireNamespace("tuneR", quietly = TRUE), 
              "tuneR package not available")
  
  # Create test signal
  sample_rate <- 44100
  duration <- 0.5
  samples <- round(sample_rate * duration)
  freq <- 440
  t <- seq(0, duration, length.out = samples)
  signal <- sin(2 * pi * freq * t)
  
  wave <- tuneR::Wave(
    left = as.integer(signal * 32767),
    samp.rate = sample_rate,
    bit = 16
  )
  
  # Check if the specific plugin is available
  plugins <- vampPlugins()
  plugin_key <- "vamp-example-plugins:amplitudefollower"
  if (!plugin_key %in% plugins$id) {
    skip("vamp-example-plugins:amplitudefollower not available")
  }
  
  # Run with different block sizes
  result_512 <- runPlugin(
    key = plugin_key,
    wave = wave,
    blockSize = 512,
    stepSize = 512
  )
  
  result_2048 <- runPlugin(
    key = plugin_key,
    wave = wave,
    blockSize = 2048,
    stepSize = 2048
  )
  
  # Smaller block/step size should produce more output frames
  expect_gt(nrow(result_512$amplitude), nrow(result_2048$amplitude))
  
  # Verify approximate frame counts
  # With step = block, frames should be approximately samples / blockSize
  expected_512 <- floor(samples / 512)
  expected_2048 <- floor(samples / 2048)
  
  # Allow some tolerance for edge effects
  expect_lt(abs(nrow(result_512$amplitude) - expected_512), 5)
  expect_lt(abs(nrow(result_2048$amplitude) - expected_2048), 5)
})

test_that("runPlugin respects custom stepSize with overlap", {
  skip_if_not(requireNamespace("tuneR", quietly = TRUE), 
              "tuneR package not available")
  
  # Create test signal
  sample_rate <- 44100
  duration <- 0.5
  samples <- round(sample_rate * duration)
  freq <- 440
  t <- seq(0, duration, length.out = samples)
  signal <- sin(2 * pi * freq * t)
  
  wave <- tuneR::Wave(
    left = as.integer(signal * 32767),
    samp.rate = sample_rate,
    bit = 16
  )
  
  # Check if the specific plugin is available
  plugins <- vampPlugins()
  plugin_key <- "vamp-example-plugins:amplitudefollower"
  if (!plugin_key %in% plugins$id) {
    skip("vamp-example-plugins:amplitudefollower not available")
  }
  
  # Run with 50% overlap (step = block/2)
  result_overlap50 <- runPlugin(
    key = plugin_key,
    wave = wave,
    blockSize = 1024,
    stepSize = 512  # 50% overlap
  )
  
  # Run with no overlap (step = block)
  result_no_overlap <- runPlugin(
    key = plugin_key,
    wave = wave,
    blockSize = 1024,
    stepSize = 1024
  )
  
  # 50% overlap should produce approximately twice as many frames
  ratio <- nrow(result_overlap50$amplitude) / nrow(result_no_overlap$amplitude)
  expect_gt(ratio, 1.8)  # Allow some tolerance
  expect_lt(ratio, 2.2)
})

test_that("runPlugin rejects invalid block and step sizes", {
  skip_if_not(requireNamespace("tuneR", quietly = TRUE), 
              "tuneR package not available")
  
  # Create minimal test signal
  wave <- tuneR::Wave(
    left = as.integer(rep(0, 1000)),
    samp.rate = 44100,
    bit = 16
  )
  
  # Check if the specific plugin is available
  plugins <- vampPlugins()
  plugin_key <- "vamp-example-plugins:amplitudefollower"
  if (!plugin_key %in% plugins$id) {
    # Try to find any available plugin
    if (nrow(plugins) > 0) {
      plugin_key <- plugins$id[1]
    } else {
      skip("No plugins available for testing")
    }
  }
  
  # Test negative blockSize
  expect_error(
    runPlugin(
      key = plugin_key,
      wave = wave,
      blockSize = -512
    ),
    "blockSize must be positive"
  )
  
  # Test zero blockSize
  expect_error(
    runPlugin(
      key = plugin_key,
      wave = wave,
      blockSize = 0
    ),
    "blockSize must be positive"
  )
  
  # Test negative stepSize
  expect_error(
    runPlugin(
      key = "vamp-example-plugins:amplitudefollower",
      wave = wave,
      stepSize = -256
    ),
    "stepSize must be positive"
  )
  
  # Test zero stepSize
  expect_error(
    runPlugin(
      key = "vamp-example-plugins:amplitudefollower",
      wave = wave,
      stepSize = 0
    ),
    "stepSize must be positive"
  )
})

test_that("frequency domain plugins work with custom block size", {
  skip_if_not(requireNamespace("tuneR", quietly = TRUE), 
              "tuneR package not available")
  
  # Create test signal with known frequency
  sample_rate <- 44100
  duration <- 1.0
  samples <- round(sample_rate * duration)
  freq <- 440  # A4
  t <- seq(0, duration, length.out = samples)
  signal <- sin(2 * pi * freq * t)
  
  wave <- tuneR::Wave(
    left = as.integer(signal * 32767),
    samp.rate = sample_rate,
    bit = 16
  )
  
  # Check if spectral centroid plugin is available (frequency domain)
  plugins <- vampPlugins()
  spectral_plugins <- plugins[plugins$input.domain == "Frequency Domain", ]
  
  if (nrow(spectral_plugins) == 0) {
    skip("No frequency domain plugins available for testing")
  }
  
  # Use first available frequency domain plugin
  test_plugin <- spectral_plugins$id[1]
  
  # Run with different FFT sizes
  result_1024 <- runPlugin(
    key = test_plugin,
    wave = wave,
    blockSize = 1024,
    stepSize = 512
  )
  
  result_4096 <- runPlugin(
    key = test_plugin,
    wave = wave,
    blockSize = 4096,
    stepSize = 2048
  )
  
  # Both should produce valid output
  expect_type(result_1024, "list")
  expect_type(result_4096, "list")
  expect_gt(length(result_1024), 0)
  expect_gt(length(result_4096), 0)
  
  # Larger FFT (4096) with same step ratio should produce fewer frames
  # but potentially better frequency resolution
  first_output_1024 <- result_1024[[1]]
  first_output_4096 <- result_4096[[1]]
  
  expect_lt(nrow(first_output_4096), nrow(first_output_1024))
})

test_that("vampPlugins shows default block and step sizes", {
  # Get plugin info
  plugins <- vampPlugins()
  
  # Check that block/step size columns exist
  expect_true("default.step.size" %in% names(plugins))
  expect_true("default.block.size" %in% names(plugins))
  
  # Values should be non-negative integers
  expect_true(all(plugins$default.step.size >= 0))
  expect_true(all(plugins$default.block.size >= 0))
  
  # For frequency domain plugins, step size should typically be <= block size
  freq_domain <- plugins[plugins$input.domain == "Frequency Domain", ]
  if (nrow(freq_domain) > 0) {
    # Filter out plugins with unspecified sizes (0)
    specified <- freq_domain[freq_domain$default.step.size > 0 & 
                             freq_domain$default.block.size > 0, ]
    if (nrow(specified) > 0) {
      expect_true(all(specified$default.step.size <= specified$default.block.size))
    }
  }
})

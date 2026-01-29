library(tuneR)

# Create a simple test audio signal
create_test_wave <- function(duration = 1, sample_rate = 44100) {
  t <- seq(0, duration, length.out = duration * sample_rate)
  # 440 Hz sine wave
  signal <- sin(2 * pi * 440 * t)
  # Convert to 16-bit integer range
  signal_int <- as.integer(signal * 32767)
  
  Wave(left = signal_int, samp.rate = sample_rate, bit = 16)
}

test_that("runPlugin without segmentLength works unchanged", {
  skip_if_not(length(vampPaths()) > 0, "No Vamp plugin paths available")
  
  plugins <- vampPlugins()
  skip_if(nrow(plugins) == 0, "No plugins available")
  
  example_plugins <- plugins[plugins$library == "vamp-example-plugins", ]
  skip_if(nrow(example_plugins) == 0, "vamp-example-plugins not installed")
  
  # Create 2 second test wave

  test_wave <- create_test_wave(duration = 2)
  
  # Run plugin without segmentation
  result <- runPlugin(
    key = "vamp-example-plugins:amplitudefollower",
    wave = test_wave
  )
  
  # Check result structure - no segment column
  expect_type(result, "list")
  expect_true(length(result) > 0)
  
  output_df <- result[[1]]
  expect_false("segment" %in% names(output_df))
  expect_true("timestamp" %in% names(output_df))
})

test_that("runPlugin with segmentLength adds segment column", {
  skip_if_not(length(vampPaths()) > 0, "No Vamp plugin paths available")
  
  plugins <- vampPlugins()
  skip_if(nrow(plugins) == 0, "No plugins available")
  
  example_plugins <- plugins[plugins$library == "vamp-example-plugins", ]
  skip_if(nrow(example_plugins) == 0, "vamp-example-plugins not installed")
  
  # Create 3 second test wave
  test_wave <- create_test_wave(duration = 3)
  
  # Run plugin with 1 second segments
  result <- runPlugin(
    key = "vamp-example-plugins:amplitudefollower",
    wave = test_wave,
    segmentLength = 1
  )
  
  # Check result structure - should have segment column
  expect_type(result, "list")
  expect_true(length(result) > 0)
  
  output_df <- result[[1]]
  expect_true("segment" %in% names(output_df))
  expect_true("timestamp" %in% names(output_df))
  
  # Should have segments 1, 2, 3 for 3 seconds
  segments <- unique(output_df$segment)
  expect_true(all(c(1, 2, 3) %in% segments))
})

test_that("segmentLength resets timestamps per segment", {
  skip_if_not(length(vampPaths()) > 0, "No Vamp plugin paths available")
  
  plugins <- vampPlugins()
  skip_if(nrow(plugins) == 0, "No plugins available")
  
  example_plugins <- plugins[plugins$library == "vamp-example-plugins", ]
  skip_if(nrow(example_plugins) == 0, "vamp-example-plugins not installed")
  
  # Create 2 second test wave
  test_wave <- create_test_wave(duration = 2)
  
  # Run plugin with 1 second segments
  result <- runPlugin(
    key = "vamp-example-plugins:amplitudefollower",
    wave = test_wave,
    segmentLength = 1
  )
  
  output_df <- result[[1]]
  
  # Get first timestamp of each segment
  seg1_start <- min(output_df$timestamp[output_df$segment == 1])
  seg2_start <- min(output_df$timestamp[output_df$segment == 2])
  
  # Both segments should start near 0
  expect_lt(seg1_start, 0.1)
  expect_lt(seg2_start, 0.1)
  
  # Max timestamp in each segment should be less than segment length
  seg1_max <- max(output_df$timestamp[output_df$segment == 1])
  seg2_max <- max(output_df$timestamp[output_df$segment == 2])
  
  expect_lt(seg1_max, 1.0)
  expect_lt(seg2_max, 1.0)
})

test_that("segmentLength validates positive values", {
  skip_if_not(length(vampPaths()) > 0, "No Vamp plugin paths available")
  
  plugins <- vampPlugins()
  skip_if(nrow(plugins) == 0, "No plugins available")
  
  example_plugins <- plugins[plugins$library == "vamp-example-plugins", ]
  skip_if(nrow(example_plugins) == 0, "vamp-example-plugins not installed")
  
  test_wave <- create_test_wave(duration = 1)
  
  # Should error with non-positive segmentLength
  expect_error(
    runPlugin(
      key = "vamp-example-plugins:amplitudefollower",
      wave = test_wave,
      segmentLength = 0
    ),
    "segmentLength must be positive"
  )
  
  expect_error(
    runPlugin(
      key = "vamp-example-plugins:amplitudefollower",
      wave = test_wave,
      segmentLength = -1
    ),
    "segmentLength must be positive"
  )
})

test_that("runPlugins with segmentLength works correctly", {
  skip_if_not(length(vampPaths()) > 0, "No Vamp plugin paths available")
  
  plugins <- vampPlugins()
  skip_if(nrow(plugins) == 0, "No plugins available")
  
  example_plugins <- plugins[plugins$library == "vamp-example-plugins", ]
  skip_if(nrow(example_plugins) == 0, "vamp-example-plugins not installed")
  
  # Create 2 second test wave
  test_wave <- create_test_wave(duration = 2)
  
  # Run multiple plugins with segmentation
  result <- runPlugins(
    keys = c("vamp-example-plugins:amplitudefollower", "vamp-example-plugins:zerocrossing"),
    wave = test_wave,
    segmentLength = 1
  )
  
  # Check both plugins have segment column
  expect_type(result, "list")
  expect_equal(length(result), 2)
  
  for (plugin_result in result) {
    output_df <- plugin_result[[1]]
    expect_true("segment" %in% names(output_df))
    
    segments <- unique(output_df$segment)
    expect_true(all(c(1, 2) %in% segments))
  }
})

test_that("segment column absent when segmentLength is NULL in runPlugins", {
  skip_if_not(length(vampPaths()) > 0, "No Vamp plugin paths available")
  
  plugins <- vampPlugins()
  skip_if(nrow(plugins) == 0, "No plugins available")
  
  example_plugins <- plugins[plugins$library == "vamp-example-plugins", ]
  skip_if(nrow(example_plugins) == 0, "vamp-example-plugins not installed")
  
  test_wave <- create_test_wave(duration = 1)
  
  # Run without segmentation
  result <- runPlugins(
    keys = c("vamp-example-plugins:amplitudefollower"),
    wave = test_wave
  )
  
  output_df <- result[[1]][[1]]
  expect_false("segment" %in% names(output_df))
})

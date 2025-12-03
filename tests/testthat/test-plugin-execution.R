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

test_that("runPlugin executes with valid inputs", {
  skip_if_not(length(vampPaths()) > 0, "No Vamp plugin paths available")
  
  plugins <- vampPlugins()
  skip_if(nrow(plugins) == 0, "No plugins available")
  
  # Look for vamp-example-plugins amplitudefollower
  example_plugins <- plugins[plugins$library == "vamp-example-plugins", ]
  skip_if(nrow(example_plugins) == 0, "vamp-example-plugins not installed")
  
  amp_follower <- example_plugins[example_plugins$plugin == "amplitudefollower", ]
  skip_if(nrow(amp_follower) == 0, "amplitudefollower plugin not found")
  
  # Create test wave
  test_wave <- create_test_wave(duration = 0.5)
  
  # Create temporary output file
  output_file <- tempfile(fileext = ".csv")
  on.exit(unlink(output_file), add = TRUE)
  
  # Run plugin
  expect_silent({
    result <- runPlugin("test_user", "vamp-example-plugins", "amplitudefollower",
                       "amplitude", 0, test_wave, output_file, TRUE)
  })
  
  # Check output file was created
  expect_true(file.exists(output_file))
  expect_true(file.size(output_file) > 0)
})

test_that("runPlugin handles mono audio", {
  skip_if_not(length(vampPaths()) > 0, "No Vamp plugin paths available")
  
  plugins <- vampPlugins()
  skip_if(nrow(plugins) == 0, "No plugins available")
  
  example_plugins <- plugins[plugins$library == "vamp-example-plugins", ]
  skip_if(nrow(example_plugins) == 0, "vamp-example-plugins not installed")
  
  # Create mono test wave
  test_wave <- create_test_wave(duration = 0.25)
  output_file <- tempfile(fileext = ".csv")
  on.exit(unlink(output_file), add = TRUE)
  
  expect_silent({
    result <- runPlugin("test_user", "vamp-example-plugins", "amplitudefollower",
                       "amplitude", 0, test_wave, output_file, TRUE)
  })
  
  expect_true(file.exists(output_file))
})

test_that("runPlugin handles stereo audio", {
  skip_if_not(length(vampPaths()) > 0, "No Vamp plugin paths available")
  
  plugins <- vampPlugins()
  skip_if(nrow(plugins) == 0, "No plugins available")
  
  example_plugins <- plugins[plugins$library == "vamp-example-plugins", ]
  skip_if(nrow(example_plugins) == 0, "vamp-example-plugins not installed")
  
  # Create stereo test wave
  t <- seq(0, 0.25, length.out = 0.25 * 44100)
  left_signal <- as.integer(sin(2 * pi * 440 * t) * 32767)
  right_signal <- as.integer(sin(2 * pi * 554 * t) * 32767)  # Different frequency
  
  test_wave <- Wave(left = left_signal, right = right_signal, 
                    samp.rate = 44100, bit = 16)
  
  output_file <- tempfile(fileext = ".csv")
  on.exit(unlink(output_file), add = TRUE)
  
  expect_silent({
    result <- runPlugin("test_user", "vamp-example-plugins", "amplitudefollower",
                       "amplitude", 0, test_wave, output_file, TRUE)
  })
  
  expect_true(file.exists(output_file))
})

test_that("runPlugin handles different output sample types", {
  skip_if_not(length(vampPaths()) > 0, "No Vamp plugin paths available")
  
  plugins <- vampPlugins()
  skip_if(nrow(plugins) == 0, "No plugins available")
  
  test_wave <- create_test_wave(duration = 0.5)
  
  # Test OneSamplePerStep (amplitudefollower)
  example_plugins <- plugins[plugins$library == "vamp-example-plugins", ]
  if (nrow(example_plugins) > 0 && 
      "amplitudefollower" %in% example_plugins$plugin) {
    output_file <- tempfile(fileext = ".csv")
    on.exit(unlink(output_file), add = TRUE)
    
    expect_silent({
      runPlugin("test", "vamp-example-plugins", "amplitudefollower",
                "amplitude", 0, test_wave, output_file, TRUE)
    })
    expect_true(file.exists(output_file))
  }
  
  # Test VariableSampleRate (percussiononsets if available)
  if (nrow(example_plugins) > 0 && 
      "percussiononsets" %in% example_plugins$plugin) {
    output_file2 <- tempfile(fileext = ".csv")
    on.exit(unlink(output_file2), add = TRUE)
    
    expect_silent({
      runPlugin("test", "vamp-example-plugins", "percussiononsets",
                "onsets", 0, test_wave, output_file2, TRUE)
    })
    expect_true(file.exists(output_file2))
  }
})

test_that("runPlugin fails gracefully with invalid plugin", {
  test_wave <- create_test_wave(duration = 0.1)
  output_file <- tempfile(fileext = ".csv")
  on.exit(unlink(output_file), add = TRUE)
  
  expect_error({
    runPlugin("test", "nonexistent-plugin", "fake-id",
              "output", 0, test_wave, output_file, TRUE)
  })
})

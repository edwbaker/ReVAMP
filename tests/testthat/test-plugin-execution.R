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
  
  amp_follower <- example_plugins[grepl("amplitudefollower", example_plugins$id), ]
  skip_if(nrow(amp_follower) == 0, "amplitudefollower plugin not found")
  
  # Create test wave
  test_wave <- create_test_wave(duration = 0.5)
  
  # Run plugin
  expect_silent({
    result <- runPlugin(
      key = "vamp-example-plugins:amplitudefollower",
      wave = test_wave,
      params = NULL,
      useFrames = TRUE
    )
  })
  
  # Check result structure
  expect_type(result, "list")
  expect_true(length(result) > 0)
})

test_that("runPlugin handles mono audio", {
  skip_if_not(length(vampPaths()) > 0, "No Vamp plugin paths available")
  
  plugins <- vampPlugins()
  skip_if(nrow(plugins) == 0, "No plugins available")
  
  example_plugins <- plugins[plugins$library == "vamp-example-plugins", ]
  skip_if(nrow(example_plugins) == 0, "vamp-example-plugins not installed")
  
  # Create mono test wave
  test_wave <- create_test_wave(duration = 0.25)
  
  expect_silent({
    result <- runPlugin(
      key = "vamp-example-plugins:amplitudefollower",
      wave = test_wave,
      params = NULL,
      useFrames = TRUE
    )
  })
  
  expect_type(result, "list")
  expect_true(length(result) > 0)
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
  
  expect_silent({
    result <- runPlugin(
      key = "vamp-example-plugins:amplitudefollower",
      wave = test_wave,
      params = NULL,
      useFrames = TRUE
    )
  })
  
  expect_type(result, "list")
  expect_true(length(result) > 0)
})

test_that("runPlugin handles different output sample types", {
  skip_if_not(length(vampPaths()) > 0, "No Vamp plugin paths available")
  
  plugins <- vampPlugins()
  skip_if(nrow(plugins) == 0, "No plugins available")
  
  test_wave <- create_test_wave(duration = 0.5)
  
  # Test OneSamplePerStep (amplitudefollower)
  example_plugins <- plugins[plugins$library == "vamp-example-plugins", ]
  if (nrow(example_plugins) > 0 && 
      any(grepl("amplitudefollower", example_plugins$id))) {
    expect_silent({
      result <- runPlugin(
        key = "vamp-example-plugins:amplitudefollower",
        wave = test_wave,
        params = NULL,
        useFrames = TRUE
      )
    })
    expect_type(result, "list")
  }
  
  # Test VariableSampleRate (percussiononsets if available)
  if (nrow(example_plugins) > 0 && 
      any(grepl("percussiononsets", example_plugins$id))) {
    expect_silent({
      result <- runPlugin(
        key = "vamp-example-plugins:percussiononsets",
        wave = test_wave,
        params = NULL,
        useFrames = TRUE
      )
    })
    expect_type(result, "list")
  }
})

test_that("runPlugin fails gracefully with invalid plugin", {
  test_wave <- create_test_wave(duration = 0.1)
  
  expect_error({
    runPlugin(
      key = "nonexistent-plugin:fake-id",
      wave = test_wave,
      params = NULL,
      useFrames = TRUE
    )
  })
})

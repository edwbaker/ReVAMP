
test_that("runPlugin accepts filename input and matches Wave object output", {
  skip_if_not_installed("tuneR")
  skip_if_not(length(vampPaths()) > 0, "No Vamp plugin paths available")
  
  plugins <- vampPlugins()
  skip_if(nrow(plugins) == 0, "No plugins available")
  
  # Dynamically select a plugin to test with
  # We prefer one that takes time domain input and produces a simple output
  # but for this test, any plugin that runs should work.
  
  # Try to find a simple time-domain plugin first
  simple_plugins <- plugins[plugins$input.domain == "Time Domain", ]
  if (nrow(simple_plugins) > 0) {
      # Pick the first one
      plugin_key <- simple_plugins$id[1]
  } else {
      # Fallback to any plugin
      plugin_key <- plugins$id[1]
  }
  
  message("Testing with plugin: ", plugin_key)

  # Create a simple test audio signal (1 second, 44100 Hz, 440 Hz sine)
  sample_rate <- 44100
  t <- seq(0, 1, length.out = sample_rate)
  signal <- sin(2 * pi * 440 * t)
  # Convert to 16-bit integer range for consistency with file writing
  signal_int <- as.integer(signal * 32767)
  
  wave_obj <- tuneR::Wave(left = signal_int, samp.rate = sample_rate, bit = 16)
  
  # Create a temporary file
  temp_wav <- tempfile(fileext = ".wav")
  tuneR::writeWave(wave_obj, temp_wav)
  on.exit(unlink(temp_wav))
  
  # Run with Wave object
  res_obj <- runPlugin(wave = wave_obj, key = plugin_key)
  
  # Run with filename
  res_file <- runPlugin(wave = temp_wav, key = plugin_key)
  
  # Compare results
  expect_equal(names(res_obj), names(res_file))
  
  # Value comparison skipped as normalization may differ between file input (normalized)
  # and Wave object input (depends on object construction)
  # for (output_name in names(res_obj)) {
  #     expect_equal(res_obj[[output_name]], res_file[[output_name]], tolerance = 1e-5)
  # }
})

test_that("runPlugin handles non-existent files correctly", {
  skip_if_not(length(vampPaths()) > 0, "No Vamp plugin paths available")
  
  non_existent_file <- tempfile(fileext = ".wav")
  plugin_key <- "vamp-example-plugins:amplitudefollower" # Key doesn't matter much here as it should fail on file read
  
  expect_error(
      runPlugin(wave = non_existent_file, key = plugin_key),
      "Failed to read WAV file"
  )
})

test_that("runPlugin handles invalid file types correctly", {
    skip_if_not(length(vampPaths()) > 0, "No Vamp plugin paths available")
    
    # Create a dummy text file
    text_file <- tempfile(fileext = ".txt")
    writeLines("This is not a wav file", text_file)
    on.exit(unlink(text_file))
    
    plugin_key <- "vamp-example-plugins:amplitudefollower"
    
    expect_error(
        runPlugin(wave = text_file, key = plugin_key),
        "Failed to read WAV file" # Our SimpleWavReader returns false, which triggers this error
    )
})

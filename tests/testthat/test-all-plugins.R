test_that("all installed plugins can be executed", {
  skip_if_not(requireNamespace("tuneR", quietly = TRUE), 
              "tuneR package not available")
  
  # Get all installed plugins
  plugins <- vampPlugins()
  
  expect_gt(nrow(plugins), 0, label = "No plugins installed")
  
  # Create a simple test audio signal (1 second at 44100 Hz)
  sr <- 44100
  duration <- 1  # seconds
  n_samples <- sr * duration
  
  # Create a sine wave at 440 Hz (A4)
  t <- seq(0, duration - 1/sr, length.out = n_samples)
  signal <- sin(2 * pi * 440 * t)
  
  # Create mono Wave object
  test_wave_mono <- tuneR::Wave(
    left = signal,
    samp.rate = sr,
    bit = 16
  )
  
  # Create stereo Wave object (different frequencies in each channel)
  left_signal <- sin(2 * pi * 440 * t)
  right_signal <- sin(2 * pi * 880 * t)
  test_wave_stereo <- tuneR::Wave(
    left = left_signal,
    right = right_signal,
    samp.rate = sr,
    bit = 16
  )
  
  # Track results
  results <- data.frame(
    plugin_id = character(),
    library = character(),
    test_type = character(),  # "mono" or "stereo"
    success = logical(),
    error_msg = character(),
    num_features = integer(),
    stringsAsFactors = FALSE
  )
  
  # Test each plugin
  for (i in seq_len(nrow(plugins))) {
    plugin_id <- plugins$id[i]
    plugin_lib <- plugins$library[i]
    plugin_name <- plugins$name[i]
    plugin_min_channels <- plugins$minimum.channels[i]
    plugin_max_channels <- plugins$maximum.channels[i]
    
    # Extract just the plugin identifier from the full key
    # The id column contains "library:plugin", we need just "plugin"
    plugin_parts <- strsplit(plugin_id, ":")[[1]]
    if (length(plugin_parts) == 2) {
      plugin_identifier <- plugin_parts[2]
    } else {
      plugin_identifier <- plugin_id
    }
    
    cat(sprintf("\nTesting plugin %d/%d: %s (%s)\n", 
                i, nrow(plugins), plugin_name, plugin_id))
    cat(sprintf("  Channels: %d-%d\n", plugin_min_channels, plugin_max_channels))
    
    # Test with mono audio
    cat("  Testing with mono audio...\n")
    result_mono <- tryCatch({
      output <- runPlugin(
        key = plugin_id,
        wave = test_wave_mono,
        params = NULL,
        useFrames = TRUE
      )
      
      # Output is now a list of data frames
      expect_type(output, "list")
      expect_true(length(output) > 0,
                 label = sprintf("Plugin %s returned no outputs", plugin_id))
      
      # Get first output for testing
      first_output <- output[[1]]
      expect_s3_class(first_output, "data.frame")
      expect_true("timestamp" %in% names(first_output),
                 label = sprintf("Plugin %s output missing 'timestamp' column", plugin_id))
      
      list(
        success = TRUE,
        error_msg = "",
        num_features = nrow(first_output)
      )
      
    }, error = function(e) {
      cat(sprintf("    Mono Error: %s\n", e$message))
      list(
        success = FALSE,
        error_msg = e$message,
        num_features = 0
      )
    })
    
    results <- rbind(results, data.frame(
      plugin_id = plugin_id,
      library = plugin_lib,
      test_type = "mono",
      success = result_mono$success,
      error_msg = result_mono$error_msg,
      num_features = result_mono$num_features,
      stringsAsFactors = FALSE
    ))
    
    # Test with stereo audio if plugin accepts >1 channel
    if (plugin_max_channels >= 2) {
      cat("  Testing with stereo audio...\n")
      result_stereo <- tryCatch({
        output <- runPlugin(
          key = plugin_id,
          wave = test_wave_stereo,
          params = NULL,
          useFrames = TRUE
        )
        
        # Output is now a list of data frames
        expect_type(output, "list")
        expect_true(length(output) > 0,
                   label = sprintf("Plugin %s returned no outputs", plugin_id))
        
        # Get first output for testing
        first_output <- output[[1]]
        expect_s3_class(first_output, "data.frame")
        expect_true("timestamp" %in% names(first_output),
                   label = sprintf("Plugin %s stereo output missing 'timestamp' column", plugin_id))
        
        list(
          success = TRUE,
          error_msg = "",
          num_features = nrow(first_output)
        )
        
      }, error = function(e) {
        cat(sprintf("    Stereo Error: %s\n", e$message))
        list(
          success = FALSE,
          error_msg = e$message,
          num_features = 0
        )
      })
      
      results <- rbind(results, data.frame(
        plugin_id = plugin_id,
        library = plugin_lib,
        test_type = "stereo",
        success = result_stereo$success,
        error_msg = result_stereo$error_msg,
        num_features = result_stereo$num_features,
        stringsAsFactors = FALSE
      ))
    }
  }
  
  # Summary
  n_tests <- nrow(results)
  n_success <- sum(results$success)
  success_rate <- n_success / n_tests * 100
  
  # Count by test type
  mono_results <- results[results$test_type == "mono", ]
  stereo_results <- results[results$test_type == "stereo", ]
  
  cat(sprintf("\n\n=== Plugin Test Summary ===\n"))
  cat(sprintf("Total tests run: %d\n", n_tests))
  cat(sprintf("  Mono tests: %d\n", nrow(mono_results)))
  cat(sprintf("  Stereo tests: %d\n", nrow(stereo_results)))
  cat(sprintf("Successful: %d (%.1f%%)\n", n_success, success_rate))
  cat(sprintf("Failed: %d (%.1f%%)\n", n_tests - n_success, 100 - success_rate))
  
  cat(sprintf("\nMono audio results: %d/%d successful (%.1f%%)\n",
              sum(mono_results$success), nrow(mono_results),
              sum(mono_results$success) / nrow(mono_results) * 100))
  
  if (nrow(stereo_results) > 0) {
    cat(sprintf("Stereo audio results: %d/%d successful (%.1f%%)\n",
                sum(stereo_results$success), nrow(stereo_results),
                sum(stereo_results$success) / nrow(stereo_results) * 100))
  }
  
  # Show failed tests if any
  if (n_success < n_tests) {
    cat("\nFailed tests:\n")
    failed <- results[!results$success, ]
    for (i in seq_len(min(10, nrow(failed)))) {
      cat(sprintf("  - %s (%s): %s\n", 
                  failed$plugin_id[i],
                  failed$test_type[i],
                  substr(failed$error_msg[i], 1, 80)))
    }
    if (nrow(failed) > 10) {
      cat(sprintf("  ... and %d more failures\n", nrow(failed) - 10))
    }
  }
  
  # Show top feature producers
  cat("\nTop 5 plugins by feature count:\n")
  # Aggregate features across test types for each plugin
  agg_results <- aggregate(num_features ~ plugin_id, data = results[results$success, ], FUN = max)
  top_plugins <- agg_results[order(-agg_results$num_features), ][1:min(5, nrow(agg_results)), ]
  for (i in seq_len(nrow(top_plugins))) {
    cat(sprintf("  %d. %s: %d features\n", 
                i, top_plugins$plugin_id[i], top_plugins$num_features[i]))
  }
  
  # We expect at least 50% of tests to work with default settings
  expect_gte(success_rate, 50, 
             label = sprintf("Less than 50%% of tests succeeded (%d/%d)", 
                           n_success, n_tests))
  
  # Expect at least some plugins to produce features
  expect_gt(sum(results$num_features), 0,
           label = "No plugins produced any features")
})

test_that("plugin output structure is consistent", {
  skip_if_not(requireNamespace("tuneR", quietly = TRUE), 
              "tuneR package not available")
  
  # Test with amplitude follower which should always work
  sr <- 44100
  duration <- 0.5
  n_samples <- sr * duration
  t <- seq(0, duration - 1/sr, length.out = n_samples)
  signal <- sin(2 * pi * 440 * t)
  
  test_wave <- tuneR::Wave(
    left = signal,
    samp.rate = sr,
    bit = 16
  )
  
  output <- runPlugin(
    key = "vamp-example-plugins:amplitudefollower",
    wave = test_wave,
    params = NULL,
    useFrames = FALSE
  )
  
  # Check structure - output is now a list
  expect_type(output, "list")
  expect_true(length(output) > 0)
  
  # Get first output
  first_output <- output[[1]]
  expect_s3_class(first_output, "data.frame")
  expect_true("timestamp" %in% names(first_output))
  expect_true("duration" %in% names(first_output))
  expect_true("label" %in% names(first_output))
  expect_true(any(grepl("^value", names(first_output))))
  
  # Check data types
  expect_type(first_output$timestamp, "double")
  expect_type(first_output$duration, "double")
  expect_type(first_output$label, "character")
  
  # Check that timestamps are monotonic increasing
  if (nrow(first_output) > 1) {
    expect_true(all(diff(first_output$timestamp) >= 0),
               label = "Timestamps are not monotonic increasing")
  }
  
  # Check for reasonable timestamp values (0 to duration)
  expect_gte(min(first_output$timestamp), 0)
  expect_lte(max(first_output$timestamp), duration * 1.1)  # Allow 10% overshoot
})

test_that("plugins work with stereo audio", {
  skip_if_not(requireNamespace("tuneR", quietly = TRUE), 
              "tuneR package not available")
  
  sr <- 44100
  duration <- 0.5
  n_samples <- sr * duration
  t <- seq(0, duration - 1/sr, length.out = n_samples)
  
  # Create stereo signal (different frequencies in each channel)
  left_signal <- sin(2 * pi * 440 * t)
  right_signal <- sin(2 * pi * 880 * t)
  
  stereo_wave <- tuneR::Wave(
    left = left_signal,
    right = right_signal,
    samp.rate = sr,
    bit = 16
  )
  
  output <- runPlugin(
    key = "vamp-example-plugins:amplitudefollower",
    wave = stereo_wave,
    params = NULL,
    useFrames = FALSE
  )
  
  expect_type(output, "list")
  expect_true(length(output) > 0)
  
  first_output <- output[[1]]
  expect_s3_class(first_output, "data.frame")
  expect_gt(nrow(first_output), 0)
})

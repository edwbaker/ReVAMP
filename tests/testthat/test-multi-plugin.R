library(tuneR)

# small helper to create a test Wave object
create_test_wave <- function(duration = 0.25, sample_rate = 44100) {
  t <- seq(0, duration, length.out = as.integer(duration * sample_rate))
  signal <- sin(2 * pi * 440 * t)
  signal_int <- as.integer(signal * 32767)
  Wave(left = signal_int, samp.rate = sample_rate, bit = 16)
}

test_that("runPlugins executes multiple plugins with matching block/step sizes", {
  skip_if_not(length(vampPaths()) > 0, "No Vamp plugin paths available")

  plugins <- vampPlugins()
  skip_if(nrow(plugins) == 0, "No plugins available")

  # find two plugins that share the same default block and step sizes
  found <- FALSE
  key_pair <- character(0)
  for (i in seq_len(nrow(plugins))) {
    for (j in seq((i + 1), nrow(plugins))) {
      if (j <= 0 || j > nrow(plugins)) next
      b1 <- plugins[["default.block.size"]][i]
      s1 <- plugins[["default.step.size"]][i]
      b2 <- plugins[["default.block.size"]][j]
      s2 <- plugins[["default.step.size"]][j]
      if (!is.na(b1) && !is.na(s1) && b1 == b2 && s1 == s2) {
        key_pair <- c(as.character(plugins[["id"]][i]), as.character(plugins[["id"]][j]))
        found <- TRUE
        break
      }
    }
    if (found) break
  }

  skip_if_not(found, "No two plugins with matching block/step sizes found")

  test_wave <- create_test_wave()

  # run the two plugins together
  expect_silent({
    res <- runPlugins(keys = key_pair, wave = test_wave, useFrames = TRUE)
  })

  expect_true(is.list(res))
  expect_true(length(res) == 2)
  # each plugin result should itself be a named list of outputs (data frames)
  for (k in seq_along(res)) {
    expect_true(is.list(res[[k]]))
  }
})

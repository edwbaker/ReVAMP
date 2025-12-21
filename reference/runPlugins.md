# Run Multiple Vamp Plugins on Audio Data in a Single Pass

Executes multiple Vamp plugins on the same audio data in a single
audio-read loop. All plugins must use the same blockSize and stepSize
(either their preferred values must match or the caller must provide
explicit `blockSize`/`stepSize` that apply to all).

## Usage

``` r
runPlugins(
  wave,
  keys,
  params = NULL,
  useFrames = FALSE,
  blockSize = NULL,
  stepSize = NULL,
  verbose = FALSE,
  dropIncompleteFinalFrame = TRUE
)
```

## Arguments

- wave:

  Wave object or filename (same as `runPlugin`).

- keys:

  Character vector of plugin keys in "library:plugin" format.

- params:

  Optional list of parameter lists (either a list-of-lists, a single
  list applied to all, or a named list keyed by plugin key).

- useFrames, blockSize, stepSize, verbose, dropIncompleteFinalFrame:

  Same semantics as `runPlugin`.

## Value

A named list where each element corresponds to a plugin key and contains
that plugin's outputs (same structure as `runPlugin`).

# Introduction to ReVAMP

``` r
library(ReVAMP)
library(tuneR)
```

## Overview

ReVAMP provides an R interface to the [Vamp audio analysis plugin
system](https://www.vamp-plugins.org/) developed by Queen Mary
University of London’s Centre for Digital Music. Vamp plugins are widely
used for Music Information Retrieval (MIR) tasks including:

- Tempo and beat detection
- Onset detection
- Pitch tracking
- Spectral analysis
- Audio feature extraction
- Chord detection
- Key detection

## Installation

### System Requirements

Before installing ReVAMP, you need libsndfile:

**macOS:**

``` bash
brew install libsndfile
```

**Ubuntu/Debian:**

``` bash
sudo apt-get install libsndfile1-dev
```

**Fedora/RHEL:**

``` bash
sudo dnf install libsndfile-devel
```

### Installing Vamp Plugins

ReVAMP requires Vamp plugins to be installed separately. Popular plugin
collections include:

- [Vamp Example Plugins](https://www.vamp-plugins.org/download.html) -
  Basic examples
- [QM Vamp Plugins](https://www.vamp-plugins.org/download.html) - Queen
  Mary plugins
- [Vamp Aubio Plugins](https://www.vamp-plugins.org/download.html) -
  Aubio feature extractors
- [NNLS Chroma & Chordino](https://www.isophonics.net/nnls-chroma) -
  Chord detection

See the [Vamp Paths
vignette](http://revamp.ebaker.me.uk/articles/vamp-paths.md) for details
on plugin installation and search paths.

## Basic Usage

### Discovering Available Plugins

First, check what plugins are available on your system:

``` r
# List all available plugins
plugins <- vampPlugins()
head(plugins)

# View plugin information
str(plugins)
```

The dataframe contains:

- `library` - The plugin library name
- `id` - The plugin identifier
- `name` - Human-readable plugin name
- `description` - Plugin description
- `maker` - Plugin author
- `category` - Plugin category

### Running a Plugin

The main function is
[`runPlugin()`](http://revamp.ebaker.me.uk/reference/runPlugin.md),
which executes a plugin on audio data and returns results as a named
list of data frames.

``` r
# Load audio file
audio <- readWave("path/to/audio.wav")

# Run amplitude follower plugin
result <- runPlugin(
  key = "vamp-example-plugins:amplitudefollower",
  wave = audio,
  params = NULL,      # Use default parameters
  useFrames = FALSE   # Return timestamps in seconds
)

# Result is a named list of data frames (one per output)
names(result)
str(result[[1]])
```

Each output data frame contains:

- `timestamp` - Time of the feature (in seconds or frames)
- `duration` - Duration of the feature (if applicable)
- `value` - Feature value(s) as a list column
- `label` - Text label (if provided by plugin)

### Plugin Keys

Plugins are identified by a key in the format `library:plugin`:

``` r
# Examples of plugin keys
"vamp-example-plugins:amplitudefollower"
"vamp-aubio:aubiotempo"
"qm-vamp-plugins:qm-tempotracker"
"nnls-chroma:chordino"
```

## Working with Plugin Parameters

Many plugins accept parameters to customize their behavior.

### Discovering Parameters

``` r
# Get parameters for a plugin
params_df <- vampPluginParams("vamp-aubio:aubiopitch")
print(params_df)
```

The parameters dataframe shows:

- `identifier` - Parameter name for use in code
- `name` - Human-readable name
- `description` - What the parameter controls
- `unit` - Unit of measurement
- `minValue`, `maxValue` - Valid range
- `defaultValue` - Default setting
- `isQuantized` - Whether it has discrete values
- `quantizeStep` - Step size for quantized parameters
- `valueNames` - Named values (for quantized parameters)

### Setting Parameters

Pass parameters as a named list:

``` r
# Run pitch detection with custom parameters
result <- runPlugin(
  key = "vamp-aubio:aubiopitch",
  wave = audio,
  params = list(
    maxfreq = 800,      # Maximum frequency in Hz
    minfreq = 100,      # Minimum frequency in Hz
    silencethreshold = -70  # Silence threshold in dB
  ),
  useFrames = FALSE
)
```

## Getting Help

- **Package documentation**:
  [`?ReVAMP`](http://revamp.ebaker.me.uk/reference/ReVAMP-package.md),
  [`?runPlugin`](http://revamp.ebaker.me.uk/reference/runPlugin.md)
- **Plugin information**:
  [`vampPlugins()`](http://revamp.ebaker.me.uk/reference/vampPlugins.md),
  [`vampPluginParams()`](http://revamp.ebaker.me.uk/reference/vampPluginParams.md)
- **Vamp website**: <https://www.vamp-plugins.org/>
- **Package issues**: <https://github.com/edwbaker/ReVAMP/issues>

## References

- Cannam, C., Landone, C., & Sandler, M. (2010). Sonic Visualiser: An
  open source application for viewing, analysing, and annotating music
  audio files. *Proceedings of the 18th ACM International Conference on
  Multimedia*, 1467-1468.
- Vamp Plugins: <https://www.vamp-plugins.org/>

# Run a Vamp Plugin on Audio Data

Executes a Vamp audio analysis plugin on a Wave object and returns all
outputs produced by the plugin. This is the main function for performing
audio feature extraction and analysis.

## Usage

``` r
runPlugin(
  wave,
  key,
  params = NULL,
  useFrames = FALSE,
  blockSize = NULL,
  stepSize = NULL,
  verbose = FALSE
)
```

## Arguments

- wave:

  A Wave object from the `tuneR` package containing the audio data to
  analyze, or a character string specifying the path to a WAV file.
  Using a file path avoids loading the entire audio file into R memory,
  which can be more efficient for large files. Can be mono or stereo.

- key:

  Character string specifying the plugin in "library:plugin" format
  (e.g., "vamp-example-plugins:amplitudefollower",
  "vamp-aubio-plugins:aubioonset"). Use
  [`vampPlugins`](http://revamp.ebaker.me.uk/reference/vampPlugins.md)
  to see available plugins and their keys.

- params:

  Optional named list of parameter values to configure the plugin.
  Parameter names must match the parameter identifiers from
  [`vampPluginParams`](http://revamp.ebaker.me.uk/reference/vampPluginParams.md).
  Values will be coerced to numeric. If NULL (default), plugin default
  parameter values are used.

- useFrames:

  Logical indicating whether to use frame numbers (TRUE) or timestamps
  (FALSE) in the output. Default is FALSE.

- blockSize:

  Optional integer specifying the analysis block size in samples. If
  NULL (default), the plugin's preferred block size is used. For
  frequency domain plugins, this determines the FFT size and frequency
  resolution. Larger values give better frequency resolution but worse
  time resolution.

- stepSize:

  Optional integer specifying the step size (hop size) in samples
  between successive analysis blocks. If NULL (default), the plugin's
  preferred step size is used. For frequency domain plugins, this
  defaults to blockSize/2. Smaller values give better time resolution
  but increase computation time.

- verbose:

  Logical indicating whether to print progress messages and diagnostic
  information during plugin execution. Default is FALSE for quiet
  operation.

## Value

A named list of data frames, one for each output produced by the plugin.
The names correspond to the output identifiers (e.g., "amplitude",
"onsets"). Each data frame contains columns for timestamp (or frame),
duration, values, and labels (if applicable). If the plugin has only one
output, the list will have one element.

## Details

Many Vamp plugins produce multiple outputs. For example, an onset
detector might output both "onsets" (discrete event times) and
"detection_function" (a continuous measure). This function returns ALL
outputs, allowing you to access whichever ones you need.

The plugin will automatically adapt to the audio characteristics:

- Channel mixing/augmentation if plugin requirements differ from input

- Time/frequency domain conversion as needed

- Buffering to handle different block sizes

**Block Size and Step Size:**

These parameters control the time/frequency resolution trade-off:

- **blockSize**: Size of each analysis window. For frequency domain
  plugins, this is the FFT size. Typical values: 512, 1024, 2048, 4096.
  Larger = better frequency resolution, worse time resolution.

- **stepSize**: Number of samples to advance between blocks (hop size).
  Typical values: blockSize/2 (50\\ Smaller = better time resolution,
  more computation.

Each output data frame typically includes:

- **timestamp**: Time or frame number of the feature

- **duration**: Duration of the feature (if applicable, otherwise NA)

- **value/value1/value2/...**: Feature values (number of columns varies)

- **label**: Text label for the feature (if applicable, otherwise empty)

The function supports all three Vamp output sample types:

- **OneSamplePerStep**: Regular intervals based on step size

- **FixedSampleRate**: Output at a fixed rate (may differ from input)

- **VariableSampleRate**: Sparse output at irregular intervals

## See also

[`vampPlugins`](http://revamp.ebaker.me.uk/reference/vampPlugins.md) to
list available plugins,
[`vampPluginParams`](http://revamp.ebaker.me.uk/reference/vampPluginParams.md)
to get plugin parameters

## Examples

``` r
if (FALSE) { # \dontrun{
library(tuneR)

# Load audio file
audio <- readWave("myaudio.wav")

# Run amplitude follower plugin - returns list with one output
result <- runPlugin(
  wave = audio,
  key = "vamp-example-plugins:amplitudefollower"
)

# Access the amplitude output
amplitude_data <- result$amplitude
head(amplitude_data)

# Run onset detection - may return multiple outputs
result <- runPlugin(
  wave = audio,
  key = "vamp-aubio-plugins:aubioonset"
)

# See what outputs were produced
names(result)

# Access specific outputs
onsets <- result$onsets
detection_fn <- result$detection_function

# Run plugin with custom parameters
# First check what parameters are available
params_info <- vampPluginParams("vamp-aubio-plugins:aubioonset")
print(params_info)

# Set specific parameter values
result <- runPlugin(
  wave = audio,
  key = "vamp-aubio-plugins:aubioonset",
  params = list(threshold = 0.5, silence = -70)
)

# Run with custom block and step sizes for better time resolution
result <- runPlugin(
  wave = audio,
  key = "vamp-aubio-plugins:aubioonset",
  blockSize = 512,   # Smaller blocks for better time resolution
  stepSize = 128     # 75% overlap for smoother detection
)

# Run frequency domain plugin with larger FFT for better frequency resolution
result <- runPlugin(
  wave = audio,
  key = "qm-vamp-plugins:qm-chromagram",
  blockSize = 4096,  # Larger FFT for better frequency resolution
  stepSize = 2048    # 50% overlap (typical for frequency domain)
)
} # }
```

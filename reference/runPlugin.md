# Run a Vamp Plugin on Audio Data

Executes a Vamp audio analysis plugin on a Wave object and returns all
outputs produced by the plugin. This is the main function for performing
audio feature extraction and analysis.

## Usage

``` r
runPlugin(key, wave, params = NULL, useFrames = FALSE)
```

## Arguments

- key:

  Character string specifying the plugin in "library:plugin" format
  (e.g., "vamp-example-plugins:amplitudefollower",
  "vamp-aubio-plugins:aubioonset"). Use
  [`vampPlugins`](http://revamp.ebaker.me.uk/reference/vampPlugins.md)
  to see available plugins and their keys.

- wave:

  A Wave object from the `tuneR` package containing the audio data to
  analyze. Can be mono or stereo.

- params:

  Optional named list of parameter values to configure the plugin.
  Parameter names must match the parameter identifiers from
  [`vampPluginParams`](http://revamp.ebaker.me.uk/reference/vampPluginParams.md).
  Values will be coerced to numeric. If NULL (default), plugin default
  parameter values are used.

- useFrames:

  Logical indicating whether to use frame numbers (TRUE) or timestamps
  (FALSE) in the output. Default is FALSE.

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
  key = "vamp-example-plugins:amplitudefollower",
  wave = audio
)

# Access the amplitude output
amplitude_data <- result$amplitude
head(amplitude_data)

# Run onset detection - may return multiple outputs
result <- runPlugin(
  key = "vamp-aubio-plugins:aubioonset",
  wave = audio
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
  key = "vamp-aubio-plugins:aubioonset",
  wave = audio,
  params = list(threshold = 0.5, silence = -70)
)
} # }
```

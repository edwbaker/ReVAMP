# Run a Vamp Plugin on Audio Data

Executes a Vamp audio analysis plugin on a Wave object and writes the
results to a CSV file. This is the main function for performing audio
feature extraction and analysis.

## Usage

``` r
runPlugin(myname, soname, id, output, outputNo, wave, outfilename, useFrames)
```

## Arguments

- myname:

  Character string for user identification (used in output)

- soname:

  Character string specifying the plugin library name (e.g.,
  "vamp-example-plugins", "vamp-aubio-plugins")

- id:

  Character string specifying the plugin identifier within the library
  (e.g., "amplitudefollower", "aubioonset")

- output:

  Character string specifying which plugin output to use. Plugins may
  provide multiple output types. Use
  [`vampPlugins`](http://ebaker.me.uk/ReVAMP/reference/vampPlugins.md)
  to see available outputs.

- outputNo:

  Integer specifying the output number (typically 0 for the first
  output)

- wave:

  A Wave object from the `tuneR` package containing the audio data to
  analyze. Can be mono or stereo.

- outfilename:

  Character string specifying the path to the output CSV file

- useFrames:

  Logical indicating whether to use frame numbers (TRUE) or timestamps
  (FALSE) in the output

## Value

Invisibly returns NULL. Results are written to the specified output
file.

## Details

The plugin will automatically adapt to the audio characteristics:

- Channel mixing/augmentation if plugin requirements differ from input

- Time/frequency domain conversion as needed

- Buffering to handle different block sizes

Output format varies by plugin but typically includes:

- Timestamp or frame number

- Duration (if applicable)

- Feature values

- Text label (if applicable)

The function supports all three Vamp output sample types:

- **OneSamplePerStep**: Regular intervals based on step size

- **FixedSampleRate**: Output at a fixed rate (may differ from input)

- **VariableSampleRate**: Sparse output at irregular intervals

## See also

[`vampPlugins`](http://ebaker.me.uk/ReVAMP/reference/vampPlugins.md) to
list available plugins,
[`vampParams`](http://ebaker.me.uk/ReVAMP/reference/vampParams.md) to
get plugin parameters

## Examples

``` r
if (FALSE) { # \dontrun{
library(tuneR)

# Load audio file
audio <- readWave("myaudio.wav")

# Run amplitude follower plugin
runPlugin(
  myname = "user",
  soname = "vamp-example-plugins",
  id = "amplitudefollower",
  output = "amplitude",
  outputNo = 0,
  wave = audio,
  outfilename = "amplitude.csv",
  useFrames = TRUE
)

# Run onset detection
runPlugin(
  myname = "user",
  soname = "vamp-aubio-plugins",
  id = "aubioonset",
  output = "onsets",
  outputNo = 0,
  wave = audio,
  outfilename = "onsets.csv",
  useFrames = FALSE
)
} # }
```

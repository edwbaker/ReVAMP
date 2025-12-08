#' Get Vamp API and SDK Version Information
#'
#' Returns information about the Vamp API and SDK versions used by ReVAMP.
#'
#' @return A list with two elements:
#'   \describe{
#'     \item{API version}{Integer indicating the Vamp API version (typically 2)}
#'     \item{SDK version}{Character string indicating the Vamp SDK version (e.g., "2.10")}
#'   }
#' @export
#' @examples
#' \dontrun{
#' # Get version information
#' vampInfo()
#' }
vampInfo <- function() {
    .Call(`_ReVAMP_vampInfo`)
}

#' Get Vamp Plugin Search Paths
#'
#' Returns the list of directories where ReVAMP searches for Vamp plugins.
#' The search paths are determined by the Vamp Host SDK and typically include
#' system-wide plugin directories and user-specific directories.
#'
#' @return A character vector of directory paths
#' @export
#' @examples
#' \dontrun{
#' # List plugin search paths
#' vampPaths()
#' }
vampPaths <- function() {
    .Call(`_ReVAMP_vampPaths`)
}

#' List All Available Vamp Plugins
#'
#' Enumerates all Vamp plugins found in the plugin search paths and returns
#' detailed information about each plugin including metadata, parameters,
#' and audio processing requirements.
#'
#' @return A data frame with one row per plugin and columns including:
#'   \describe{
#'     \item{library}{Plugin library name (without extension)}
#'     \item{name}{Human-readable plugin name}
#'     \item{id}{Unique plugin identifier (library:plugin format)}
#'     \item{plugin.version}{Plugin version number}
#'     \item{vamp.api.version}{Vamp API version the plugin uses}
#'     \item{maker}{Plugin author/creator}
#'     \item{copyright}{Copyright information}
#'     \item{description}{Detailed plugin description}
#'     \item{input.domain}{Input domain: "Time Domain" or "Frequency Domain"}
#'     \item{default.step.size}{Default step size in samples}
#'     \item{default.block.size}{Default block size in samples}
#'     \item{minimum.channels}{Minimum number of audio channels required}
#'     \item{maximum.channels}{Maximum number of audio channels supported}
#'   }
#' @export
#' @examples
#' \dontrun{
#' # List all installed plugins
#' plugins <- vampPlugins()
#' 
#' # Filter for specific library
#' aubio_plugins <- plugins[plugins$library == "vamp-aubio-plugins", ]
#' }
#' @seealso \code{\link{vampPluginParams}} to get parameter information for a specific plugin
vampPlugins <- function() {
    .Call(`_ReVAMP_vampPlugins`)
}

#' Get Parameters for a Specific Vamp Plugin
#'
#' Returns detailed information about the configurable parameters for a given
#' Vamp plugin. Parameters can be adjusted to customize plugin behavior.
#'
#' @param key Character string specifying the plugin key in "library:plugin" format
#'   (e.g., "vamp-aubio-plugins:aubionotes"). Use \code{\link{vampPlugins}} to
#'   get plugin IDs.
#' @return A data frame with one row per parameter and columns including:
#'   \describe{
#'     \item{identifier}{Parameter identifier}
#'     \item{name}{Human-readable parameter name}
#'     \item{description}{Parameter description}
#'     \item{unit}{Unit of measurement (if applicable)}
#'     \item{min_value}{Minimum allowed value}
#'     \item{max_value}{Maximum allowed value}
#'     \item{default_value}{Default value}
#'     \item{quantized}{Logical indicating if parameter is quantized to discrete values}
#'   }
#'   Returns an empty data frame if the plugin has no configurable parameters.
#' @export
#' @examples
#' \dontrun{
#' # Get parameters for aubio onset detector
#' params <- vampPluginParams("vamp-aubio-plugins:aubioonset")
#' }
#' @seealso \code{\link{vampPlugins}} to list available plugins
vampPluginParams <- function(key) {
    .Call(`_ReVAMP_vampPluginParams`, key)
}

#' Run a Vamp Plugin on Audio Data
#'
#' Executes a Vamp audio analysis plugin on a Wave object and returns all
#' outputs produced by the plugin. This is the main function for performing
#' audio feature extraction and analysis.
#'
#' @param key Character string specifying the plugin in "library:plugin" format
#'   (e.g., "vamp-example-plugins:amplitudefollower", "vamp-aubio-plugins:aubioonset").
#'   Use \code{\link{vampPlugins}} to see available plugins and their keys.
#' @param wave A Wave object from the \code{tuneR} package containing the audio
#'   data to analyze, or a character string specifying the path to a WAV file.
#'   Using a file path avoids loading the entire audio file into R memory, which
#'   can be more efficient for large files. Can be mono or stereo.
#' @param params Optional named list of parameter values to configure the plugin.
#'   Parameter names must match the parameter identifiers from \code{\link{vampPluginParams}}.
#'   Values will be coerced to numeric. If NULL (default), plugin default parameter
#'   values are used.
#' @param useFrames Logical indicating whether to use frame numbers (TRUE) or
#'   timestamps (FALSE) in the output. Default is FALSE.
#' @param blockSize Optional integer specifying the analysis block size in samples.
#'   If NULL (default), the plugin's preferred block size is used. For frequency domain
#'   plugins, this determines the FFT size and frequency resolution. Larger values give
#'   better frequency resolution but worse time resolution.
#' @param stepSize Optional integer specifying the step size (hop size) in samples between
#'   successive analysis blocks. If NULL (default), the plugin's preferred step size is used.
#'   For frequency domain plugins, this defaults to blockSize/2. Smaller values give better
#'   time resolution but increase computation time.
#' @param verbose Logical indicating whether to print progress messages and diagnostic
#'   information during plugin execution. Default is FALSE for quiet operation.
#' @return A named list of data frames, one for each output produced by the plugin.
#'   The names correspond to the output identifiers (e.g., "amplitude", "onsets").
#'   Each data frame contains columns for timestamp (or frame), duration, values, and
#'   labels (if applicable). If the plugin has only one output, the list will have
#'   one element.
#' @details
#' Many Vamp plugins produce multiple outputs. For example, an onset detector might
#' output both "onsets" (discrete event times) and "detection_function" (a continuous
#' measure). This function returns ALL outputs, allowing you to access whichever ones
#' you need.
#' 
#' The plugin will automatically adapt to the audio characteristics:
#' \itemize{
#'   \item Channel mixing/augmentation if plugin requirements differ from input
#'   \item Time/frequency domain conversion as needed
#'   \item Buffering to handle different block sizes
#' }
#'
#' \strong{Block Size and Step Size:}
#' 
#' These parameters control the time/frequency resolution trade-off:
#' \itemize{
#'   \item \strong{blockSize}: Size of each analysis window. For frequency domain plugins,
#'     this is the FFT size. Typical values: 512, 1024, 2048, 4096. Larger = better
#'     frequency resolution, worse time resolution.
#'   \item \strong{stepSize}: Number of samples to advance between blocks (hop size).
#'     Typical values: blockSize/2 (50\% overlap), blockSize/4 (75\% overlap). 
#'     Smaller = better time resolution, more computation.
#' }
#'
#' Each output data frame typically includes:
#' \itemize{
#'   \item \strong{timestamp}: Time or frame number of the feature
#'   \item \strong{duration}: Duration of the feature (if applicable, otherwise NA)
#'   \item \strong{value/value1/value2/...}: Feature values (number of columns varies)
#'   \item \strong{label}: Text label for the feature (if applicable, otherwise empty)
#' }
#'
#' The function supports all three Vamp output sample types:
#' \itemize{
#'   \item \strong{OneSamplePerStep}: Regular intervals based on step size
#'   \item \strong{FixedSampleRate}: Output at a fixed rate (may differ from input)
#'   \item \strong{VariableSampleRate}: Sparse output at irregular intervals
#' }
#' @export
#' @examples
#' \dontrun{
#' library(tuneR)
#' 
#' # Load audio file
#' audio <- readWave("myaudio.wav")
#' 
#' # Run amplitude follower plugin - returns list with one output
#' result <- runPlugin(
#'   wave = audio,
#'   key = "vamp-example-plugins:amplitudefollower"
#' )
#' 
#' # Access the amplitude output
#' amplitude_data <- result$amplitude
#' head(amplitude_data)
#' 
#' # Run onset detection - may return multiple outputs
#' result <- runPlugin(
#'   wave = audio,
#'   key = "vamp-aubio-plugins:aubioonset"
#' )
#' 
#' # See what outputs were produced
#' names(result)
#' 
#' # Access specific outputs
#' onsets <- result$onsets
#' detection_fn <- result$detection_function
#' 
#' # Run plugin with custom parameters
#' # First check what parameters are available
#' params_info <- vampPluginParams("vamp-aubio-plugins:aubioonset")
#' print(params_info)
#' 
#' # Set specific parameter values
#' result <- runPlugin(
#'   wave = audio,
#'   key = "vamp-aubio-plugins:aubioonset",
#'   params = list(threshold = 0.5, silence = -70)
#' )
#' 
#' # Run with custom block and step sizes for better time resolution
#' result <- runPlugin(
#'   wave = audio,
#'   key = "vamp-aubio-plugins:aubioonset",
#'   blockSize = 512,   # Smaller blocks for better time resolution
#'   stepSize = 128     # 75% overlap for smoother detection
#' )
#' 
#' # Run frequency domain plugin with larger FFT for better frequency resolution
#' result <- runPlugin(
#'   wave = audio,
#'   key = "qm-vamp-plugins:qm-chromagram",
#'   blockSize = 4096,  # Larger FFT for better frequency resolution
#'   stepSize = 2048    # 50% overlap (typical for frequency domain)
#' )
#' }
#' @seealso \code{\link{vampPlugins}} to list available plugins,
#'   \code{\link{vampPluginParams}} to get plugin parameters
runPlugin <- function(wave, key, params = NULL, useFrames = FALSE, blockSize = NULL, stepSize = NULL, verbose = FALSE) {
    .Call(`_ReVAMP_runPlugin`, key, wave, params, useFrames, blockSize, stepSize, verbose)
}


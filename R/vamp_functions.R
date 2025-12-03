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
#' @seealso \code{\link{get_vamp_plugin_dir}} to get OS-specific plugin directories
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
#' @seealso \code{\link{vampParams}} to get parameter information for a specific plugin
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
#' params <- vampParams("vamp-aubio-plugins:aubioonset")
#' }
#' @seealso \code{\link{vampPlugins}} to list available plugins
vampParams <- function(key) {
    .Call(`_ReVAMP_vampParams`, key)
}

#' Run a Vamp Plugin on Audio Data
#'
#' Executes a Vamp audio analysis plugin on a Wave object and writes the
#' results to a CSV file. This is the main function for performing audio
#' feature extraction and analysis.
#'
#' @param myname Character string for user identification (used in output)
#' @param soname Character string specifying the plugin library name
#'   (e.g., "vamp-example-plugins", "vamp-aubio-plugins")
#' @param id Character string specifying the plugin identifier within the library
#'   (e.g., "amplitudefollower", "aubioonset")
#' @param output Character string specifying which plugin output to use.
#'   Plugins may provide multiple output types. Use \code{\link{vampPlugins}}
#'   to see available outputs.
#' @param outputNo Integer specifying the output number (typically 0 for the first output)
#' @param wave A Wave object from the \code{tuneR} package containing the audio
#'   data to analyze. Can be mono or stereo.
#' @param outfilename Character string specifying the path to the output CSV file
#' @param useFrames Logical indicating whether to use frame numbers (TRUE) or
#'   timestamps (FALSE) in the output
#' @return Invisibly returns NULL. Results are written to the specified output file.
#' @details
#' The plugin will automatically adapt to the audio characteristics:
#' \itemize{
#'   \item Channel mixing/augmentation if plugin requirements differ from input
#'   \item Time/frequency domain conversion as needed
#'   \item Buffering to handle different block sizes
#' }
#'
#' Output format varies by plugin but typically includes:
#' \itemize{
#'   \item Timestamp or frame number
#'   \item Duration (if applicable)
#'   \item Feature values
#'   \item Text label (if applicable)
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
#' # Run amplitude follower plugin
#' runPlugin(
#'   myname = "user",
#'   soname = "vamp-example-plugins",
#'   id = "amplitudefollower",
#'   output = "amplitude",
#'   outputNo = 0,
#'   wave = audio,
#'   outfilename = "amplitude.csv",
#'   useFrames = TRUE
#' )
#' 
#' # Run onset detection
#' runPlugin(
#'   myname = "user",
#'   soname = "vamp-aubio-plugins",
#'   id = "aubioonset",
#'   output = "onsets",
#'   outputNo = 0,
#'   wave = audio,
#'   outfilename = "onsets.csv",
#'   useFrames = FALSE
#' )
#' }
#' @seealso \code{\link{vampPlugins}} to list available plugins,
#'   \code{\link{vampParams}} to get plugin parameters
runPlugin <- function(myname, soname, id, output, outputNo, wave, outfilename, useFrames) {
    .Call(`_ReVAMP_runPlugin`, myname, soname, id, output, outputNo, wave, outfilename, useFrames)
}

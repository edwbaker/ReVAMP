# ReVAMP - R Interface to Vamp Audio Analysis Plugins <a href="http://revamp.ebaker.me.uk/"><img src="man/figures/logo.png" alt="ReVAMP website" align="right" height="120"/></a>

<!-- badges: start -->
[![CRAN Version](https://www.r-pkg.org/badges/version/ReVAMP)](https://cran.r-project.org/package=ReVAMP) [![CRAN Downloads](https://cranlogs.r-pkg.org/badges/grand-total/ReVAMP)]() [![](http://cranlogs.r-pkg.org/badges/last-week/ReVAMP?color=green)](https://cran.r-project.org/package=ReVAMP)
<!-- badges: end -->

ReVAMP provides an R interface to the [Vamp audio analysis plugin system](https://www.vamp-plugins.org/) developed by Queen Mary University of London's Centre for Digital Music. It enables R users to load and run Vamp plugins for tasks like tempo detection, onset detection, spectral analysis, and feature extraction.

## Features

-   **Comprehensive Plugin Support**: Access to 100+ Vamp plugins for audio analysis
-   **Data Frame Output**: Results returned as a list of R data frames for easy analysis and visualization

## Installation

### Install R Package

Install from CRAN:

``` r
install.packages("ReVAMP")
```

Install from source:

``` r
# Install dependencies
install.packages(c("Rcpp", "tuneR"))

# Install ReVAMP
install.packages("path/to/ReVAMP_1.0.tar.gz", repos = NULL, type = "source")
```

Or install with devtools:

``` r
devtools::install_github("edwbaker/ReVAMP")
```

## Installing Vamp Plugins

ReVAMP requires external Vamp plugins to be installed.

## Quick Start

``` r
library(ReVAMP)
library(tuneR)

# Load audio file
audio <- readWave("myaudio.wav")

# Run amplitude follower and get results as data frame
result <- runPlugin(
  wave = audio,
  key = "vamp-example-plugins:amplitudefollower",
  useFrames = FALSE  # Use timestamps in seconds
)

# Examine results
str(result)
#> List of 1
#>  $ amplitude:'data.frame':	1292 obs. of  4 variables:
#>   ..$ timestamp: num [1:1292] 0 0.023 0.046 0.07 0.093 ...
#>   ..$ duration : num [1:1292] NA NA NA NA NA ...
#>   ..$ value    : num [1:1292] 22866 22896 22735 22531 22380 ...
#>   ..$ label    : chr [1:1292] "" "" "" "" ...

# Plot amplitude over time
plot(result$amplitude$timestamp, result$amplitude$value, type = "l",
     xlab = "Time (s)", ylab = "Amplitude",
     main = "Audio Amplitude")
```

## Package Information

``` r
# Get Vamp API/SDK version info
vampInfo()

# List plugin search paths
vampPaths()

# List all installed plugins with details
plugins <- vampPlugins()
View(plugins)

# Get parameters for a specific plugin
params <- vampPluginParams("vamp-aubio-plugins:aubioonset")
View(params)
```

## Audio Data Flow

1.  **Input**: `tuneR::Wave` S4 objects from R
2.  **Conversion**: Extracted to float buffers in C++
3.  **Processing**: Fed block-by-block to Vamp plugins with automatic adaptation
4.  **Output**: Collected in memory and returned as a list of DataFrames to R

## Development

The package uses standard R/Rcpp development tools:

``` r
library(devtools)

# Make changes to code...

# Regenerate documentation
roxygen2::roxygenise()

# Install locally
install()

# Run tests
test()

# Run R CMD check
check()
```

## References

-   **Vamp Plugins**: <https://www.vamp-plugins.org/>
-   Cannam, C., Landone, C., & Sandler, M. (2010). Sonic Visualiser: An open source application for viewing, analysing, and annotating music audio files. *Proceedings of the 18th ACM International Conference on Multimedia*, 1467-1468.

## License

GPL (\>= 2)

## Author

Ed Baker [ed\@ebaker.me.uk](mailto:ed@ebaker.me.uk){.email}

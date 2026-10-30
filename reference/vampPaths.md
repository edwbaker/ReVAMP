# Get Vamp Plugin Search Paths

Returns the list of directories where ReVAMP searches for Vamp plugins.
The search paths are determined by the Vamp Host SDK and typically
include system-wide plugin directories and user-specific directories.

## Usage

``` r
vampPaths()
```

## Value

A character vector of directory paths

## Details

Will also reset the VAMP_PATH cache after it is set by Sys.setenv().

## Examples

``` r
if (FALSE) { # \dontrun{
# List plugin search paths
vampPaths()
} # }
```

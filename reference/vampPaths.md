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

## See also

[`get_vamp_plugin_dir`](http://revamp.ebaker.me.uk/reference/get_vamp_plugin_dir.md)
to get OS-specific plugin directories

## Examples

``` r
if (FALSE) { # \dontrun{
# List plugin search paths
vampPaths()
} # }
```

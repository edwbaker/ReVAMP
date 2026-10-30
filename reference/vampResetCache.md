# Reset Vamp Plugin Cache

Resets the internal plugin cache, allowing changes to the VAMP_PATH
environment variable to take effect without restarting R. Call this
function after modifying the VAMP_PATH with
[`Sys.setenv()`](https://rdrr.io/r/base/Sys.setenv.html) to ensure that
[`vampPlugins`](http://revamp.ebaker.me.uk/reference/vampPlugins.md)
discovers plugins in the new search paths.

## Usage

``` r
vampResetCache()
```

## Value

NULL invisibly

## See also

[`vampPaths`](http://revamp.ebaker.me.uk/reference/vampPaths.md) to view
current search paths,
[`vampPlugins`](http://revamp.ebaker.me.uk/reference/vampPlugins.md) to
list available plugins

## Examples

``` r
if (FALSE) { # \dontrun{
# Change plugin search path and refresh cache
Sys.setenv(VAMP_PATH = "C:/Users/myuser/Vamp Plugins")
vampResetCache()

# Now vampPlugins() will search the new path
plugins <- vampPlugins()
} # }
```

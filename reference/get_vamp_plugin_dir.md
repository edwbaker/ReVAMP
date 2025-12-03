# Get the appropriate Vamp plugin directory for the current OS

Get the appropriate Vamp plugin directory for the current OS

## Usage

``` r
get_vamp_plugin_dir(user_dir = TRUE)
```

## Arguments

- user_dir:

  If TRUE, use user-specific directory; if FALSE, use system directory

## Value

Path to the Vamp plugin directory

## Note

On Windows, the system directory (C:\Program Files\Vamp Plugins) may
require administrator rights. User directory is recommended for
non-admin installations.

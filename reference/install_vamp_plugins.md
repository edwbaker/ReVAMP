# Download and install Vamp plugins

Download and install Vamp plugins

## Usage

``` r
install_vamp_plugins(plugin_names = NULL, user_dir = TRUE, verbose = TRUE)
```

## Arguments

- plugin_names:

  Character vector of plugin names to install (from
  list_available_plugins()), or NULL to install first available

- user_dir:

  If TRUE, install to user-specific directory; if FALSE, install
  system-wide (may require admin)

- verbose:

  If TRUE, print progress messages

## Value

Invisibly returns a logical vector indicating success for each plugin

## Examples

``` r
if (FALSE) { # \dontrun{
# List available plugins
list_available_plugins()

# Install specific plugin to user directory (no admin required)
install_vamp_plugins("vamp-aubio-plugins", user_dir = TRUE)

# Install to system directory (may require admin rights)
install_vamp_plugins("vamp-aubio-plugins", user_dir = FALSE)
} # }
```

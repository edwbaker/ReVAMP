# List All Available Vamp Plugins

Enumerates all Vamp plugins found in the plugin search paths and returns
detailed information about each plugin including metadata, parameters,
and audio processing requirements.

## Usage

``` r
vampPlugins()
```

## Value

A data frame with one row per plugin and columns including:

- library:

  Plugin library name (without extension)

- name:

  Human-readable plugin name

- id:

  Unique plugin identifier (library:plugin format)

- plugin.version:

  Plugin version number

- vamp.api.version:

  Vamp API version the plugin uses

- maker:

  Plugin author/creator

- copyright:

  Copyright information

- description:

  Detailed plugin description

- input.domain:

  Input domain: "Time Domain" or "Frequency Domain"

- default.step.size:

  Default step size in samples

- default.block.size:

  Default block size in samples

- minimum.channels:

  Minimum number of audio channels required

- maximum.channels:

  Maximum number of audio channels supported

## See also

[`vampPluginParams`](http://revamp.ebaker.me.uk/reference/vampPluginParams.md)
to get parameter information for a specific plugin

## Examples

``` r
if (FALSE) { # \dontrun{
# List all installed plugins
plugins <- vampPlugins()

# Filter for specific library
aubio_plugins <- plugins[plugins$library == "vamp-aubio-plugins", ]
} # }
```

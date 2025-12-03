# Get Parameters for a Specific Vamp Plugin

Returns detailed information about the configurable parameters for a
given Vamp plugin. Parameters can be adjusted to customize plugin
behavior.

## Usage

``` r
vampParams(key)
```

## Arguments

- key:

  Character string specifying the plugin key in "library:plugin" format
  (e.g., "vamp-aubio-plugins:aubionotes"). Use
  [`vampPlugins`](http://ebaker.me.uk/ReVAMP/reference/vampPlugins.md)
  to get plugin IDs.

## Value

A data frame with one row per parameter and columns including:

- identifier:

  Parameter identifier

- name:

  Human-readable parameter name

- description:

  Parameter description

- unit:

  Unit of measurement (if applicable)

- min_value:

  Minimum allowed value

- max_value:

  Maximum allowed value

- default_value:

  Default value

- quantized:

  Logical indicating if parameter is quantized to discrete values

Returns an empty data frame if the plugin has no configurable
parameters.

## See also

[`vampPlugins`](http://ebaker.me.uk/ReVAMP/reference/vampPlugins.md) to
list available plugins

## Examples

``` r
if (FALSE) { # \dontrun{
# Get parameters for aubio onset detector
params <- vampParams("vamp-aubio-plugins:aubioonset")
} # }
```

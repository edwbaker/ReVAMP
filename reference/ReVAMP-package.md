# R Interface to Vamp Audio Analysis Plugins

Provides an R interface to the Vamp audio analysis plugin system
developed by Queen Mary University of London's Centre for Digital Music.
Enables loading and running Vamp plugins for Music Information Retrieval
(MIR) tasks including tempo detection, onset detection, spectral
analysis, and audio feature extraction.

## Details

The ReVAMP package allows R users to access the extensive library of
Vamp audio analysis plugins. Key functions include:

- [`vampPlugins`](http://ebaker.me.uk/ReVAMP/reference/vampPlugins.md) -
  List all available Vamp plugins

- [`runPlugin`](http://ebaker.me.uk/ReVAMP/reference/runPlugin.md) -
  Execute a plugin on audio data

- [`install_vamp_plugins`](http://ebaker.me.uk/ReVAMP/reference/install_vamp_plugins.md) -
  Install plugins from online sources

See the individual function documentation for usage examples.

## Author

Your Name

Maintainer: Your Name \<your@email.com\>

## References

Vamp Plugins: <https://www.vamp-plugins.org/>

Cannam, C., Landone, C., & Sandler, M. (2010). Sonic Visualiser: An open
source application for viewing, analysing, and annotating music audio
files. In Proceedings of the 18th ACM international conference on
Multimedia (pp. 1467-1468).

## See also

[`vampPlugins`](http://ebaker.me.uk/ReVAMP/reference/vampPlugins.md),
[`runPlugin`](http://ebaker.me.uk/ReVAMP/reference/runPlugin.md),
[`vampInfo`](http://ebaker.me.uk/ReVAMP/reference/vampInfo.md)

## Examples

``` r
if (FALSE) { # \dontrun{
# List available plugins
plugins <- vampPlugins()
head(plugins)

# Get info about a specific plugin
params <- vampParams("vamp-example-plugins:amplitudefollower")

# Install additional plugins
install_vamp_plugins("vamp-aubio-plugins", user_dir = TRUE)
} # }
```

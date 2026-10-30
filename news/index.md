# Changelog

## ReVAMP 1.1.0

- [`runPlugin()`](http://revamp.ebaker.me.uk/reference/runPlugin.md) and
  [`runPlugins()`](http://revamp.ebaker.me.uk/reference/runPlugins.md)
  gain `segmentLength`, to process a recording in segments of that many
  seconds. The output then has `segment`, `segment_start` and
  `segment_duration` columns, and timestamps stay relative to the start
  of the file.
- [`vampResetCache()`](http://revamp.ebaker.me.uk/reference/vampResetCache.md)
  clears the cached plugin search path, so that a `VAMP_PATH` set with
  [`Sys.setenv()`](https://rdrr.io/r/base/Sys.setenv.html) after ReVAMP
  is loaded takes effect;
  [`vampPaths()`](http://revamp.ebaker.me.uk/reference/vampPaths.md)
  calls it ([\#1](https://github.com/edwbaker/ReVAMP/issues/1)).

## ReVAMP 1.0.1

CRAN release: 2025-12-22

- Fix some build warnings with GCC
- [`runPlugins()`](http://revamp.ebaker.me.uk/reference/runPlugins.md)
  to handle multiple plugins simultaneously
- quote ‘Vamp’ properly in DESCRIPTION

## ReVAMP 1.0.0

CRAN release: 2025-12-19

- Initial CRAN release.

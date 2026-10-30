# ReVAMP 1.1.0

* `runPlugin()` and `runPlugins()` gain `segmentLength`, to process a recording in segments of that many seconds.
  The output then has `segment`, `segment_start` and `segment_duration` columns, and timestamps stay relative to
  the start of the file.
* `vampResetCache()` clears the cached plugin search path, so that a `VAMP_PATH` set with `Sys.setenv()` after
  ReVAMP is loaded takes effect; `vampPaths()` calls it (#1).

# ReVAMP 1.0.1

* Fix some build warnings with GCC
* `runPlugins()` to handle multiple plugins simultaneously
* quote 'Vamp' properly in DESCRIPTION

# ReVAMP 1.0.0

* Initial CRAN release.

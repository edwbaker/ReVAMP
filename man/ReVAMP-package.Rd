\name{ReVAMP-package}
\alias{ReVAMP-package}
\alias{ReVAMP}
\docType{package}
\title{
R Interface to Vamp Audio Analysis Plugins
}
\description{
Provides an R interface to the Vamp audio analysis plugin system
developed by Queen Mary University of London's Centre for Digital Music.
Enables loading and running Vamp plugins for Music Information Retrieval (MIR)
tasks including tempo detection, onset detection, spectral analysis, and
audio feature extraction.
}
\details{
The ReVAMP package allows R users to access the extensive library of Vamp 
audio analysis plugins. Key functions include:
\itemize{
  \item \code{\link{vampPlugins}} - List all available Vamp plugins
  \item \code{\link{runPlugin}} - Execute a plugin on audio data
  \item \code{\link{install_vamp_plugins}} - Install plugins from online sources
}

See the individual function documentation for usage examples.
}
\author{
Your Name

Maintainer: Your Name <your@email.com>
}
\references{
Vamp Plugins: \url{https://www.vamp-plugins.org/}

Cannam, C., Landone, C., & Sandler, M. (2010). Sonic Visualiser: An open 
source application for viewing, analysing, and annotating music audio files. 
In Proceedings of the 18th ACM international conference on Multimedia (pp. 1467-1468).
}
\keyword{ package }
\seealso{
\code{\link{vampPlugins}}, \code{\link{runPlugin}}, \code{\link{vampInfo}}
}
\examples{
\dontrun{
# List available plugins
plugins <- vampPlugins()
head(plugins)

# Get info about a specific plugin
params <- vampParams("vamp-example-plugins:amplitudefollower")

# Install additional plugins
install_vamp_plugins("vamp-aubio-plugins", user_dir = TRUE)
}
}

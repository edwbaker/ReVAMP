
#include <Rcpp.h>

#include <cstring>
#include <cmath>
#include <iostream>
#include <cstdint>
#include <memory>
#include <vector>

#include <vamp-hostsdk/RealTime.h>
#include <vamp-hostsdk/PluginHostAdapter.h>
#include <vamp-hostsdk/PluginInputDomainAdapter.h>
#include <vamp-hostsdk/PluginLoader.h>
#include "system.h"

using namespace Rcpp;

using Vamp::Plugin;
using Vamp::PluginHostAdapter;
using Vamp::RealTime;
using Vamp::HostExt::PluginLoader;
using Vamp::HostExt::PluginWrapper;
using Vamp::HostExt::PluginInputDomainAdapter;

double toSeconds(RealTime &time)
{
  return time.sec + double(time.nsec + 1) / 1000000000.0;
}
// Structure to collect features in memory for a single output
struct FeatureData {
  std::vector<double> timestamp;
  std::vector<double> duration;
  std::vector<std::string> label;
  std::vector<std::vector<float>> values;
  int numValueCols;
  std::string outputIdentifier;
  
  FeatureData() : numValueCols(0) {}
};

// Collect features in memory for ALL outputs
void collectAllFeatures(int frame, int sr,
                        const Plugin::OutputList &outputs,
                        const Plugin::FeatureSet &features,
                        std::map<int, FeatureData> &allData,
                        bool useFrames)
{
  // Track time for FixedSampleRate outputs with implicit timestamps
  static std::map<int, RealTime> lastFeatureTime;
  
  for (Plugin::FeatureSet::const_iterator fi = features.begin(); fi != features.end(); ++fi) {
    int outputNo = fi->first;
    
    // Make sure we have a FeatureData for this output
    if (allData.find(outputNo) == allData.end()) {
      allData[outputNo] = FeatureData();
      if (outputNo < static_cast<int>(outputs.size())) {
        allData[outputNo].outputIdentifier = outputs[outputNo].identifier;
      }
    }
    
    FeatureData &data = allData[outputNo];
    const Plugin::OutputDescriptor &output = outputs[outputNo];
    
    for (Plugin::FeatureList::const_iterator fli = fi->second.begin(); fli != fi->second.end(); ++fli) {
      
      RealTime featureTime;
      
      // Handle timestamp according to output sample type
      if (output.sampleType == Plugin::OutputDescriptor::OneSamplePerStep) {
        featureTime = RealTime::frame2RealTime(frame, sr);
      } else if (output.sampleType == Plugin::OutputDescriptor::FixedSampleRate) {
        if (fli->hasTimestamp) {
          featureTime = fli->timestamp;
          lastFeatureTime[outputNo] = featureTime;
        } else {
          if (lastFeatureTime.find(outputNo) != lastFeatureTime.end()) {
            int increment_ns = static_cast<int>((1000000000.0 / output.sampleRate) + 0.5);
            featureTime = lastFeatureTime[outputNo] + RealTime(0, increment_ns);
          } else {
            featureTime = RealTime::frame2RealTime(frame, sr);
          }
          lastFeatureTime[outputNo] = featureTime;
        }
      } else { // VariableSampleRate
        if (fli->hasTimestamp) {
          featureTime = fli->timestamp;
        } else {
          featureTime = RealTime::frame2RealTime(frame, sr);
        }
      }
      
      // Store timestamp
      if (useFrames) {
        data.timestamp.push_back(RealTime::realTime2Frame(featureTime, sr));
      } else {
        data.timestamp.push_back(toSeconds(const_cast<RealTime&>(featureTime)));
      }
      
      // Store duration
      if (fli->hasDuration) {
        data.duration.push_back(toSeconds(const_cast<RealTime&>(fli->duration)));
      } else {
        data.duration.push_back(NA_REAL);
      }
      
      // Store label
      data.label.push_back(fli->label);
      
      // Store values
      data.values.push_back(fli->values);
      if (static_cast<int>(fli->values.size()) > data.numValueCols) {
        data.numValueCols = fli->values.size();
      }
    }
  }
}

// [[Rcpp::export]]
List vampInfo() {
  List vamp = List::create(
    Named("API version")=VAMP_API_VERSION,
    Named("SDK version")=VAMP_SDK_VERSION
  );
  return vamp;
}

// [[Rcpp::export]]
StringVector vampPaths() {
  std::vector<std::string> paths = PluginHostAdapter::getPluginPath();
  StringVector cv = StringVector::create();
  for (auto i : paths) {
    cv.push_back(i);
  }
  return(cv);
}

// [[Rcpp::export]]
DataFrame vampPlugins() {
  PluginLoader *loader = PluginLoader::getInstance();
  std::vector<PluginLoader::PluginKey> plugins = loader->listPlugins();
  typedef std::multimap<std::string, PluginLoader::PluginKey>
    LibraryMap;
  LibraryMap libraryMap;
  
  for (size_t i = 0; i < plugins.size(); ++i) {
    std::string path = loader->getLibraryPathForPlugin(plugins[i]);
    libraryMap.insert(LibraryMap::value_type(path, plugins[i]));
  }
  
  StringVector vp_lib = StringVector::create();
  StringVector vp_name = StringVector::create();
  StringVector vp_id = StringVector::create();
  NumericVector vp_plug_v = NumericVector::create();
  NumericVector vp_vamp_api_v = NumericVector::create();
  StringVector vp_maker = StringVector::create();
  StringVector vp_rights = StringVector::create();
  StringVector vp_desc = StringVector::create();
  StringVector vp_domain = StringVector::create();
  NumericVector vp_dss = NumericVector::create();
  NumericVector vp_dbs = NumericVector::create();
  NumericVector vp_min_c = NumericVector::create();
  NumericVector vp_max_c = NumericVector::create();
  
  for (LibraryMap::iterator i = libraryMap.begin(); i != libraryMap.end(); ++i) {
    PluginLoader::PluginKey key = i->second;
    
    Plugin *plugin = loader->loadPlugin(key, 48000);
    if (plugin) {
      std::string::size_type ki = i->second.find(':');
      vp_lib.push_back(i->second.substr(0, ki));
      
      vp_name.push_back(plugin->getName());
      vp_id.push_back(key);
      vp_plug_v.push_back(plugin->getPluginVersion());
      vp_vamp_api_v.push_back(plugin->getVampApiVersion());
      vp_maker.push_back(plugin->getMaker());
      vp_rights.push_back(plugin->getCopyright());
      vp_desc.push_back(plugin->getDescription());
      vp_domain.push_back((plugin->getInputDomain() == Vamp::Plugin::TimeDomain ? "Time Domain" : "Frequency Domain"));
      vp_dss.push_back(plugin->getPreferredStepSize());
      vp_dbs.push_back(plugin->getPreferredBlockSize());
      vp_min_c.push_back(plugin->getMinChannelCount());
      vp_max_c.push_back(plugin->getMaxChannelCount());
      
      delete plugin;
    }
  }
  DataFrame ret = DataFrame::create(
    Named("library") = vp_lib,
    Named("name") = vp_name,
    Named("id") = vp_id,
    Named("plugin.version") = vp_plug_v,
    Named("vamp.api.version") = vp_vamp_api_v,
    Named("maker") = vp_maker,
    Named("copyright") = vp_rights,
    Named("description") = vp_desc,
    Named("input.domain") = vp_domain,
    Named("default.step.size") = vp_dss,
    Named("default.block.size") = vp_dbs,
    Named("minimum.channels") = vp_min_c,
    Named("maximum.channels") = vp_max_c
  );
  return(ret);
}

// [[Rcpp::export]]
DataFrame vampPluginParams(std::string key) {
  PluginLoader *loader = PluginLoader::getInstance();
  Plugin *plugin = loader->loadPlugin(key, 48000);
  Plugin::ParameterList params = plugin->getParameterDescriptors();
  
  CharacterVector pm_name = CharacterVector::create();
  CharacterVector pm_id = CharacterVector::create();
  CharacterVector pm_desc = CharacterVector::create();
  CharacterVector pm_unit = CharacterVector::create();
  NumericVector pm_range_min = NumericVector::create();
  NumericVector pm_range_max = NumericVector::create();
  NumericVector pm_default = NumericVector::create();
  
  for (size_t j = 0; j < params.size(); ++j) {
    Plugin::ParameterDescriptor &pd(params[j]);
    pm_name.push_back(pd.name);
    pm_id.push_back(pd.identifier);
    pm_desc.push_back(pd.description);
    pm_unit.push_back(pd.unit);
    pm_range_min.push_back(pd.minValue);
    pm_range_max.push_back(pd.maxValue);
    pm_default.push_back(pd.defaultValue);
  }
  DataFrame ret = DataFrame::create(
    Named("name") = pm_name,
    Named("identifier") = pm_id,
    Named("description") = pm_desc,
    Named("unit") = pm_unit,
    Named("range.min") = pm_range_min,
    Named("range.max") = pm_range_max,
    Named("default") = pm_default
  );
  return(ret);
}

// [[Rcpp::export]]
List runPlugin(std::string key, S4 wave, Nullable<List> params = R_NilValue, bool useFrames = false, Nullable<int> blockSize = R_NilValue, Nullable<int> stepSize = R_NilValue, bool verbose = false)
{
  PluginLoader *loader = PluginLoader::getInstance();
  
  // Split key into soname and id
  size_t colonPos = key.find(':');
  if (colonPos == std::string::npos) {
    Rcpp::stop("Invalid plugin key format. Expected 'library:plugin'");
  }
  std::string soname = key.substr(0, colonPos);
  std::string id = key.substr(colonPos + 1);
  
  PluginLoader::PluginKey pluginKey = loader->composePluginKey(soname, id);
  
  // Audio file info (extracted from Wave object, not from file)
  struct {
    int samplerate;
    int64_t frames;
    int channels;
  } sfinfo = {0};
  
  // Get sample rate from Wave object
  sfinfo.samplerate = wave.slot("samp.rate");
  
  // Data structure to collect features for all outputs
  std::map<int, FeatureData> allFeatureData;
  
  // Determine channel count from Wave object
  NumericVector left_channel = wave.slot("left");
  sfinfo.frames = left_channel.length();
  
  // Check if stereo (right channel exists and has data)
  bool is_stereo = false;
  NumericVector right_channel;
  try {
    right_channel = wave.slot("right");
    if (right_channel.length() > 0) {
      is_stereo = true;
    }
  } catch(...) {
    // Mono file - right channel doesn't exist
    is_stereo = false;
  }
  
  sfinfo.channels = is_stereo ? 2 : 1;
  
  // Use unique_ptr for automatic cleanup
  std::unique_ptr<Plugin, std::function<void(Plugin*)>> plugin(
    loader->loadPlugin(pluginKey, sfinfo.samplerate, PluginLoader::ADAPT_ALL_SAFE),
    [](Plugin* p) { delete p; }
  );
  if (!plugin) {
    Rcpp::stop("Failed to load plugin '" + key + "'");
  }
  
  if (verbose) {
    Rcpp::Rcerr << "Running plugin: \"" << plugin->getIdentifier() << "\"..." << std::endl;
  }
  
  // Note that the following would be much simpler if we used a
  // PluginBufferingAdapter as well -- i.e. if we had passed
  // PluginLoader::ADAPT_ALL to loader->loadPlugin() above, instead
  // of ADAPT_ALL_SAFE.  Then we could simply specify our own block
  // size, keep the step size equal to the block size, and ignore
  // the plugin's bleatings.  However, there are some issues with
  // using a PluginBufferingAdapter that make the results sometimes
  // technically different from (if effectively the same as) the
  // un-adapted plugin, so we aren't doing that here.  See the
  // PluginBufferingAdapter documentation for details.
  
  int actualBlockSize;
  int actualStepSize;
  
  // Use user-provided blockSize if given, otherwise use plugin's preferred
  if (blockSize.isNotNull()) {
    actualBlockSize = as<int>(blockSize);
    if (actualBlockSize <= 0) {
      Rcpp::stop("blockSize must be positive");
    }
  } else {
    actualBlockSize = plugin->getPreferredBlockSize();
    if (actualBlockSize == 0) {
      actualBlockSize = 1024;
    }
  }
  
  // Use user-provided stepSize if given, otherwise use plugin's preferred
  if (stepSize.isNotNull()) {
    actualStepSize = as<int>(stepSize);
    if (actualStepSize <= 0) {
      Rcpp::stop("stepSize must be positive");
    }
  } else {
    actualStepSize = plugin->getPreferredStepSize();
    if (actualStepSize == 0) {
      if (plugin->getInputDomain() == Plugin::FrequencyDomain) {
        actualStepSize = actualBlockSize/2;
      } else {
        actualStepSize = actualBlockSize;
      }
    }
  }
  
  if (actualStepSize > actualBlockSize) {
    Rcpp::Rcerr << "WARNING: stepSize " << actualStepSize << " > blockSize " << actualBlockSize << ", resetting blockSize to ";
    if (plugin->getInputDomain() == Plugin::FrequencyDomain) {
      actualBlockSize = actualStepSize * 2;
    } else {
      actualBlockSize = actualStepSize;
    }
    Rcpp::Rcerr << actualBlockSize << std::endl;
  }
  int overlapSize = actualBlockSize - actualStepSize;
  int64_t currentStep = 0;
  int finalStepsRemaining = std::max(1, (actualBlockSize / actualStepSize) - 1); // at end of file, this many part-silent frames needed after we hit EOF
  
  // Use actual channel count from Wave object (PluginChannelAdapter will handle mismatches)
  int channels = sfinfo.channels;
  
  // Use smart pointers for automatic memory management
  std::unique_ptr<float[]> filebuf(new float[actualBlockSize * channels]);
  std::vector<std::unique_ptr<float[]>> plugbuf(channels);
  for (int c = 0; c < channels; ++c) {
    plugbuf[c].reset(new float[actualBlockSize + 2]);
  }
  
  // Pre-allocate raw pointer array for plugin API (reused each iteration)
  std::vector<float*> plugbuf_raw(channels);
  
  if (verbose) {
    Rcpp::Rcerr << "Using block size = " << actualBlockSize << ", step size = "
         << actualStepSize << std::endl;
  }
  
  // The channel queries here are for informational purposes only --
  // a PluginChannelAdapter is being used automatically behind the
  // scenes, and it will take case of any channel mismatch
  
  int minch = plugin->getMinChannelCount();
  int maxch = plugin->getMaxChannelCount();
  if (verbose) {
    Rcpp::Rcerr << "Plugin accepts " << minch << " -> " << maxch << " channel(s)" << std::endl;
    Rcpp::Rcerr << "Sound file has " << channels << " (will mix/augment if necessary)" << std::endl;
  }
  
  Plugin::OutputList outputs = plugin->getOutputDescriptors();
  if (verbose) {
    Rcpp::Rcerr << "Plugin has " << outputs.size() << " output(s)" << std::endl;
  }
  Plugin::FeatureSet features;
  
  int progress = 0;
  
  RealTime rt;
  PluginWrapper *wrapper = 0;
  RealTime adjustment = RealTime::zeroTime;
  
  // Declare these here to avoid goto issues
  NumericVector left;
  int totalSamples = 0;
  int samplesRead = 0;
  
  if (outputs.empty()) {
    Rcpp::Rcerr << "ERROR: Plugin has no outputs!" << std::endl;
    return List::create();
  }
  
  // Set plugin parameters if provided
  if (params.isNotNull()) {
    List paramList(params);
    CharacterVector paramNames = paramList.names();
    
    for (int i = 0; i < paramList.size(); i++) {
      std::string paramId = Rcpp::as<std::string>(paramNames[i]);
      float paramValue = Rcpp::as<float>(paramList[i]);
      
      try {
        plugin->setParameter(paramId, paramValue);
        if (verbose) {
          Rcpp::Rcerr << "Set parameter '" << paramId << "' = " << paramValue << std::endl;
        }
      } catch (std::exception &e) {
        Rcpp::Rcerr << "WARNING: Failed to set parameter '" << paramId << "': " << e.what() << std::endl;
      }
    }
  }
  
  if (!plugin->initialise(channels, actualStepSize, actualBlockSize)) {
    Rcpp::Rcerr << "ERROR: Plugin initialise (channels = " << channels
         << ", stepSize = " << actualStepSize << ", blockSize = "
         << actualBlockSize << ") failed." << std::endl;
    return List::create();
  }
  
  wrapper = dynamic_cast<PluginWrapper *>(plugin.get());
  if (wrapper) {
    // See documentation for
    // PluginInputDomainAdapter::getTimestampAdjustment
    PluginInputDomainAdapter *ida =
      wrapper->getWrapper<PluginInputDomainAdapter>();
    if (ida) adjustment = ida->getTimestampAdjustment();
  }
  
  // Here we iterate over the frames, avoiding asking the numframes in case it's streaming input.
  
  left = wave.slot("left");
  totalSamples = left.length();
  samplesRead = 0;
  
  // Get right channel if stereo
  if (channels == 2) {
    right_channel = wave.slot("right");
  }

  do {
    
    int count=0;

    if ((actualBlockSize==actualStepSize) || (currentStep==0)) {

      // read a full fresh block
      int samplesToRead = std::min(actualBlockSize, totalSamples - samplesRead);
      
      // Put data into filebuf (interleaved for stereo)
      for (int i = 0; i < samplesToRead; i++) {
        filebuf.get()[i * channels] = left[samplesRead + i];
        if (channels == 2) {
          filebuf.get()[i * channels + 1] = right_channel[samplesRead + i];
        }
      }
      // Zero-pad if we don't have enough samples
      for (int i = samplesToRead; i < actualBlockSize; i++) {
        filebuf.get()[i * channels] = 0.0f;
        if (channels == 2) {
          filebuf.get()[i * channels + 1] = 0.0f;
        }
      }
      
      count = samplesToRead;
      samplesRead += count;
      
      if (count != actualBlockSize) --finalStepsRemaining;
    } else {

      // otherwise shunt the existing data down and read the remainder.
      memmove(filebuf.get(), filebuf.get() + (actualStepSize * channels), overlapSize * channels * sizeof(float));
      
      int samplesToRead = std::min(actualStepSize, totalSamples - samplesRead);
      for (int i = 0; i < samplesToRead; i++) {
        filebuf.get()[(overlapSize + i) * channels] = left[samplesRead + i];
        if (channels == 2) {
          filebuf.get()[(overlapSize + i) * channels + 1] = right_channel[samplesRead + i];
        }
      }
      // Zero-pad if we don't have enough samples
      for (int i = samplesToRead; i < actualStepSize; i++) {
        filebuf.get()[(overlapSize + i) * channels] = 0.0f;
        if (channels == 2) {
          filebuf.get()[(overlapSize + i) * channels + 1] = 0.0f;
        }
      }
      
      count = overlapSize + samplesToRead;
      samplesRead += actualStepSize;
      
      if (samplesToRead != actualStepSize) --finalStepsRemaining;
    }

    // De-interleave audio data for plugin
    for (int c = 0; c < channels; ++c) {
      int j = 0;
      while (j < count) {
        plugbuf[c].get()[j] = filebuf.get()[j * channels + c];
        ++j;
      }

      while (j < actualBlockSize) {
        plugbuf[c].get()[j] = 0.0f;
        ++j;
      }
    }

    rt = RealTime::frame2RealTime(currentStep * actualStepSize, sfinfo.samplerate);

    // Update raw pointer array (reuse pre-allocated vector)
    for (int c = 0; c < channels; ++c) {
      plugbuf_raw[c] = plugbuf[c].get();
    }
    features = plugin->process(plugbuf_raw.data(), rt);

    // Collect features for ALL outputs
    collectAllFeatures
      (RealTime::realTime2Frame(rt + adjustment, sfinfo.samplerate),
       sfinfo.samplerate, outputs, features, allFeatureData, useFrames);

    if (verbose && sfinfo.frames > 0){
      int pp = progress;
      progress = static_cast<int>((float(currentStep * actualStepSize) / sfinfo.frames) * 100.f + 0.5f);
      if (progress != pp) {
        Rcpp::Rcerr << "\r" << progress << "%";
      }
    }
    
    ++currentStep;
    
  } while (finalStepsRemaining > 0);
  
  if (verbose) {
    Rcpp::Rcerr << "\rDone" << std::endl;
  }
  
  rt = RealTime::frame2RealTime(currentStep * actualStepSize, sfinfo.samplerate);
  
  features = plugin->getRemainingFeatures();
  
  // Collect remaining features for ALL outputs
  collectAllFeatures(RealTime::realTime2Frame(rt + adjustment, sfinfo.samplerate),
                     sfinfo.samplerate, outputs, features, allFeatureData, useFrames);
  
  // Memory automatically cleaned up by smart pointers
  
  // Create a List to hold DataFrames for each output
  List result;
  
  for (auto &pair : allFeatureData) {
    FeatureData &featureData = pair.second;
    
    DataFrame df;
    
    if (featureData.timestamp.empty()) {
      // No features extracted for this output
      df = DataFrame::create(
        Named("timestamp") = NumericVector::create(),
        Named("duration") = NumericVector::create(),
        Named("label") = CharacterVector::create()
      );
    } else {
      // Build value columns
      List valueColumns;
      std::vector<std::string> colNames(featureData.numValueCols);
      for (int i = 0; i < featureData.numValueCols; i++) {
        NumericVector col(featureData.timestamp.size(), NA_REAL);
        for (size_t j = 0; j < featureData.values.size(); j++) {
          if (i < static_cast<int>(featureData.values[j].size())) {
            col[j] = featureData.values[j][i];
          }
        }
        colNames[i] = (featureData.numValueCols > 1) ? "value" + std::to_string(i + 1) : "value";
        valueColumns[colNames[i]] = col;
      }
      
      // Build the DataFrame
      List columns;
      columns["timestamp"] = wrap(featureData.timestamp);
      columns["duration"] = wrap(featureData.duration);
      
      // Add value columns using pre-built names
      for (int i = 0; i < featureData.numValueCols; i++) {
        columns[colNames[i]] = valueColumns[colNames[i]];
      }
      
      columns["label"] = wrap(featureData.label);
      
      df = DataFrame(columns);
    }
    
    // Use output identifier as name
    result[featureData.outputIdentifier] = df;
  }
  
  return result;
}

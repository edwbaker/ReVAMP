
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
#include "SimpleWavReader.h"

using namespace Rcpp;

using Vamp::Plugin;
using Vamp::PluginHostAdapter;
using Vamp::RealTime;
using Vamp::HostExt::PluginLoader;
using Vamp::HostExt::PluginWrapper;
using Vamp::HostExt::PluginInputDomainAdapter;

double toSeconds(const RealTime &time)
{
  return time.sec + double(time.nsec + 1) / 1000000000.0;
}


// Struct to collect features in memory for single output
struct FeatureData {
  std::vector<double> timestamp;
  std::vector<double> duration;
  std::vector<std::string> label;
  std::vector<std::vector<float>> values;
  std::vector<int> segment;  // Segment index (1-based), only used when segmentLength is set
  int numValueCols;
  std::string outputIdentifier;
  
  FeatureData() : numValueCols(0) {}
};

// Collect features in memory for ALL outputs
void collectAllFeatures(int frame, int sr,
                        const Plugin::OutputList &outputs,
                        const Plugin::FeatureSet &features,
                        std::map<int, FeatureData> &allData,
                        bool useFrames,
                        std::map<int, RealTime> &lastFeatureTime,
                        int currentSegment,
                        double segmentStartTime)
{
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
      
      // Store timestamp (absolute position in original file)
      // When segmenting, add segmentStartTime to get absolute file position
      if (useFrames) {
        int frameVal = RealTime::realTime2Frame(featureTime, sr);
        if (currentSegment > 0) {
          // Add segment start frame to get absolute position
          int segmentStartFrame = static_cast<int>(segmentStartTime * sr);
          frameVal += segmentStartFrame;
        }
        data.timestamp.push_back(frameVal);
      } else {
        double timeVal = toSeconds(featureTime);
        if (currentSegment > 0) {
          timeVal += segmentStartTime;
        }
        data.timestamp.push_back(timeVal);
      }
      
      // Store segment index (only if segmentation is active)
      if (currentSegment > 0) {
        data.segment.push_back(currentSegment);
      }
      
      // Store duration
      if (fli->hasDuration) {
        data.duration.push_back(toSeconds(fli->duration));
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
void vampResetCache() {
  PluginLoader::resetInstance();
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
List runPlugin(std::string key, RObject wave, Nullable<List> params = R_NilValue, bool useFrames = false, Nullable<int> blockSize = R_NilValue, Nullable<int> stepSize = R_NilValue, bool verbose = false, bool dropIncompleteFinalFrame = true, Nullable<double> segmentLength = R_NilValue)
{
  PluginLoader *loader = PluginLoader::getInstance();
  
  // Segmentation setup
  bool doSegmentation = segmentLength.isNotNull();
  double segmentLengthSec = doSegmentation ? as<double>(segmentLength) : 0.0;
  if (doSegmentation && segmentLengthSec <= 0) {
    Rcpp::stop("segmentLength must be positive");
  }
  
  // Split key into soname and id
  size_t colonPos = key.find(':');
  if (colonPos == std::string::npos) {
    Rcpp::stop("Invalid plugin key format. Expected 'library:plugin'");
  }
  std::string soname = key.substr(0, colonPos);
  std::string id = key.substr(colonPos + 1);
  
  PluginLoader::PluginKey pluginKey = loader->composePluginKey(soname, id);
  
  // Audio file info
  struct {
    int samplerate;
    int64_t frames;
    int channels;
  } sfinfo = {0};
  
  bool useFile = false;
  std::vector<float> fileData;
  NumericVector left_channel;
  NumericVector right_channel;
  double scale_factor = 1.0;

  if (wave.isS4()) {
      S4 waveObj(wave);
      sfinfo.samplerate = waveObj.slot("samp.rate");
      
      left_channel = waveObj.slot("left");
      sfinfo.frames = left_channel.length();
      
      // Check if stereo (right channel exists and has data)
      bool is_stereo = false;
      try {
        right_channel = waveObj.slot("right");
        if (right_channel.length() > 0) {
          is_stereo = true;
        }
      } catch(...) {
        // Mono file - right channel doesn't exist
        is_stereo = false;
      }
      sfinfo.channels = is_stereo ? 2 : 1;

      // Check for PCM and bit depth to normalize
      bool pcm = false;
      try {
          pcm = as<bool>(waveObj.slot("pcm"));
      } catch(...) {
          pcm = false; // Default to false if slot missing
      }

      if (pcm) {
          int bit = 16;
          try {
              bit = as<int>(waveObj.slot("bit"));
          } catch(...) {
              bit = 16; // Default to 16 if slot missing
          }
          
          if (bit == 8) scale_factor = 1.0 / 128.0;
          else if (bit == 16) scale_factor = 1.0 / 32768.0;
          else if (bit == 24) scale_factor = 1.0 / 8388608.0;
          else if (bit == 32) scale_factor = 1.0 / 2147483648.0;
      }
  } else if (is<CharacterVector>(wave)) {
      std::string filename = as<std::string>(wave);
      SimpleWavReader::Header header;
      if (!SimpleWavReader::read(filename, fileData, header)) {
          Rcpp::stop("Failed to read WAV file: " + filename);
      }
      sfinfo.samplerate = header.sampleRate;
      sfinfo.channels = header.channels;
      sfinfo.frames = fileData.size() / header.channels;
      useFile = true;
  } else {
      Rcpp::stop("wave argument must be an S4 Wave object or a filename string");
  }
  
  // Data structure to collect features for all outputs
  std::map<int, FeatureData> allFeatureData;
  
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
  int finalStepsRemaining = dropIncompleteFinalFrame ? 0 : std::max(1, (actualBlockSize / actualStepSize) - 1);
  
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
  // scenes, and it will take care of any channel mismatch
  
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
    PluginInputDomainAdapter *ida =
      wrapper->getWrapper<PluginInputDomainAdapter>();
    if (ida) adjustment = ida->getTimestampAdjustment();
  }
  
  totalSamples = (int)sfinfo.frames;
  samplesRead = 0;
  
  // Track time for FixedSampleRate outputs with implicit timestamps
  std::map<int, RealTime> lastFeatureTime;

  // Segmentation tracking
  int segmentLengthSamples = doSegmentation ? static_cast<int>(segmentLengthSec * sfinfo.samplerate) : 0;
  int currentSegment = doSegmentation ? 1 : 0;  // 0 means no segmentation
  double segmentStartTime = 0.0;
  int nextSegmentBoundary = segmentLengthSamples;

  do {
    
    int count=0;

    if ((actualBlockSize==actualStepSize) || (currentStep==0)) {

      // read a full fresh block
      int samplesToRead = std::min(actualBlockSize, totalSamples - samplesRead);
      
      // Put data into filebuf (interleaved for stereo)
      for (int i = 0; i < samplesToRead; i++) {
        if (useFile) {
             for (int c = 0; c < channels; ++c) {
                 filebuf.get()[i * channels + c] = fileData[(samplesRead + i) * channels + c];
             }
        } else {
            filebuf.get()[i * channels] = left_channel[samplesRead + i] * scale_factor;
            if (channels == 2) {
              filebuf.get()[i * channels + 1] = right_channel[samplesRead + i] * scale_factor;
            }
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
        if (useFile) {
             for (int c = 0; c < channels; ++c) {
                 filebuf.get()[(overlapSize + i) * channels + c] = fileData[(samplesRead + i) * channels + c];
             }
        } else {
            filebuf.get()[(overlapSize + i) * channels] = left_channel[samplesRead + i] * scale_factor;
            if (channels == 2) {
              filebuf.get()[(overlapSize + i) * channels + 1] = right_channel[samplesRead + i] * scale_factor;
            }
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

    collectAllFeatures
      (RealTime::realTime2Frame(rt + adjustment, sfinfo.samplerate),
       sfinfo.samplerate, outputs, features, allFeatureData, useFrames, lastFeatureTime,
       currentSegment, segmentStartTime);

    // Check for segment boundary crossing
    if (doSegmentation && samplesRead >= nextSegmentBoundary && samplesRead < totalSamples) {
      // Get remaining features from plugin before reset
      Plugin::FeatureSet remainingFeatures = plugin->getRemainingFeatures();
      collectAllFeatures(RealTime::realTime2Frame(rt + adjustment, sfinfo.samplerate),
                         sfinfo.samplerate, outputs, remainingFeatures, allFeatureData, useFrames, lastFeatureTime,
                         currentSegment, segmentStartTime);
      
      // Move to next segment
      currentSegment++;
      segmentStartTime = static_cast<double>(nextSegmentBoundary) / sfinfo.samplerate;
      nextSegmentBoundary += segmentLengthSamples;
      
      // Reset plugin for new segment
      plugin->reset();
      lastFeatureTime.clear();
      
      if (verbose) {
        Rcpp::Rcerr << "\nStarting segment " << currentSegment << " at " << segmentStartTime << "s" << std::endl;
      }
    }

    if (verbose && sfinfo.frames > 0){
      int pp = progress;
      progress = static_cast<int>((float(currentStep * actualStepSize) / sfinfo.frames) * 100.f + 0.5f);
      if (progress != pp) {
        Rcpp::Rcerr << "\r" << progress << "%";
      }
    }
    
    ++currentStep;
    
  } while (samplesRead < totalSamples || finalStepsRemaining > 0);
  
  if (verbose) {
    Rcpp::Rcerr << "\rDone" << std::endl;
  }
  
  rt = RealTime::frame2RealTime(currentStep * actualStepSize, sfinfo.samplerate);
  
  features = plugin->getRemainingFeatures();
  
  // Collect remaining features for ALL outputs
  collectAllFeatures(RealTime::realTime2Frame(rt + adjustment, sfinfo.samplerate),
                     sfinfo.samplerate, outputs, features, allFeatureData, useFrames, lastFeatureTime,
                     currentSegment, segmentStartTime);
  
  
  // Create a List to hold DataFrames for each output
  List result;
  
  for (auto &pair : allFeatureData) {
    FeatureData &featureData = pair.second;
    
    DataFrame df;
    
    if (featureData.timestamp.empty()) {
      // No features extracted for this output
      if (doSegmentation) {
        df = DataFrame::create(
          Named("segment") = IntegerVector::create(),
          Named("timestamp") = NumericVector::create(),
          Named("duration") = NumericVector::create(),
          Named("label") = CharacterVector::create()
        );
      } else {
        df = DataFrame::create(
          Named("timestamp") = NumericVector::create(),
          Named("duration") = NumericVector::create(),
          Named("label") = CharacterVector::create()
        );
      }
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
      
      // Add segment column first if segmentation is active
      if (doSegmentation) {
        columns["segment"] = wrap(featureData.segment);
      }
      
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

// [[Rcpp::export]]
List runPlugins(CharacterVector keys, RObject wave, Nullable<List> params = R_NilValue, bool useFrames = false, Nullable<int> blockSize = R_NilValue, Nullable<int> stepSize = R_NilValue, bool verbose = false, bool dropIncompleteFinalFrame = true, Nullable<double> segmentLength = R_NilValue)
{
  PluginLoader *loader = PluginLoader::getInstance();

  // Segmentation setup
  bool doSegmentation = segmentLength.isNotNull();
  double segmentLengthSec = doSegmentation ? as<double>(segmentLength) : 0.0;
  if (doSegmentation && segmentLengthSec <= 0) {
    Rcpp::stop("segmentLength must be positive");
  }

  // Convert keys to std::vector<string>
  std::vector<std::string> keylist;
  for (int i = 0; i < keys.size(); ++i) keylist.push_back(Rcpp::as<std::string>(keys[i]));

  if (keylist.empty()) {
    Rcpp::stop("No plugin keys provided");
  }

  // Audio file info (same as runPlugin)
  struct {
    int samplerate;
    int64_t frames;
    int channels;
  } sfinfo = {0};

  bool useFile = false;
  std::vector<float> fileData;
  NumericVector left_channel;
  NumericVector right_channel;
  double scale_factor = 1.0;

  if (wave.isS4()) {
      S4 waveObj(wave);
      sfinfo.samplerate = waveObj.slot("samp.rate");

      left_channel = waveObj.slot("left");
      sfinfo.frames = left_channel.length();

      bool is_stereo = false;
      try {
        right_channel = waveObj.slot("right");
        if (right_channel.length() > 0) {
          is_stereo = true;
        }
      } catch(...) {
        is_stereo = false;
      }
      sfinfo.channels = is_stereo ? 2 : 1;

      bool pcm = false;
      try {
          pcm = as<bool>(waveObj.slot("pcm"));
      } catch(...) {
          pcm = false;
      }

      if (pcm) {
          int bit = 16;
          try {
              bit = as<int>(waveObj.slot("bit"));
          } catch(...) {
              bit = 16;
          }
          if (bit == 8) scale_factor = 1.0 / 128.0;
          else if (bit == 16) scale_factor = 1.0 / 32768.0;
          else if (bit == 24) scale_factor = 1.0 / 8388608.0;
          else if (bit == 32) scale_factor = 1.0 / 2147483648.0;
      }
  } else if (is<CharacterVector>(wave)) {
      std::string filename = as<std::string>(wave);
      SimpleWavReader::Header header;
      if (!SimpleWavReader::read(filename, fileData, header)) {
          Rcpp::stop("Failed to read WAV file: " + filename);
      }
      sfinfo.samplerate = header.sampleRate;
      sfinfo.channels = header.channels;
      sfinfo.frames = fileData.size() / header.channels;
      useFile = true;
  } else {
      Rcpp::stop("wave argument must be an S4 Wave object or a filename string");
  }

  int nplugins = (int)keylist.size();

  // Load plugins and determine per-plugin preferred sizes
  struct PluginState {
    std::unique_ptr<Plugin, std::function<void(Plugin*)>> plugin;
    Plugin::OutputList outputs;
    std::map<int, FeatureData> allFeatureData;
    std::map<int, RealTime> lastFeatureTime;
    std::string key;
    RealTime adjustment;
  };

  std::vector<std::unique_ptr<PluginState>> states;
  states.reserve(nplugins);

  std::vector<int> resolvedBlockSizes(nplugins);
  std::vector<int> resolvedStepSizes(nplugins);

  // Interpret params argument: can be list-of-lists or single list
  std::vector<List> paramsForPlugin(nplugins);
  if (params.isNotNull()) {
    List p(params);
    if (p.size() == 1) {
      for (int i=0;i<nplugins;++i) paramsForPlugin[i] = p[0];
    } else if (p.size() == nplugins) {
      for (int i=0;i<nplugins;++i) paramsForPlugin[i] = p[i];
    } else {
      Rcpp::RObject namesObj = p.names();
      if (!namesObj.isNULL()) {
        CharacterVector names = Rcpp::as<CharacterVector>(namesObj);
        for (int i=0;i<nplugins;++i) {
          std::string k = keylist[i];
          bool found=false;
          for (int j=0;j<p.size();++j) {
            if (Rcpp::as<std::string>(names[j]) == k) {
              paramsForPlugin[i] = p[j];
              found=true; break;
            }
          }
          if (!found) paramsForPlugin[i] = List::create();
        }
      } else {
        for (int i=0;i<nplugins;++i) paramsForPlugin[i] = List::create();
      }
    }
  } else {
    for (int i=0;i<nplugins;++i) paramsForPlugin[i] = List::create();
  }

  for (int i = 0; i < nplugins; ++i) {
    std::string key = keylist[i];
    size_t colonPos = key.find(':');
    if (colonPos == std::string::npos) {
      Rcpp::stop("Invalid plugin key format. Expected 'library:plugin'");
    }
    std::string soname = key.substr(0, colonPos);
    std::string id = key.substr(colonPos + 1);
    PluginLoader::PluginKey pluginKey = loader->composePluginKey(soname, id);

    std::unique_ptr<Plugin, std::function<void(Plugin*)>> plugin(
      loader->loadPlugin(pluginKey, sfinfo.samplerate, PluginLoader::ADAPT_ALL_SAFE),
      [](Plugin* p){ delete p; }
    );
    if (!plugin) {
      Rcpp::stop("Failed to load plugin '" + key + "'");
    }

    int actualBlockSize;
    int actualStepSize;

    if (blockSize.isNotNull()) {
      actualBlockSize = as<int>(blockSize);
      if (actualBlockSize <= 0) Rcpp::stop("blockSize must be positive");
    } else {
      actualBlockSize = plugin->getPreferredBlockSize();
      if (actualBlockSize == 0) actualBlockSize = 1024;
    }
    if (stepSize.isNotNull()) {
      actualStepSize = as<int>(stepSize);
      if (actualStepSize <= 0) Rcpp::stop("stepSize must be positive");
    } else {
      actualStepSize = plugin->getPreferredStepSize();
      if (actualStepSize == 0) {
        if (plugin->getInputDomain() == Plugin::FrequencyDomain) actualStepSize = actualBlockSize/2;
        else actualStepSize = actualBlockSize;
      }
    }

    if (actualStepSize > actualBlockSize) {
      if (plugin->getInputDomain() == Plugin::FrequencyDomain) actualBlockSize = actualStepSize * 2;
      else actualBlockSize = actualStepSize;
    }

    resolvedBlockSizes[i] = actualBlockSize;
    resolvedStepSizes[i] = actualStepSize;

    auto st = std::unique_ptr<PluginState>(new PluginState());
    st->plugin = std::move(plugin);
    st->outputs = st->plugin->getOutputDescriptors();
    st->key = key;
    states.push_back(std::move(st));
  }

  // Ensure all block/step sizes are equal
  for (int i=1;i<nplugins;++i) {
    if (resolvedBlockSizes[i] != resolvedBlockSizes[0] || resolvedStepSizes[i] != resolvedStepSizes[0]) {
      Rcpp::stop("All plugins must have the same blockSize and stepSize (or provide explicit blockSize/stepSize that matches for all plugins)");
    }
  }

  int actualBlockSize = resolvedBlockSizes[0];
  int actualStepSize = resolvedStepSizes[0];
  int overlapSize = actualBlockSize - actualStepSize;
  int64_t currentStep = 0;
  int finalStepsRemaining = dropIncompleteFinalFrame ? 0 : std::max(1, (actualBlockSize / actualStepSize) - 1);

  int channels = sfinfo.channels;

  std::unique_ptr<float[]> filebuf(new float[actualBlockSize * channels]);
  std::vector<std::unique_ptr<float[]>> plugbuf(channels);
  for (int c = 0; c < channels; ++c) {
    plugbuf[c].reset(new float[actualBlockSize + 2]);
  }
  std::vector<float*> plugbuf_raw(channels);

  if (verbose) {
    Rcpp::Rcerr << "Using block size = " << actualBlockSize << ", step size = " << actualStepSize << std::endl;
  }

  // initialise each plugin, set params
  for (int i=0;i<nplugins;++i) {
    Plugin *p = states[i]->plugin.get();
    if (paramsForPlugin[i].size() > 0) {
      List paramList(paramsForPlugin[i]);
      CharacterVector paramNames = paramList.names();
      for (int j=0;j<paramList.size();++j) {
        std::string paramId = Rcpp::as<std::string>(paramNames[j]);
        float paramValue = Rcpp::as<float>(paramList[j]);
        try { p->setParameter(paramId, paramValue); } catch(...) { }
      }
    }
    if (!p->initialise(channels, actualStepSize, actualBlockSize)) {
      Rcpp::stop("Plugin initialise failed for '" + states[i]->key + "' (channels = " + std::to_string(channels) + ", stepSize = " + std::to_string(actualStepSize) + ", blockSize = " + std::to_string(actualBlockSize) + ")");
    }
    // store per-plugin timestamp adjustment if available (as single-plugin path does)
    PluginWrapper *wrapper = dynamic_cast<PluginWrapper *>(p);
    states[i]->adjustment = RealTime::zeroTime;
    if (wrapper) {
      PluginInputDomainAdapter *ida = wrapper->getWrapper<PluginInputDomainAdapter>();
      if (ida) states[i]->adjustment = ida->getTimestampAdjustment();
    }
  }

  int totalSamples = (int)sfinfo.frames;
  int samplesRead = 0;

  // Track progress
  int progress = 0;

  RealTime rt;

  // Segmentation tracking
  int segmentLengthSamples = doSegmentation ? static_cast<int>(segmentLengthSec * sfinfo.samplerate) : 0;
  int currentSegment = doSegmentation ? 1 : 0;  // 0 means no segmentation
  double segmentStartTime = 0.0;
  int nextSegmentBoundary = segmentLengthSamples;

  // Main processing loop: read audio once, pass to every plugin
  do {
    int count = 0;
    if ((actualBlockSize==actualStepSize) || (currentStep==0)) {
      int samplesToRead = std::min(actualBlockSize, totalSamples - samplesRead);
      if (useFile) {
        if (samplesToRead > 0) {
          std::memcpy(filebuf.get(), &fileData[samplesRead * channels], samplesToRead * channels * sizeof(float));
        }
      } else {
        double *lptr = &left_channel[0];
        double *rptr = (channels == 2) ? &right_channel[0] : nullptr;
        for (int i = 0; i < samplesToRead; ++i) {
          filebuf.get()[i * channels] = static_cast<float>(lptr[samplesRead + i] * scale_factor);
          if (channels == 2) filebuf.get()[i * channels + 1] = static_cast<float>(rptr[samplesRead + i] * scale_factor);
        }
      }
      for (int i = samplesToRead; i < actualBlockSize; i++) {
        filebuf.get()[i * channels] = 0.0f;
        if (channels == 2) filebuf.get()[i * channels + 1] = 0.0f;
      }
      count = samplesToRead;
      samplesRead += count;
      if (count != actualBlockSize) --finalStepsRemaining;
    } else {
      memmove(filebuf.get(), filebuf.get() + (actualStepSize * channels), overlapSize * channels * sizeof(float));
      int samplesToRead = std::min(actualStepSize, totalSamples - samplesRead);
      if (useFile) {
        if (samplesToRead > 0) {
          std::memcpy(filebuf.get() + overlapSize * channels, &fileData[samplesRead * channels], samplesToRead * channels * sizeof(float));
        }
      } else {
        double *lptr = &left_channel[0];
        double *rptr = (channels == 2) ? &right_channel[0] : nullptr;
        for (int i = 0; i < samplesToRead; ++i) {
          filebuf.get()[(overlapSize + i) * channels] = static_cast<float>(lptr[samplesRead + i] * scale_factor);
          if (channels == 2) filebuf.get()[(overlapSize + i) * channels + 1] = static_cast<float>(rptr[samplesRead + i] * scale_factor);
        }
      }
      for (int i = samplesToRead; i < actualStepSize; i++) {
        filebuf.get()[(overlapSize + i) * channels] = 0.0f;
        if (channels == 2) filebuf.get()[(overlapSize + i) * channels + 1] = 0.0f;
      }
      count = overlapSize + samplesToRead;
      samplesRead += actualStepSize;
      if (samplesToRead != actualStepSize) --finalStepsRemaining;
    }

    // De-interleave audio data for plugin (pointer-stepping copy)
    for (int c = 0; c < channels; ++c) {
      float *dest = plugbuf[c].get();
      float *src = filebuf.get() + c;
      int j = 0;
      for (; j < count; ++j) {
        dest[j] = *src;
        src += channels;
      }
      for (; j < actualBlockSize; ++j) dest[j] = 0.0f;
    }

    rt = RealTime::frame2RealTime(currentStep * actualStepSize, sfinfo.samplerate);

    for (int c = 0; c < channels; ++c) plugbuf_raw[c] = plugbuf[c].get();

    // Run each plugin on the same block; compute adjusted frame per-plugin
    for (int pi=0; pi<nplugins; ++pi) {
      Plugin *p = states[pi]->plugin.get();
      Plugin::FeatureSet features = p->process(plugbuf_raw.data(), rt);
      RealTime adj_rt = rt + states[pi]->adjustment;
      int frame_for_features = RealTime::realTime2Frame(adj_rt, sfinfo.samplerate);
      collectAllFeatures(frame_for_features, sfinfo.samplerate, states[pi]->outputs, features, states[pi]->allFeatureData, useFrames, states[pi]->lastFeatureTime, currentSegment, segmentStartTime);
    }

    // Check for segment boundary crossing
    if (doSegmentation && samplesRead >= nextSegmentBoundary && samplesRead < totalSamples) {
      // Get remaining features from all plugins before reset
      for (int pi=0; pi<nplugins; ++pi) {
        Plugin *p = states[pi]->plugin.get();
        Plugin::FeatureSet remainingFeatures = p->getRemainingFeatures();
        RealTime adj_rt = rt + states[pi]->adjustment;
        int frame_for_features = RealTime::realTime2Frame(adj_rt, sfinfo.samplerate);
        collectAllFeatures(frame_for_features, sfinfo.samplerate, states[pi]->outputs, remainingFeatures, states[pi]->allFeatureData, useFrames, states[pi]->lastFeatureTime, currentSegment, segmentStartTime);
      }
      
      // Move to next segment
      currentSegment++;
      segmentStartTime = static_cast<double>(nextSegmentBoundary) / sfinfo.samplerate;
      nextSegmentBoundary += segmentLengthSamples;
      
      // Reset all plugins for new segment
      for (int pi=0; pi<nplugins; ++pi) {
        states[pi]->plugin->reset();
        states[pi]->lastFeatureTime.clear();
      }
      
      if (verbose) {
        Rcpp::Rcerr << "\nStarting segment " << currentSegment << " at " << segmentStartTime << "s" << std::endl;
      }
    }

    if (verbose && sfinfo.frames > 0) {
      int pp = progress;
      progress = static_cast<int>((float(currentStep * actualStepSize) / sfinfo.frames) * 100.f + 0.5f);
      if (progress != pp) {
        Rcpp::Rcerr << "\r" << progress << "%";
      }
    }

    ++currentStep;
  } while (samplesRead < totalSamples || finalStepsRemaining > 0);

  if (verbose) Rcpp::Rcerr << "\rDone" << std::endl;

  rt = RealTime::frame2RealTime(currentStep * actualStepSize, sfinfo.samplerate);

  // Get remaining features for each plugin
  for (int pi=0; pi<nplugins; ++pi) {
    Plugin *p = states[pi]->plugin.get();
    Plugin::FeatureSet features = p->getRemainingFeatures();
    collectAllFeatures(RealTime::realTime2Frame(rt, sfinfo.samplerate), sfinfo.samplerate, states[pi]->outputs, features, states[pi]->allFeatureData, useFrames, states[pi]->lastFeatureTime, currentSegment, segmentStartTime);
  }

  // Build result: a list per plugin, each a named list of outputs
  List result;
  for (int pi=0; pi<nplugins; ++pi) {
    List pluginResult;
    for (auto &pair : states[pi]->allFeatureData) {
      FeatureData &featureData = pair.second;
      DataFrame df;
      if (featureData.timestamp.empty()) {
        if (doSegmentation) {
          df = DataFrame::create(
            Named("segment") = IntegerVector::create(),
            Named("timestamp") = NumericVector::create(),
            Named("duration") = NumericVector::create(),
            Named("label") = CharacterVector::create()
          );
        } else {
          df = DataFrame::create(
            Named("timestamp") = NumericVector::create(),
            Named("duration") = NumericVector::create(),
            Named("label") = CharacterVector::create()
          );
        }
      } else {
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
        List columns;
        
        // Add segment column first if segmentation is active
        if (doSegmentation) {
          columns["segment"] = wrap(featureData.segment);
        }
        
        columns["timestamp"] = wrap(featureData.timestamp);
        columns["duration"] = wrap(featureData.duration);
        for (int i = 0; i < featureData.numValueCols; i++) {
          columns[colNames[i]] = valueColumns[colNames[i]];
        }
        columns["label"] = wrap(featureData.label);
        df = DataFrame(columns);
      }
      pluginResult[featureData.outputIdentifier] = df;
    }
    // name the plugin entry by its key
    result[states[pi]->key] = pluginResult;
  }

  return result;
}

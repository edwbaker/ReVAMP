
#include <Rcpp.h>

#include <cstring>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <fstream>
#include <set>
#include <sndfile.h>

#include <vamp-hostsdk/RealTime.h>
#include <vamp-hostsdk/PluginHostAdapter.h>
#include <vamp-hostsdk/PluginInputDomainAdapter.h>
#include <vamp-hostsdk/PluginLoader.h>
#include "system.h"

using namespace Rcpp;
using namespace std;


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
// Structure to collect features in memory
struct FeatureData {
  std::vector<double> timestamp;
  std::vector<double> duration;
  std::vector<std::string> label;
  std::vector<std::vector<float>> values;
  int numValueCols;
  
  FeatureData() : numValueCols(0) {}
};

void printFeatures(int frame, int sr,
                const Plugin::OutputDescriptor &output, int outputNo,
                const Plugin::FeatureSet &features, std::ofstream *out, bool useFrames)
{
  if (!out) return;
  
  // Track time for FixedSampleRate outputs with implicit timestamps
  static std::map<int, RealTime> lastFeatureTime;
  
  for (Plugin::FeatureSet::const_iterator fi = features.begin(); fi != features.end(); ++fi) {
    if (fi->first != outputNo) continue;
    
    for (Plugin::FeatureList::const_iterator fli = fi->second.begin(); fli != fi->second.end(); ++fli) {
      
      RealTime featureTime;
      
      // Handle timestamp according to output sample type
      if (output.sampleType == Plugin::OutputDescriptor::OneSamplePerStep) {
        // OneSamplePerStep: Always use the frame from process() call
        // Never read feature timestamp, even if erroneously set
        featureTime = RealTime::frame2RealTime(frame, sr);
        
      } else if (output.sampleType == Plugin::OutputDescriptor::FixedSampleRate) {
        // FixedSampleRate: Check hasTimestamp
        if (fli->hasTimestamp) {
          // Explicit timestamp provided
          featureTime = fli->timestamp;
          lastFeatureTime[outputNo] = featureTime;
        } else {
          // Implicit timestamp: increment from previous feature
          if (lastFeatureTime.find(outputNo) != lastFeatureTime.end()) {
            // Calculate time increment from sample rate
            int increment_ns = (int)((1000000000.0 / output.sampleRate) + 0.5);
            featureTime = lastFeatureTime[outputNo] + RealTime(0, increment_ns);
          } else {
            // First feature with no explicit timestamp
            featureTime = RealTime::frame2RealTime(frame, sr);
          }
          lastFeatureTime[outputNo] = featureTime;
        }
        
      } else { // VariableSampleRate
        // VariableSampleRate: Must always have explicit timestamp
        if (fli->hasTimestamp) {
          featureTime = fli->timestamp;
        } else {
          // Should not happen, but fall back to frame time
          featureTime = RealTime::frame2RealTime(frame, sr);
        }
      }
      
      // Output timestamp
      if (useFrames) {
        *out << RealTime::realTime2Frame(featureTime, sr);
      } else {
        *out << toSeconds(const_cast<RealTime&>(featureTime));
      }
      
      // Duration
      if (fli->hasDuration) {
        *out << "," << toSeconds(const_cast<RealTime&>(fli->duration));
      }
      
      // Values
      for (size_t j = 0; j < fli->values.size(); ++j) {
        *out << "," << fli->values[j];
      }
      
      // Label
      if (fli->label != "") {
        *out << "," << fli->label;
      }
      
      *out << endl;
    }
  }
}

// Collect features in memory for return to R
void collectFeatures(int frame, int sr,
                     const Plugin::OutputDescriptor &output, int outputNo,
                     const Plugin::FeatureSet &features, FeatureData &data, bool useFrames)
{
  // Track time for FixedSampleRate outputs with implicit timestamps
  static std::map<int, RealTime> lastFeatureTime;
  
  for (Plugin::FeatureSet::const_iterator fi = features.begin(); fi != features.end(); ++fi) {
    if (fi->first != outputNo) continue;
    
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
            int increment_ns = (int)((1000000000.0 / output.sampleRate) + 0.5);
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
      if ((int)fli->values.size() > data.numValueCols) {
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
  vector<string> paths = PluginHostAdapter::getPluginPath();
  StringVector cv = StringVector::create();
  for (auto i : paths) {
    cv.push_back(i);
  }
  return(cv);
}

// [[Rcpp::export]]
DataFrame vampPlugins() {
  PluginLoader *loader = PluginLoader::getInstance();
  vector<PluginLoader::PluginKey> plugins = loader->listPlugins();
  typedef multimap<string, PluginLoader::PluginKey>
    LibraryMap;
  LibraryMap libraryMap;
  
  for (size_t i = 0; i < plugins.size(); ++i) {
    string path = loader->getLibraryPathForPlugin(plugins[i]);
    libraryMap.insert(LibraryMap::value_type(path, plugins[i]));
  }
  
  string prevPath = "";
  int index = 0;
  
  StringVector vp_lib = StringVector::create();
  StringVector vp_name = StringVector::create();
  StringVector vp_id = StringVector::create();
  NumericVector vp_plug_v = NumericVector::create();
  NumericVector vp_vamp_api_v = NumericVector::create();
  StringVector vp_maker = StringVector::create();
  StringVector vp_rights = StringVector::create();
  StringVector vp_desc = StringVector::create();
  StringVector vp_default_bin = StringVector::create();
  StringVector vp_domain = StringVector::create();
  NumericVector vp_dss = NumericVector::create();
  NumericVector vp_dbs = NumericVector::create();
  NumericVector vp_min_c = NumericVector::create();
  NumericVector vp_max_c = NumericVector::create();
  
  for (LibraryMap::iterator i = libraryMap.begin(); i != libraryMap.end(); ++i) {
    string path = i->first;
    PluginLoader::PluginKey key = i->second;
    
    if (path != prevPath) {
      prevPath = path;
      index = 0;
    }
    
    Plugin *plugin = loader->loadPlugin(key, 48000);
    if (plugin) {
      string::size_type ki = i->second.find(':');
      vp_lib.push_back(i->second.substr(0, ki));
      char c = char('A' + index);
      if (c > 'Z') c = char('a' + (index - 26));
      
      PluginLoader::PluginCategoryHierarchy category =
        loader->getPluginCategory(key);
      string catstr;
      if (!category.empty()) {
        for (size_t ci = 0; ci < category.size(); ++ci) {
          if (ci > 0) catstr += " > ";
          catstr += category[ci];
        }
      }
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
      
      ++index;
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
DataFrame vampParams(std::string key) {
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
    if (pd.isQuantized) {
      Rcpp::Rcout << " - Quantize Step:      "
           << pd.quantizeStep << endl;
    }
    if (!pd.valueNames.empty()) {
      Rcpp::Rcout << " - Value Names:        ";
      for (size_t k = 0; k < pd.valueNames.size(); ++k) {
        if (k > 0) Rcpp::Rcout << ", ";
        Rcpp::Rcout << "\"" << pd.valueNames[k] << "\"";
      }
      Rcpp::Rcout << endl;
    }
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
DataFrame runPlugin(std::string myname, std::string soname, std::string id,
              std::string output, int outputNo, S4 wave,
              std::string outfilename, bool useFrames)
{
  PluginLoader *loader = PluginLoader::getInstance();
  
  PluginLoader::PluginKey key = loader->composePluginKey(soname, id);
  
  SNDFILE *sndfile;
  SF_INFO sfinfo;
  memset(&sfinfo, 0, sizeof(SF_INFO));
  
  // Get sample rate from Wave object
  sfinfo.samplerate = wave.slot("samp.rate");
  
  // Data structure to collect features
  FeatureData featureData;
  
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
  
  ofstream *out = 0;
  if (outfilename != "") {
    out = new ofstream(outfilename.c_str(), ios::out);
    if (!*out) {
      delete out;
      Rcpp::stop("Failed to open output file '" + outfilename + "' for writing");
    }
  }
  
  Plugin *plugin = loader->loadPlugin
    (key, sfinfo.samplerate, PluginLoader::ADAPT_ALL_SAFE);
  if (!plugin) {
    if (out) {
      out->close();
      delete out;
    }
    Rcpp::stop("Failed to load plugin '" + id + "' from library '" + soname + "'");
  }
  
  Rcpp::Rcerr << "Running plugin: \"" << plugin->getIdentifier() << "\"..." << endl;
  
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
  
  int blockSize = plugin->getPreferredBlockSize();
  int stepSize = plugin->getPreferredStepSize();
  
  if (blockSize == 0) {
    blockSize = 1024;
  }
  if (stepSize == 0) {
    if (plugin->getInputDomain() == Plugin::FrequencyDomain) {
      stepSize = blockSize/2;
    } else {
      stepSize = blockSize;
    }
  } else if (stepSize > blockSize) {
    Rcpp::Rcerr << "WARNING: stepSize " << stepSize << " > blockSize " << blockSize << ", resetting blockSize to ";
    if (plugin->getInputDomain() == Plugin::FrequencyDomain) {
      blockSize = stepSize * 2;
    } else {
      blockSize = stepSize;
    }
    Rcpp::Rcerr << blockSize << endl;
  }
  int overlapSize = blockSize - stepSize;
  sf_count_t currentStep = 0;
  int finalStepsRemaining = max(1, (blockSize / stepSize) - 1); // at end of file, this many part-silent frames needed after we hit EOF
  
  // Use actual channel count from Wave object (PluginChannelAdapter will handle mismatches)
  int channels = sfinfo.channels;
  
  float *filebuf = new float[blockSize * channels];
  float **plugbuf = new float*[channels];
  for (int c = 0; c < channels; ++c) plugbuf[c] = new float[blockSize + 2];
  
  Rcpp::Rcerr << "Using block size = " << blockSize << ", step size = "
       << stepSize << endl;
  
  // The channel queries here are for informational purposes only --
  // a PluginChannelAdapter is being used automatically behind the
  // scenes, and it will take case of any channel mismatch
  
  int minch = plugin->getMinChannelCount();
  int maxch = plugin->getMaxChannelCount();
  Rcpp::Rcerr << "Plugin accepts " << minch << " -> " << maxch << " channel(s)" << endl;
  Rcpp::Rcerr << "Sound file has " << channels << " (will mix/augment if necessary)" << endl;
  
  Plugin::OutputList outputs = plugin->getOutputDescriptors();
  Plugin::OutputDescriptor od;
  Plugin::FeatureSet features;
  
  int returnValue = 1;
  int progress = 0;
  
  RealTime rt;
  PluginWrapper *wrapper = 0;
  RealTime adjustment = RealTime::zeroTime;
  
  // Declare these here to avoid goto issues
  NumericVector left;
  int totalSamples = 0;
  int samplesRead = 0;
  
  if (outputs.empty()) {
    Rcpp::Rcerr << "ERROR: Plugin has no outputs!" << endl;
    goto done;
  }
  
  if (outputNo < 0) {
    
    for (size_t oi = 0; oi < outputs.size(); ++oi) {
      if (outputs[oi].identifier == output) {
        outputNo = oi;
        break;
      }
    }
    
    if (outputNo < 0) {
      Rcpp::Rcerr << "ERROR: Non-existent output \"" << output << "\" requested" << endl;
      goto done;
    }
    
  } else {
    
    if (int(outputs.size()) <= outputNo) {
      Rcpp::Rcerr << "ERROR: Output " << outputNo << " requested, but plugin has only " << outputs.size() << " output(s)" << endl;
      goto done;
    }        
  }
  
  od = outputs[outputNo];
  Rcpp::Rcerr << "Output is: \"" << od.identifier << "\"" << endl;
  
  if (!plugin->initialise(channels, stepSize, blockSize)) {
    Rcpp::Rcerr << "ERROR: Plugin initialise (channels = " << channels
         << ", stepSize = " << stepSize << ", blockSize = "
         << blockSize << ") failed." << endl;
    goto done;
  }
  
  wrapper = dynamic_cast<PluginWrapper *>(plugin);
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

    if ((blockSize==stepSize) || (currentStep==0)) {

      // read a full fresh block
      int samplesToRead = min(blockSize, totalSamples - samplesRead);
      
      // Put data into filebuf (interleaved for stereo)
      for (int i = 0; i < samplesToRead; i++) {
        filebuf[i * channels] = left[samplesRead + i];
        if (channels == 2) {
          filebuf[i * channels + 1] = right_channel[samplesRead + i];
        }
      }
      // Zero-pad if we don't have enough samples
      for (int i = samplesToRead; i < blockSize; i++) {
        filebuf[i * channels] = 0.0f;
        if (channels == 2) {
          filebuf[i * channels + 1] = 0.0f;
        }
      }
      
      count = samplesToRead;
      samplesRead += stepSize;
      
      if (count != blockSize) --finalStepsRemaining;
    } else {

      // otherwise shunt the existing data down and read the remainder.
      memmove(filebuf, filebuf + (stepSize * channels), overlapSize * channels * sizeof(float));
      
      int samplesToRead = min(stepSize, totalSamples - samplesRead);
      for (int i = 0; i < samplesToRead; i++) {
        filebuf[(overlapSize + i) * channels] = left[samplesRead + i];
        if (channels == 2) {
          filebuf[(overlapSize + i) * channels + 1] = right_channel[samplesRead + i];
        }
      }
      // Zero-pad if we don't have enough samples
      for (int i = samplesToRead; i < stepSize; i++) {
        filebuf[(overlapSize + i) * channels] = 0.0f;
        if (channels == 2) {
          filebuf[(overlapSize + i) * channels + 1] = 0.0f;
        }
      }
      
      count = overlapSize + samplesToRead;
      samplesRead += stepSize;
      
      if (samplesToRead != stepSize) --finalStepsRemaining;
    }

    // De-interleave audio data for plugin
    for (int c = 0; c < channels; ++c) {
      int j = 0;
      while (j < count) {
        plugbuf[c][j] = filebuf[j * channels + c];
        ++j;
      }

      while (j < blockSize) {
        plugbuf[c][j] = 0.0f;
        ++j;
      }
    }

    rt = RealTime::frame2RealTime(currentStep * stepSize, sfinfo.samplerate);

    features = plugin->process(plugbuf, rt);

    // Collect features in memory
    collectFeatures
      (RealTime::realTime2Frame(rt + adjustment, sfinfo.samplerate),
       sfinfo.samplerate, od, outputNo, features, featureData, useFrames);
    
    // Also write to file if requested
    if (out) {
      printFeatures
        (RealTime::realTime2Frame(rt + adjustment, sfinfo.samplerate),
         sfinfo.samplerate, od, outputNo, features, out, useFrames);
    }

    if (sfinfo.frames > 0){
      int pp = progress;
      progress = (int)((float(currentStep * stepSize) / sfinfo.frames) * 100.f + 0.5f);
      if (progress != pp && out) {
        Rcpp::Rcerr << "\r" << progress << "%";
      }
    }
    
    ++currentStep;
    
  } while (finalStepsRemaining > 0);
  
  if (out) Rcpp::Rcerr << "\rDone" << endl;
  
  rt = RealTime::frame2RealTime(currentStep * stepSize, sfinfo.samplerate);
  
  features = plugin->getRemainingFeatures();
  
  // Collect remaining features
  collectFeatures(RealTime::realTime2Frame(rt + adjustment, sfinfo.samplerate),
                  sfinfo.samplerate, od, outputNo, features, featureData, useFrames);
  
  // Also write to file if requested
  if (out) {
    printFeatures(RealTime::realTime2Frame(rt + adjustment, sfinfo.samplerate),
                  sfinfo.samplerate, od, outputNo, features, out, useFrames);
  }
  
  returnValue = 0;
  
  done:
    delete plugin;
  if (out) {
    out->close();
    delete out;
  }
  
  // Build and return DataFrame
  if (returnValue != 0) {
    // Return empty DataFrame on error
    return DataFrame::create();
  }
  
  // Create DataFrame from collected features
  DataFrame result;
  
  if (featureData.timestamp.empty()) {
    // No features extracted
    result = DataFrame::create(
      Named("timestamp") = NumericVector::create(),
      Named("duration") = NumericVector::create(),
      Named("label") = CharacterVector::create()
    );
  } else {
    // Build value columns
    List valueColumns;
    for (int i = 0; i < featureData.numValueCols; i++) {
      NumericVector col(featureData.timestamp.size(), NA_REAL);
      for (size_t j = 0; j < featureData.values.size(); j++) {
        if (i < (int)featureData.values[j].size()) {
          col[j] = featureData.values[j][i];
        }
      }
      std::string colName = "value";
      if (featureData.numValueCols > 1) {
        colName += std::to_string(i + 1);
      }
      valueColumns[colName] = col;
    }
    
    // Build the DataFrame
    List columns;
    columns["timestamp"] = wrap(featureData.timestamp);
    columns["duration"] = wrap(featureData.duration);
    
    // Add value columns
    for (int i = 0; i < featureData.numValueCols; i++) {
      std::string colName = "value";
      if (featureData.numValueCols > 1) {
        colName += std::to_string(i + 1);
      }
      columns[colName] = valueColumns[colName];
    }
    
    columns["label"] = wrap(featureData.label);
    
    result = DataFrame(columns);
  }
  
  return result;
}

// [[Rcpp::export]]
void rcpp_type(RObject x){
  if(is<NumericVector>(x)){
    if(Rf_isMatrix(x)) Rcout << "NumericMatrix\n";
    else Rcout << "NumericVector\n";       
  }
  else if(is<IntegerVector>(x)){
    if(Rf_isFactor(x)) Rcout << "factor\n";
    else Rcout << "IntegerVector\n";
  }
  else if(is<CharacterVector>(x))
    Rcout << "CharacterVector\n";
  else if(is<LogicalVector>(x))
    Rcout << "LogicalVector\n";
  else if(is<DataFrame>(x))
    Rcout << "DataFrame\n";
  else if(is<List>(x))
    Rcout << "List\n";
  else if(x.isS4())
    Rcout << "S4\n";
  else if(x.isNULL())
    Rcout << "NULL\n";
  else
    Rcout << "unknown\n";
}

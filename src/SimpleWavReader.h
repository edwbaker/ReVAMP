#ifndef SIMPLE_WAV_READER_H
#define SIMPLE_WAV_READER_H

#include <string>
#include <vector>
#include <fstream>
// #include <iostream>
#include <cstring>
#include <algorithm>
#include <cstdint>
#include <Rcpp.h>

class SimpleWavReader {
public:
    struct Header {
        uint16_t channels;
        uint32_t sampleRate;
        uint16_t bitsPerSample;
        uint32_t dataSize; // in bytes
        uint16_t audioFormat; // 1 = PCM, 3 = IEEE Float
    };

    static bool read(const std::string& filename, std::vector<float>& data, Header& header) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            Rcpp::Rcerr << "Failed to open file: " << filename << "\n";
            return false;
        }

        char chunkId[4];
        file.read(chunkId, 4);
        if (std::strncmp(chunkId, "RIFF", 4) != 0) {
             Rcpp::Rcerr << "Not a RIFF file" << "\n";
             return false;
        }

        uint32_t riffSize;
        file.read(reinterpret_cast<char*>(&riffSize), 4);

        char format[4];
        file.read(format, 4);
        if (std::strncmp(format, "WAVE", 4) != 0) {
             Rcpp::Rcerr << "Not a WAVE file" << "\n";
             return false;
        }

        bool fmtFound = false;
        bool dataFound = false;

        while (file.read(chunkId, 4)) {
            uint32_t chunkSize;
            file.read(reinterpret_cast<char*>(&chunkSize), 4);
            
            // Rcpp::Rcerr << "Chunk: " << std::string(chunkId, 4) << " Size: " << chunkSize << std::endl;

            if (std::strncmp(chunkId, "fmt ", 4) == 0) {
                file.read(reinterpret_cast<char*>(&header.audioFormat), 2);
                file.read(reinterpret_cast<char*>(&header.channels), 2);
                file.read(reinterpret_cast<char*>(&header.sampleRate), 4);
                uint32_t byteRate;
                file.read(reinterpret_cast<char*>(&byteRate), 4);
                uint16_t blockAlign;
                file.read(reinterpret_cast<char*>(&blockAlign), 2);
                file.read(reinterpret_cast<char*>(&header.bitsPerSample), 2);
                
                // Handle WAVE_FORMAT_EXTENSIBLE (65534)
                uint32_t bytesRead = 16;
                if (header.audioFormat == 65534) {
                    uint16_t cbSize;
                    file.read(reinterpret_cast<char*>(&cbSize), 2);
                    bytesRead += 2;
                    
                    if (cbSize >= 22) {
                        uint16_t validBitsPerSample;
                        file.read(reinterpret_cast<char*>(&validBitsPerSample), 2);
                        uint32_t dwChannelMask;
                        file.read(reinterpret_cast<char*>(&dwChannelMask), 4);
                        
                        // Read SubFormat GUID (16 bytes)
                        // The first 2 bytes of the GUID match the standard PCM/Float codes
                        uint16_t subFormatCode;
                        file.read(reinterpret_cast<char*>(&subFormatCode), 2);
                        
                        // Skip the rest of the GUID (14 bytes)
                        file.seekg(14, std::ios::cur);
                        
                        // Update audioFormat to the actual underlying format
                        header.audioFormat = subFormatCode;
                        bytesRead += 22;
                    }
                }

                // Rcpp::Rcerr << "Format: " << header.audioFormat << " Channels: " << header.channels << " Rate: " << header.sampleRate << " Bits: " << header.bitsPerSample << std::endl;

                if (chunkSize > bytesRead) {
                    file.seekg(chunkSize - bytesRead, std::ios::cur);
                }
                fmtFound = true;
            } else if (std::strncmp(chunkId, "data", 4) == 0) {
                if (!fmtFound) {
                     Rcpp::Rcerr << "data chunk before fmt chunk" << "\n";
                     return false; 
                }

                header.dataSize = chunkSize;
                int numSamples = header.dataSize / (header.bitsPerSample / 8);
                data.resize(numSamples);

                if (header.audioFormat == 1) { // PCM
                    if (header.bitsPerSample == 16) {
                        std::vector<int16_t> buffer(numSamples);
                        file.read(reinterpret_cast<char*>(buffer.data()), header.dataSize);
                        for (int i = 0; i < numSamples; ++i) {
                            data[i] = buffer[i] / 32768.0f;
                        }
                    } else if (header.bitsPerSample == 8) {
                        std::vector<uint8_t> buffer(numSamples);
                        file.read(reinterpret_cast<char*>(buffer.data()), header.dataSize);
                        for (int i = 0; i < numSamples; ++i) {
                            data[i] = (buffer[i] - 128) / 128.0f;
                        }
                    } else if (header.bitsPerSample == 24) {
                         // 24-bit is tricky (3 bytes). 
                         // Read 3 bytes at a time.
                         uint8_t buf[3];
                         for (int i = 0; i < numSamples; ++i) {
                             file.read(reinterpret_cast<char*>(buf), 3);
                             int32_t val = (buf[0]) | (buf[1] << 8) | (buf[2] << 16);
                             if (val & 0x800000) val |= 0xFF000000; // Sign extend
                             data[i] = val / 8388608.0f;
                         }
                    } else if (header.bitsPerSample == 32) {
                        std::vector<int32_t> buffer(numSamples);
                        file.read(reinterpret_cast<char*>(buffer.data()), header.dataSize);
                        for (int i = 0; i < numSamples; ++i) {
                            data[i] = buffer[i] / 2147483648.0f;
                        }
                    } else {
                         Rcpp::Rcerr << "Unsupported PCM bit depth: " << header.bitsPerSample << "\n";
                         return false;
                    }
                } else if (header.audioFormat == 3) { // IEEE Float
                    if (header.bitsPerSample == 32) {
                        file.read(reinterpret_cast<char*>(data.data()), header.dataSize);
                    } else {
                        Rcpp::Rcerr << "Unsupported float bit depth: " << header.bitsPerSample << "\n";
                        return false;
                    }
                } else {
                    Rcpp::Rcerr << "Unsupported audio format: " << header.audioFormat << "\n";
                    return false;
                }
                dataFound = true;
                break; // Stop after reading data
            } else {
                // Rcpp::Rcerr << "Skipping chunk: " << std::string(chunkId, 4) << std::endl;
                file.seekg(chunkSize + (chunkSize & 1), std::ios::cur);
            }
        }
        if (!dataFound) Rcpp::Rcerr << "No data chunk found" << "\n";
        return dataFound;
    }
};

#endif

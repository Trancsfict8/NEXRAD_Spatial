#include "Level3Parser.h"
#include <sstream>
#include <algorithm>
#include "Deps/zlib/zlib.h"
#include "Deps/bzip2/bzlib.h"

std::vector<FStormAttribute> Level3Parser::ParseStormAttributes(const uint8_t* data, size_t size, bool isTVS) {
    std::vector<FStormAttribute> results;
    if (size == 0 || !data) return results;

    const uint8_t* payloadData = data;
    size_t payloadSize = size;
    
    // Robust scan for compression magic bytes within the first 100 bytes
    for (size_t i = 0; i < std::min((size_t)100, size - 3); i++) {
        if (data[i] == 'B' && data[i+1] == 'Z' && data[i+2] == 'h') {
            payloadData = data + i;
            payloadSize = size - i;
            break;
        } else if (data[i] == 0x78 && (data[i+1] == 0x9c || data[i+1] == 0x01 || data[i+1] == 0xda)) {
            payloadData = data + i;
            payloadSize = size - i;
            break;
        }
    }

    std::string str;
    
    // Check for bzip2 compression
    if (payloadSize > 3 && payloadData[0] == 'B' && payloadData[1] == 'Z' && payloadData[2] == 'h') {
        unsigned int outLen = 5000000; // 5MB should be enough
        char* outBuf = new char[outLen];
        int bzError = BZ2_bzBuffToBuffDecompress(outBuf, &outLen, (char*)payloadData, payloadSize, 0, 0);
        if (bzError == BZ_OK) {
            str = std::string(outBuf, outLen);
        }
        delete[] outBuf;
    }
    // Check for zlib compression
    else if (payloadSize > 2 && payloadData[0] == 0x78 && (payloadData[1] == 0x9c || payloadData[1] == 0x01 || payloadData[1] == 0xda)) {
        uLongf outLen = 5000000;
        Bytef* outBuf = new Bytef[outLen];
        int zError = uncompress(outBuf, &outLen, payloadData, payloadSize);
        if (zError == Z_OK) {
            str = std::string((char*)outBuf, outLen);
        }
        delete[] outBuf;
    }
    
    // Fallback to reading raw
    if (str.empty()) {
        str = std::string((const char*)data, size); 
    }

    
    // Look for the AZ/RAN header in the alphanumeric block
    size_t headerPos = str.find("AZ/RAN");
    if (headerPos == std::string::npos) return results;
    
    size_t lineStart = str.find('\n', headerPos);
    if (lineStart == std::string::npos) return results;
    
    while (true) {
        lineStart++; // Skip newline
        if (lineStart >= str.length()) break;

        size_t lineEnd = str.find('\n', lineStart);
        if (lineEnd == std::string::npos) lineEnd = str.length();
        
        std::string line = str.substr(lineStart, lineEnd - lineStart);
        
        // Remove carriage returns
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        
        if (line.empty() || line.find_first_not_of(" \t") == std::string::npos) {
            // End of table or empty line
            if (results.size() > 0 && line.empty()) break;
            lineStart = lineEnd;
            continue; 
        }
        
        // Example Hail line:   C4       125/45     90    50      1.50   
        // Example TVS line:    C4       125/45     50    30    40    2.5   15.0 
        std::stringstream ss(line);
        std::string stormId, azRan;
        ss >> stormId >> azRan;
        
        if (!stormId.empty() && !azRan.empty() && stormId.find("---") == std::string::npos) {
            size_t slashPos = azRan.find('/');
            if (slashPos != std::string::npos) {
                try {
                    FStormAttribute attr;
                    attr.stormID = stormId;
                    attr.azimuth = std::stof(azRan.substr(0, slashPos));
                    attr.range = std::stof(azRan.substr(slashPos + 1));
                    attr.isTVS = isTVS;
                    attr.isHail = !isTVS;
                    results.push_back(attr);
                } catch (...) {
                    // Ignore parsing errors for individual lines (e.g. invalid float cast)
                }
            }
        }
        
        lineStart = lineEnd;
        if (lineStart >= str.length()) break;
    }
    
    return results;
}

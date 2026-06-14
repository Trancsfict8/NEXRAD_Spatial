#pragma once

#include "CoreMinimal.h"
#include <string>
#include <vector>

struct FStormAttribute {
    std::string stormID;
    float azimuth; // degrees
    float range;   // NM
    bool isTVS;
    bool isHail;
};

class NEXRADSPATIAL_API Level3Parser {
public:
    // Scans raw Level 3 file bytes for the Tabular Alphanumeric Block and extracts the attributes
    static std::vector<FStormAttribute> ParseStormAttributes(const uint8_t* data, size_t size, bool isTVS);
};

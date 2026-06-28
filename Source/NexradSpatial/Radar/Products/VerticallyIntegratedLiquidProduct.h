#pragma once
#include "RadarProduct.h"

class RadarProductVerticallyIntegratedLiquid : public RadarProduct {
public:
	RadarProductVerticallyIntegratedLiquid();
	virtual RadarData* deriveVolume(std::map<RadarData::VolumeType, RadarData*> inputProducts) override;
};

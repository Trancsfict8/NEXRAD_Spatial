#include "VerticallyIntegratedLiquidProduct.h"
#include <algorithm>
#include <cmath>

#define PIF 3.14159265358979323846f

RadarProductVerticallyIntegratedLiquid::RadarProductVerticallyIntegratedLiquid(){
	volumeType = RadarData::VOLUME_VERTICALLY_INTEGRATED_LIQUID;
	productType = PRODUCT_DERIVED_VOLUME;
	name = "Vertically Integrated Liquid";
	shortName = "VIL";
	dependencies = {RadarData::VOLUME_REFLECTIVITY};
}

RadarData* RadarProductVerticallyIntegratedLiquid::deriveVolume(std::map<RadarData::VolumeType, RadarData*> inputProducts){
	RadarData* radarData = new RadarData();
	RadarData* refData = inputProducts[RadarData::VOLUME_REFLECTIVITY];
	refData->Decompress();
	
	// Ensure we copy metadata to avoid the solid 360-degree cylinder bug
	radarData->CopyFrom(refData, true);
	radarData->buffer = new float[radarData->fullBufferSize];
	std::fill(radarData->buffer, radarData->buffer + radarData->fullBufferSize, 0.0f);
	
	float minValue = 999999.0f;
	float maxValue = -999999.0f;
	
	for(int theta = 0; theta < radarData->thetaBufferCount; theta++){
		for(int radius = 0; radius < radarData->radiusBufferCount; radius++){
			float vil = 0.0f;
			float dist = (radarData->stats.innerDistance + radius) * radarData->stats.pixelSize;
			
			// Base layer (ground to sweep 0)
			if (radarData->sweepBufferCount > 0) {
				float dbz0 = refData->buffer[radarData->thetaBufferSize * (theta + 1) + radius];
				if (dbz0 > 0) {
					float z0 = std::pow(10.0f, dbz0 / 10.0f);
					float M = 3.44e-6f * std::pow(z0, 4.0f / 7.0f);
					float el0 = radarData->sweepInfo[0].elevationAngle;
					float dz = dist * std::sin(el0 * PIF / 180.0f);
					vil += M * dz;
				}
			}
			
			for(int sweep = 0; sweep < radarData->sweepBufferCount - 1; sweep++){
				int loc1 = radarData->thetaBufferSize * (theta + 1) + radarData->sweepBufferSize * sweep;
				int loc2 = radarData->thetaBufferSize * (theta + 1) + radarData->sweepBufferSize * (sweep + 1);
				
				float dbz1 = refData->buffer[loc1 + radius];
				float dbz2 = refData->buffer[loc2 + radius];
				
				if(dbz1 <= 0 && dbz2 <= 0) continue;
				
				float z1 = dbz1 > 0 ? std::pow(10.0f, dbz1 / 10.0f) : 0;
				float z2 = dbz2 > 0 ? std::pow(10.0f, dbz2 / 10.0f) : 0;
				float avgZ = (z1 + z2) / 2.0f;
				
				if(avgZ <= 0) continue;
				
				// Calculate liquid water content M
				float M = 3.44e-6f * std::pow(avgZ, 4.0f / 7.0f);
				
				float el1 = radarData->sweepInfo[sweep].elevationAngle;
				float el2 = radarData->sweepInfo[sweep + 1].elevationAngle;
				
				float h1 = dist * std::sin(el1 * PIF / 180.0f);
				float h2 = dist * std::sin(el2 * PIF / 180.0f);
				float dz = std::abs(h2 - h1);
				
				vil += M * dz;
			}
			
			// Fill all sweeps to create a vertical column of VIL data
			for(int sweep = 0; sweep < radarData->sweepBufferCount; sweep++){
				int loc = radarData->thetaBufferSize * (theta + 1) + radarData->sweepBufferSize * sweep;
				radarData->buffer[loc + radius] = vil;
			}
			
			if (vil > 0) {
				minValue = std::min(minValue, vil);
				maxValue = std::max(maxValue, vil);
			}
		}
	}
	
	if (minValue > maxValue) {
		minValue = 0;
		maxValue = 1;
	}
	
	// Ensure stats are explicitly set
	radarData->stats.minValue = minValue;
	radarData->stats.maxValue = std::max(maxValue, 0.0001f);
	radarData->stats.volumeType = volumeType;
	
	radarData->Interpolate();
	return radarData;
}

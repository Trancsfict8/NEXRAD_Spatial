#include "WeatherAudioManager.h"
#include "Async/Async.h"
#include "../Mapping/StormAttributeManager.h"
#include "../Radar/Globe.h"
#include "../Application/GlobalState.h"
#include "RadarGameStateBase.h"
#include "EngineUtils.h"

UWeatherAudioManager::UWeatherAudioManager()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UWeatherAudioManager::BeginPlay()
{
    Super::BeginPlay();
    InitializeAudioComponents();
}

void UWeatherAudioManager::InitializeAudioComponents()
{
    AActor* Owner = GetOwner();
    if (!Owner) return;

    if (!RainAmbientAudio) {
        RainAmbientAudio = NewObject<UAudioComponent>(Owner, TEXT("RainAmbientAudioComp"));
        RainAmbientAudio->SetupAttachment(Owner->GetRootComponent());
        RainAmbientAudio->RegisterComponent();
        USoundBase* RainSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/MS_RainAmbient.MS_RainAmbient"));
        if (RainSound) RainAmbientAudio->SetSound(RainSound);
        USoundAttenuation* RainAtten = LoadObject<USoundAttenuation>(nullptr, TEXT("/Game/Audio/SA_RainEnvelope.SA_RainEnvelope"));
        if (RainAtten) RainAmbientAudio->AttenuationSettings = RainAtten;
        RainAmbientAudio->bAllowSpatialization = false;
        RainAmbientAudio->Play();
    }
    
    if (!SevereCoreAudio) {
        SevereCoreAudio = NewObject<UAudioComponent>(Owner, TEXT("SevereCoreAudioComp"));
        SevereCoreAudio->SetupAttachment(Owner->GetRootComponent());
        SevereCoreAudio->RegisterComponent();
        USoundBase* CoreSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/MS_SevereCore.MS_SevereCore"));
        if (CoreSound) SevereCoreAudio->SetSound(CoreSound);
        USoundAttenuation* CoreAtten = LoadObject<USoundAttenuation>(nullptr, TEXT("/Game/Audio/SA_SevereCore.SA_SevereCore"));
        if (CoreAtten) SevereCoreAudio->AttenuationSettings = CoreAtten;
        SevereCoreAudio->bAllowSpatialization = false;
        SevereCoreAudio->Play();
    }
    
    if (!HailAudio) {
        HailAudio = NewObject<UAudioComponent>(Owner, TEXT("HailAudioComp"));
        HailAudio->SetupAttachment(Owner->GetRootComponent());
        HailAudio->RegisterComponent();
        USoundBase* HailSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/MS_Hail.MS_Hail"));
        if (HailSound) HailAudio->SetSound(HailSound);
        // Hail uses same attenuation as core
        USoundAttenuation* CoreAtten = LoadObject<USoundAttenuation>(nullptr, TEXT("/Game/Audio/SA_SevereCore.SA_SevereCore"));
        if (CoreAtten) HailAudio->AttenuationSettings = CoreAtten;
        HailAudio->bAllowSpatialization = false;
        HailAudio->Play();
    }

    // Silence audio parameters immediately on spawn so it doesn't blast while loading
    if (RainAmbientAudio) RainAmbientAudio->SetFloatParameter(FName("RainIntensity"), 0.0f);
    if (SevereCoreAudio) SevereCoreAudio->SetFloatParameter(FName("CoreDensity"), 0.0f);
    if (HailAudio) {
        HailAudio->SetBoolParameter(FName("IsHailing"), false);
        HailAudio->SetFloatParameter(FName("HailSize"), 0.0f);
    }
}

void UWeatherAudioManager::OnVolumeUpdate(RadarData* data, FVector InRadarCenter)
{
    if (!data || (!data->buffer && !data->buffer8Bit) || !data->sweepInfo) return;
    
    // Only build the audio grid from reflectivity data.
    // Other products (velocity, correlation coefficient, etc.) have completely different
    // value ranges and would produce an empty or nonsensical grid.
    if (data->stats.volumeType != RadarData::VOLUME_REFLECTIVITY) return;
    
    if (bIsProcessing.exchange(true)) return; // Drop if already processing

    RadarCenter = InRadarCenter;

    // Make local copies to avoid game thread lifetime issues
    // Using SetNumUninitialized + Memcpy avoids TArray reallocations during Append and is much faster for large arrays
    TArray<uint8_t> LocalBuffer8;
    TArray<float> LocalBufferFloat;
    bool bUsing8Bit = false;

    if (data->buffer8Bit != nullptr) {
        LocalBuffer8.SetNumUninitialized(data->fullBufferSize);
        FMemory::Memcpy(LocalBuffer8.GetData(), data->buffer8Bit, data->fullBufferSize * sizeof(uint8_t));
        bUsing8Bit = true;
    } else {
        LocalBufferFloat.SetNumUninitialized(data->fullBufferSize);
        FMemory::Memcpy(LocalBufferFloat.GetData(), data->buffer, data->fullBufferSize * sizeof(float));
    }

    TArray<RadarData::SweepInfo> LocalSweeps;
    LocalSweeps.SetNumUninitialized(data->sweepBufferCount);
    FMemory::Memcpy(LocalSweeps.GetData(), data->sweepInfo, data->sweepBufferCount * sizeof(RadarData::SweepInfo));

    RadarData::Stats LocalStats = data->stats;
    FVector Center = InRadarCenter;

    int radiusBufferCount = data->radiusBufferCount;
    int thetaBufferCount = data->thetaBufferCount;
    int sweepBufferCount = data->sweepBufferCount;
    int sweepBufferSize = data->sweepBufferSize;
    int thetaBufferSize = data->thetaBufferSize;

    // Launch async task
    // MoveTemp the local arrays into the lambda to avoid a secondary expensive allocation + copy of the 100MB buffer
    Async(EAsyncExecution::ThreadPool, [this, LocalBuffer8 = MoveTemp(LocalBuffer8), LocalBufferFloat = MoveTemp(LocalBufferFloat), bUsing8Bit, LocalSweeps = MoveTemp(LocalSweeps), LocalStats, Center, radiusBufferCount, thetaBufferCount, sweepBufferCount, sweepBufferSize, thetaBufferSize]() {
        TArray<float> TempGrid;
        TempGrid.Init(-INFINITY, GridResXY * GridResXY * GridResZ);

        FVector TempRainSum = FVector::ZeroVector;
        FVector TempCoreSum = FVector::ZeroVector;
        int TempRainCount = 0;
        int TempCoreCount = 0;

        const float GridCellSizeXY = (GridBoundsXY * 2) / GridResXY; // 78.125 UU
        const float GridCellSizeZ = GridBoundsZ / GridResZ;          // 12.5 UU

        for (int s = 0; s < sweepBufferCount; s++) {
            float elevDeg = LocalSweeps[s].elevationAngle;
            float elevRad = FMath::DegreesToRadians(elevDeg);
            float cosElev = FMath::Cos(elevRad);
            float sinElev = FMath::Sin(elevRad);

            for (int t = 0; t < thetaBufferCount; t++) {
                float azimuthDeg = t * 360.0f / thetaBufferCount;
                float azimRad = FMath::DegreesToRadians(azimuthDeg);
                float cosAzim = FMath::Cos(azimRad);
                float sinAzim = FMath::Sin(azimRad);

                for (int r = 0; r < radiusBufferCount; r++) {
                    int bufferIndex = s * sweepBufferSize + (t + 1) * thetaBufferSize + r;
                    float val = LocalStats.noDataValue;
                    if (bUsing8Bit) {
                        uint8_t intVal = LocalBuffer8[bufferIndex];
                        if (intVal != 0) {
                            float range = LocalStats.maxValue - LocalStats.minValue;
                            float norm = (intVal - 1) / 254.0f;
                            val = (norm * range) + LocalStats.minValue;
                        }
                    } else {
                        val = LocalBufferFloat[bufferIndex];
                    }

                    if (val == LocalStats.noDataValue || val < 0.0f) continue;

                    float distMeters = (r + LocalStats.innerDistance) * LocalStats.pixelSize;
                    float groundDistMeters = distMeters * cosElev;
                    float zMeters = distMeters * sinElev;
                    float xMeters = groundDistMeters * cosAzim;
                    float yMeters = groundDistMeters * sinAzim;

                    FVector localOffset(xMeters * 0.01f, yMeters * 0.01f, zMeters * 0.01f);

                    if (val >= 25.0f && val < 50.0f) {
                        TempRainSum += localOffset;
                        TempRainCount++;
                    } else if (val >= 50.0f) {
                        TempCoreSum += localOffset;
                        TempCoreCount++;
                    }

                    int gx = FMath::FloorToInt((localOffset.X + GridBoundsXY) / GridCellSizeXY);
                    int gy = FMath::FloorToInt((localOffset.Y + GridBoundsXY) / GridCellSizeXY);
                    int gz = FMath::FloorToInt(localOffset.Z / GridCellSizeZ);

                    if (gx >= 0 && gx < GridResXY && gy >= 0 && gy < GridResXY && gz >= 0 && gz < GridResZ) {
                        int flatIdx = gx + (gy * GridResXY) + (gz * GridResXY * GridResXY);
                        TempGrid[flatIdx] = FMath::Max(TempGrid[flatIdx], val);
                    }
                }
            }
        }

        // Lock and apply to game thread state
        {
            FScopeLock Lock(&GridMutex);
            AudioGrid = MoveTemp(TempGrid);
            
            // Convert centroid from grid/radar space to world space before adding Center
            // Grid→World: worldX = gridY, worldY = -gridX, worldZ = gridZ
            if (TempRainCount > 0) {
                FVector avgGrid = TempRainSum / TempRainCount;
                RainEnvelopeCentroid = Center + FVector(avgGrid.Y, -avgGrid.X, avgGrid.Z);
            }
            if (TempCoreCount > 0) {
                FVector avgGrid = TempCoreSum / TempCoreCount;
                SevereCoreCentroid = Center + FVector(avgGrid.Y, -avgGrid.X, avgGrid.Z);
            }
            
            RainVoxelCount = TempRainCount;
            CoreVoxelCount = TempCoreCount;

            GridReady = true;
            bIsProcessing.store(false);
        }
    });
}

bool UWeatherAudioManager::GetCurrentCellData(FVector CameraPos, float& OutMean, float& OutMax, FVector& CellCenter)
{
    FScopeLock Lock(&GridMutex);
    if (!GridReady || AudioGrid.IsEmpty()) return false;

    FVector localPos = CameraPos - RadarCenter;
    
    // The audio grid is built using radar azimuth convention:
    //   Azimuth 0° (North) → grid +X,  Azimuth 90° (East) → grid +Y
    // But the globe world-space axes are oriented differently:
    //   North → world -Y,  East → world +X
    // So we need to remap: gridX = -localPos.Y, gridY = localPos.X
    float gridLocalX = -localPos.Y;
    float gridLocalY = localPos.X;
    float gridLocalZ = localPos.Z;
    
    const float GridCellSizeXY = (GridBoundsXY * 2) / GridResXY;
    const float GridCellSizeZ = GridBoundsZ / GridResZ;

    int gx = FMath::FloorToInt((gridLocalX + GridBoundsXY) / GridCellSizeXY);
    int gy = FMath::FloorToInt((gridLocalY + GridBoundsXY) / GridCellSizeXY);
    int gz = FMath::FloorToInt(gridLocalZ / GridCellSizeZ);

    // If the player flies far away, the earth curves downward and CameraPos.Z drops below RadarCenter.Z!
    // We clamp gz to 0 so we still search the storm column above them even if they are 'below' the radar's horizontal plane.
    gz = FMath::Max(0, gz);

    if (gx >= 0 && gx < GridResXY && gy >= 0 && gy < GridResXY && gz < GridResZ) {
        // Project column down: Find the MAXIMUM radar return in all grid cells in the entire column
        // This ensures the sound doesn't drop out when flying up past the lowest radar elevation scans
        float maxColumnVal = -INFINITY;
        for (int z = 0; z < GridResZ; z++) {
            int flatIdx = gx + (gy * GridResXY) + (z * GridResXY * GridResXY);
            if (AudioGrid[flatIdx] > maxColumnVal) {
                maxColumnVal = AudioGrid[flatIdx];
            }
        }
        
        OutMax = maxColumnVal;
        OutMean = OutMax; // We only store max now, which is better for localized audio intensity

        // Convert grid-local coordinates back to world-space offset
        // Inverse of the forward transform: worldX = gridLocalY, worldY = -gridLocalX
        float gridCellX = -GridBoundsXY + (gx + 0.5f) * GridCellSizeXY;
        float gridCellY = -GridBoundsXY + (gy + 0.5f) * GridCellSizeXY;
        float gridCellZ = (gz + 0.5f) * GridCellSizeZ;
        CellCenter = RadarCenter + FVector(gridCellY, -gridCellX, gridCellZ);
        return true;
    }
    
    return false;
}

void UWeatherAudioManager::UpdateAudioState(float DeltaTime, FVector CameraPos)
{
    if (!GridReady) return;
    float localMean = -INFINITY;
    float localMax = -INFINITY;
    FVector cellCenter;
    bool insideVolume = GetCurrentCellData(CameraPos, localMean, localMax, cellCenter);

    // Check if the user has disabled storm sounds in the settings
    // Also, mute the sounds if the player is under 5000 ft (1524 meters) to avoid ground clutter triggering rain sounds.
    bool bEnableStormSounds = true;
    if (UWorld* World = GetWorld()) {
        if (ARadarGameStateBase* GameState = World->GetGameState<ARadarGameStateBase>()) {
            bEnableStormSounds = GameState->globalState.enableStormSounds;
            
            if (bEnableStormSounds && GameState->globalState.globe) {
                SimpleVector3<double> latLonAlt = GameState->globalState.globe->GetLatLonAltDegrees(SimpleVector3<double>(CameraPos.X, CameraPos.Y, CameraPos.Z));
                double ex = GameState->globalState.elevationExaggeration;
                double realAltitudeMeters = (ex > 0.0) ? (latLonAlt.z / ex) : latLonAlt.z;
                
                if (realAltitudeMeters < 1524.0) { // 1524 meters = 5000 ft
                    bEnableStormSounds = false;
                }
            }
        }
    }

    if (insideVolume && bEnableStormSounds) {
        float targetRain = 0.0f;
        float targetCore = 0.0f;
        
        // Map rain intensity (25 dBZ to 50 dBZ)
        if (localMax >= 25.0f && localMax < 50.0f) {
            // Start at 0.3 so it's immediately audible at 25 dBZ
            float lerpVal = FMath::Clamp((localMax - 25.0f) / 25.0f, 0.0f, 1.0f);
            targetRain = FMath::Lerp(0.3f, 1.0f, lerpVal);
        }
        else if (localMax >= 50.0f) {
            // Above 50, rain stays maxed while core fades in
            targetRain = 1.0f;
            targetCore = FMath::Clamp((localMax - 50.0f) / 20.0f, 0.0f, 1.0f);
        }

        // Apply targets directly (Removed vertical attenuation so you hear the storm at any altitude in the cell)
        RainIntensity = targetRain;
        CoreDensity = targetCore;

        // if (GEngine) {
        //     GEngine->AddOnScreenDebugMessage(101, 1.0f, FColor::Green, FString::Printf(TEXT("INSIDE VOLUME - Cell Max: %.1f dBZ | Rain: %.2f | Core: %.2f"), localMax, RainIntensity, CoreDensity));
        // }

        // Place audio directly at CameraPos to ensure it is never outside the Attenuation radius.
        if (RainAmbientAudio) RainAmbientAudio->SetWorldLocation(CameraPos);
        if (SevereCoreAudio) SevereCoreAudio->SetWorldLocation(CameraPos);

    } else {
        // Fade completely out when exiting the storm grid (above/below or far outside)
        float targetRain = 0.0f;
        float targetCore = 0.0f;
        
        // Snap immediately to 0
        RainIntensity = targetRain;
        CoreDensity = targetCore;

        // if (GEngine) {
        //     GEngine->AddOnScreenDebugMessage(101, 1.0f, FColor::Yellow, FString::Printf(TEXT("OUTSIDE VOLUME - Fading Out Audio")));
        // }

        if (RainAmbientAudio) RainAmbientAudio->SetWorldLocation(CameraPos);
        if (SevereCoreAudio) SevereCoreAudio->SetWorldLocation(CameraPos);
    }

    IsHailing = false;
    HailSize = 0.0f;

    // Trigger hail automatically if radar return is extremely high
    if (insideVolume && bEnableStormSounds && localMax > 55.0f) {
        IsHailing = true;
        HailSize = FMath::Clamp((localMax - 55.0f) / 10.0f, 0.2f, 1.0f);
    }
    if (!IsValid(CachedStormMgr)) {
        if (UWorld* World = GetWorld()) {
            for (TActorIterator<AStormAttributeManager> It(World); It; ++It) {
                CachedStormMgr = *It;
                break;
            }
        }
    }
    
    if (CachedStormMgr) {
        AStormAttributeManager::FStormAttr closestHail;
        float minDist = FLT_MAX;
        
        // Copy the array inside the lock so we don't hold the mutex while doing expensive math
        TArray<AStormAttributeManager::FStormAttr> localAttributes;
        {
            FScopeLock Lock(&CachedStormMgr->attributesMutex);
            localAttributes = CachedStormMgr->currentAttributes;
        }

        for (const auto& attr : localAttributes) {
            if (attr.type == "HAIL") {
                // Approximate position for the hail using standard UE distance from center.
                // We don't have direct globe access easily here without GlobalState, 
                // but we can query GlobalState if needed. Wait, we still need GlobalState for Globe!
                GlobalState* globalState = nullptr;
                if (UWorld* World = GetWorld()) {
                    if (ARadarGameStateBase* GameState = World->GetGameState<ARadarGameStateBase>()) {
                        globalState = &GameState->globalState;
                    }
                }
                if (globalState && globalState->globe) {
                    SimpleVector3<double> hailPos = globalState->globe->GetPointScaledDegrees(
                        (double)attr.lat, 
                        (double)attr.lon, 
                        (double)(attr.top * 1000.0f * 0.3048f)
                    );
                    FVector hailFPos(hailPos.x, hailPos.y, hailPos.z);
                    float dist = FVector::Dist(CameraPos, hailFPos);
                    if (dist < minDist) {
                        minDist = dist;
                        closestHail = attr;
                    }
                }
            }
        }

        // 50.0f UU = 5 km radius for hail core audio
        if (minDist < 50.0f && bEnableStormSounds) { 
            IsHailing = true;
            HailSize = closestHail.maxSize;
        }
    }

    if (HailAudio) {
        HailAudio->SetWorldLocation(CameraPos);
    }

    if (RainAmbientAudio) {
        RainAmbientAudio->SetFloatParameter(FName("RainIntensity"), RainIntensity);
    }
    if (SevereCoreAudio) {
        SevereCoreAudio->SetFloatParameter(FName("CoreDensity"), CoreDensity);
    }
    if (HailAudio) {
        HailAudio->SetBoolParameter(FName("IsHailing"), IsHailing);
        HailAudio->SetFloatParameter(FName("HailSize"), HailSize);
    }
}

void UWeatherAudioManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

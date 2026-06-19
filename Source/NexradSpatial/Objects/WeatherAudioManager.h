#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/AudioComponent.h"
#include "../Radar/RadarData.h"
#include <mutex>

#include "WeatherAudioManager.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class NEXRADSPATIAL_API UWeatherAudioManager : public UActorComponent
{
    GENERATED_BODY()

public:
    UWeatherAudioManager();

    // --- Audio Components ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather Audio")
    UAudioComponent* RainAmbientAudio = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather Audio")
    UAudioComponent* SevereCoreAudio = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather Audio")
    UAudioComponent* HailAudio = nullptr;

    // --- MetaSound Runtime Inputs ---
    float RainIntensity = 0.0f;       // 0.0-1.0
    float CoreDensity = 0.0f;         // 0.0-1.0
    bool IsHailing = false;
    float HailSize = 0.0f;

    // --- Spatial Grid ---
    TArray<float> AudioGrid;
    const float GridBoundsXY = 5000.0f; // +/- 5000 UU = +/- 500 km
    const float GridBoundsZ = 200.0f;   // 0 to 200 UU = 0 to 20 km
    const int GridResXY = 128;
    const int GridResZ = 16;

    // --- Centroids (for outside-volume positioning) ---
    FVector RainEnvelopeCentroid = FVector::ZeroVector;
    FVector SevereCoreCentroid = FVector::ZeroVector;
    int RainVoxelCount = 0;
    int CoreVoxelCount = 0;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Background thread hook
    void OnVolumeUpdate(RadarData* data, FVector InRadarCenter);
    
    // Run on main thread
    void UpdateAudioState(float DeltaTime, FVector CameraPos);

    // Get current cell data for ImGui
    bool GetCurrentCellData(FVector CameraPos, float& OutMean, float& OutMax, FVector& CellCenter);

protected:
    virtual void BeginPlay() override;

private:
    FCriticalSection GridMutex;
    bool GridReady = false;
    FVector RadarCenter;

    // Async task state
    std::atomic<bool> bIsProcessing{false};

    void InitializeAudioComponents();

    class AStormAttributeManager* CachedStormMgr = nullptr;
};

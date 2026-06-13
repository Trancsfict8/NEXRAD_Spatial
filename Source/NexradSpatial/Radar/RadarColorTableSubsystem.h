// Copyright

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "RadarColorTableSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FCustomColorTableData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Radar Color Tables")
    TArray<float> ColorData;

    UPROPERTY(BlueprintReadOnly, Category = "Radar Color Tables")
    float MinValue = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Radar Color Tables")
    float MaxValue = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Radar Color Tables")
    bool bIsValid = false;
};

/**
 * Subsystem to manage dynamic generation of Radar Color Tables from CSV/PAL files
 */
UCLASS()
class NEXRADSPATIAL_API URadarColorTableSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UPROPERTY(BlueprintReadOnly, Category = "Radar Color Tables")
    TMap<FString, FCustomColorTableData> ReflectivityTables;

    UPROPERTY(BlueprintReadOnly, Category = "Radar Color Tables")
    TMap<FString, FCustomColorTableData> VelocityTables;

    /** 
     * Pushes the dynamic 1D Texture to a Material Instance Dynamic. 
     * Handles the normalization of the raw radar moments to a standard 0.0 to 1.0 UV space.
     */
    UFUNCTION(BlueprintCallable, Category = "Radar Color Tables")
    void SetRadarColorTableMaterial(UMaterialInstanceDynamic* Material, FName TextureParamName, FName MinParamName, FName MaxParamName, bool bIsReflectivity, FString TableName, bool& bOutSuccess);

    /** Gets the dynamically generated custom Color Table raw float data and its min/max values. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Radar Color Tables")
    void GetRadarColorTable(bool bIsReflectivity, FString TableName, TArray<float>& OutColorData, float& OutMinValue, float& OutMaxValue, bool& bOutIsValid);

private:
    void ScanAndLoadFolder(const FString& FolderPath, TMap<FString, FCustomColorTableData>& OutTables);
    bool ParseColorTableFile(const FString& FilePath, FCustomColorTableData& OutData);
};

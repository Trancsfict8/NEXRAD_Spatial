#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IHttpRequest.h"
#include "Materials/Material.h"
#include <vector>

class UInstancedStaticMeshComponent;
class UTextRenderComponent;
#include "StormAttributeManager.generated.h"

class Globe;

UCLASS()
class NEXRADSPATIAL_API AStormAttributeManager : public AActor
{
    GENERATED_BODY()
    
public:    
    AStormAttributeManager();
    virtual void Tick(float DeltaTime) override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

private:
    void FetchStormAttributes();
    void OnAttributesFetched(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    void UpdateMeshes();
    void HideAllText();

    UPROPERTY(VisibleAnywhere)
    UInstancedStaticMeshComponent* TvsMeshComponent;
    
    UPROPERTY(VisibleAnywhere)
    UInstancedStaticMeshComponent* HailSmallMeshComponent;
    
    UPROPERTY(VisibleAnywhere)
    UInstancedStaticMeshComponent* HailMediumMeshComponent;
    
    UPROPERTY(VisibleAnywhere)
    UInstancedStaticMeshComponent* HailLargeMeshComponent;

    UPROPERTY(VisibleAnywhere)
    UInstancedStaticMeshComponent* TrackMeshComponent;

    UPROPERTY(VisibleAnywhere)
    UInstancedStaticMeshComponent* TrackMarkerMeshComponent;

    Globe* globe = nullptr;
    float lastFetchTime = 0;
    float fetchInterval = 60.0f; // Fetch every 60 seconds
    
    FString lastSiteId = "";
    bool meshesDirty = true;
    bool meshesNeedClear = true;
    
public:
    struct FStormAttr {
        FString type;
        float lat;
        float lon;
        float maxSize;
        float drct;
        float sknt;
        float top;
        FString storm_id;
    };
    
    TArray<FStormAttr> currentAttributes;
    FCriticalSection attributesMutex;
private:
    std::vector<int> callbackIds;
    
    TArray<UTextRenderComponent*> TextComponents;
    TArray<UTextRenderComponent*> EchoTopTextComponents;
    UMaterial* TextMaterial = nullptr;
};

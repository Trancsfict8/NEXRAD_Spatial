#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpRequest.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
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

    Globe* globe = nullptr;
    float lastFetchTime = 0;
    float fetchInterval = 60.0f; // Fetch every 60 seconds
    
    FString lastSiteId = "";
    
    struct FStormAttr {
        FString type;
        float lat;
        float lon;
        float maxSize;
    };
    
    TArray<FStormAttr> currentAttributes;
    FCriticalSection attributesMutex;
    
    TArray<UTextRenderComponent*> TextComponents;
    UMaterial* TextMaterial = nullptr;
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IHttpRequest.h"
#include <map>
#include <vector>
#include "../Mapping/Data/ShapeFile.h"
#include "WarningManager.generated.h"

class AGISPolyline;
class Globe;

UCLASS()
class NEXRADSPATIAL_API AWarningManager : public AActor
{
	GENERATED_BODY()
	
public:	
	AWarningManager();
	~AWarningManager();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

public:	
	virtual void Tick(float DeltaTime) override;

	void FetchWarnings();
	void OnWarningsFetched(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	std::vector<GISObject*> warningObjects;
	std::map<uint64_t, AGISPolyline*> polylines;

	GISGroup tornadoGroup;
	GISGroup severeGroup;
	GISGroup flashFloodGroup;
	GISGroup specialMarineGroup;
	GISGroup otherGroup;

	Globe* globe = nullptr;
	float lastFetchTime = 0.0f;
	float fetchInterval = 120.0f; // Every 2 minutes
};

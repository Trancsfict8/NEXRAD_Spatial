#include "WarningManager.h"
#include "GISPolyline.h"
#include "../Radar/Globe.h"
#include "../Objects/RadarGameStateBase.h"
#include "../Application/GlobalState.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "JsonUtilities.h"
#include <cmath>

AWarningManager::AWarningManager()
{
	PrimaryActorTick.bCanEverTick = true;
	
	tornadoGroup = GISGroup(3000000, 8, 1.0f, 0.0f, 0.0f); // Red, 3000km view distance, 8m thick line
	severeGroup = GISGroup(3000000, 8, 1.0f, 0.8f, 0.0f); // Yellow
	flashFloodGroup = GISGroup(3000000, 8, 0.0f, 1.0f, 0.0f); // Green
	specialMarineGroup = GISGroup(3000000, 8, 0.0f, 0.5f, 1.0f); // Blue
	otherGroup = GISGroup(3000000, 8, 0.5f, 0.5f, 0.5f); // Gray
}

AWarningManager::~AWarningManager()
{
	for (GISObject* obj : warningObjects) {
		if (obj->geometry) free(obj->geometry);
		obj->Delete();
		delete obj;
	}
	warningObjects.clear();
}

void AWarningManager::BeginPlay()
{
	Super::BeginPlay();
	FetchWarnings();
}

void AWarningManager::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	Super::EndPlay(endPlayReason);
}

void AWarningManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	GlobalState* globalState = nullptr;
	if (GetWorld()->GetGameState<ARadarGameStateBase>()) {
		globalState = &GetWorld()->GetGameState<ARadarGameStateBase>()->globalState;
	}
	if (!globalState) return;
	globe = globalState->globe;
	
	if (lastElevationExaggeration != globalState->elevationExaggeration) {
		lastElevationExaggeration = globalState->elevationExaggeration;
		for (auto& pair : polylines) {
			if (pair.second) pair.second->Destroy();
		}
		polylines.clear();
		for (GISObject* obj : warningObjects) {
			obj->shown = false;
		}
	}
	
	lastFetchTime += DeltaTime;
	if (lastFetchTime > fetchInterval) {
		lastFetchTime = 0;
		FetchWarnings();
	}
	
	// Distance culling and rendering logic
	if (globe && globe->scale > 0) {
		FVector camLoc = GetWorld()->GetFirstPlayerController()->PlayerCameraManager->GetCameraLocation();
		SimpleVector3<float> camSimple(camLoc.X, camLoc.Y, camLoc.Z);
		
		for (size_t i = 0; i < warningObjects.size(); i++) {
			GISObject* obj = warningObjects[i];
			GISGroup* group = &otherGroup;
			if (obj->groupId == 1) group = &tornadoGroup;
			else if (obj->groupId == 2) group = &severeGroup;
			else if (obj->groupId == 3) group = &flashFloodGroup;
			else if (obj->groupId == 4) group = &specialMarineGroup;
			
			bool isEnabled = true;
			if (obj->groupId == 1 && !globalState->showTornadoWarnings) isEnabled = false;
			if (obj->groupId == 2 && !globalState->showSevereWarnings) isEnabled = false;
			if (obj->groupId == 3 && !globalState->showFlashFloodWarnings) isEnabled = false;
			if (obj->groupId == 4 && !globalState->showMarineWarnings) isEnabled = false;

			Globe dummyGlobe = {};
			SimpleVector3<double> camSpherical = globe->GetLocationScaled(camSimple);
			SimpleVector3<double> camMeters = dummyGlobe.GetPoint(camSpherical);
			SimpleVector3<double> objMeters(obj->location.x, obj->location.y, obj->location.z);
			float dist = camMeters.Distance(objMeters);
			
			bool shouldShow = isEnabled && (dist < group->showDistance);
			
			if (shouldShow && !obj->shown) {
				AGISPolyline* line = GetWorld()->SpawnActor<AGISPolyline>(AGISPolyline::StaticClass());
				line->DisplayObject(obj, group);
				line->PositionObject(globe);
				line->SetBrightness(1.0f);
				polylines[i] = line;
				obj->shown = true;
			} else if (!shouldShow && obj->shown) {
				if (polylines.count(i)) {
					polylines[i]->Destroy();
					polylines.erase(i);
				}
				obj->shown = false;
			} else if (shouldShow && obj->shown) {
				if (polylines.count(i)) {
					polylines[i]->PositionObject(globe);
				}
			}
		}
	}
}

void AWarningManager::FetchWarnings()
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> httpRequest = FHttpModule::Get().CreateRequest();
	httpRequest->SetURL("https://api.weather.gov/alerts/active");
	httpRequest->SetVerb("GET");
	httpRequest->SetHeader("User-Agent", "OpenStormVR (contact@openstorm.vr)");
	httpRequest->OnProcessRequestComplete().BindUObject(this, &AWarningManager::OnWarningsFetched);
	httpRequest->ProcessRequest();
}

void AWarningManager::OnWarningsFetched(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid()) return;
	
	FString jsonStr = Response->GetContentAsString();
	TSharedPtr<FJsonObject> jsonObject;
	TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(jsonStr);
	
	if (FJsonSerializer::Deserialize(reader, jsonObject) && jsonObject.IsValid()) {
		// Clear old warnings
		for (auto& pair : polylines) {
			pair.second->Destroy();
		}
		polylines.clear();
		for (GISObject* obj : warningObjects) {
			if (obj->geometry) free(obj->geometry);
			obj->Delete();
			delete obj;
		}
		warningObjects.clear();
		
		const TArray<TSharedPtr<FJsonValue>>* features;
		if (jsonObject->TryGetArrayField(TEXT("features"), features)) {
			for (int i = 0; i < features->Num(); i++) {
				TSharedPtr<FJsonObject> feature = (*features)[i]->AsObject();
				if (!feature.IsValid()) continue;
				
				TSharedPtr<FJsonObject> properties = feature->GetObjectField(TEXT("properties"));
				TSharedPtr<FJsonObject> geometry = feature->GetObjectField(TEXT("geometry"));
				if (!properties.IsValid() || !geometry.IsValid()) continue;
				
				FString eventName = properties->GetStringField(TEXT("event"));
				uint8_t groupId = 5;
				if (eventName == "Tornado Warning") groupId = 1;
				else if (eventName == "Severe Thunderstorm Warning") groupId = 2;
				else if (eventName == "Flash Flood Warning") groupId = 3;
				else if (eventName == "Special Marine Warning") groupId = 4;
				else continue; // Only care about severe warnings for now
				
				FString geomType = geometry->GetStringField(TEXT("type"));
				const TArray<TSharedPtr<FJsonValue>>* coordinates;
				if (geometry->TryGetArrayField(TEXT("coordinates"), coordinates)) {
					if (geomType == "Polygon" && coordinates->Num() > 0) {
						if ((*coordinates)[0]->Type == EJson::Array) {
							const TArray<TSharedPtr<FJsonValue>>& ring = (*coordinates)[0]->AsArray();
							GISObject* obj = new GISObject();
							obj->groupId = groupId;
							obj->shown = false;
							obj->geometryCount = ring.Num() * 2;
							obj->geometry = (float*)malloc(sizeof(float) * obj->geometryCount);
							
							float sumLat = 0;
							float sumLon = 0;
							
							for (int j = 0; j < ring.Num(); j++) {
								if (ring[j]->Type == EJson::Array) {
									const TArray<TSharedPtr<FJsonValue>>& pt = ring[j]->AsArray();
									if (pt.Num() >= 2) {
										float lon = pt[0]->AsNumber();
										float lat = pt[1]->AsNumber();
										obj->geometry[j*2] = lat;
										obj->geometry[j*2 + 1] = lon;
										sumLat += lat;
										sumLon += lon;
									}
								}
							}
							if (ring.Num() > 0) {
								sumLat /= ring.Num();
								sumLon /= ring.Num();
								Globe dummyGlobe = {};
								SimpleVector3<double> loc = dummyGlobe.GetPointDegrees(sumLat, sumLon, 0);
								obj->location = SimpleVector3<float>(loc.x, loc.y, loc.z);
							}
							
							warningObjects.push_back(obj);
						}
					}
				}
			}
			fprintf(stderr, "Fetched %d NWS warnings\n", (int)warningObjects.size());
		}
	} else {
		fprintf(stderr, "Failed to parse NWS warnings JSON\n");
	}
}

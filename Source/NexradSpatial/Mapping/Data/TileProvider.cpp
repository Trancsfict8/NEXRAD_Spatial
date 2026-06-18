#include "TileProvider.h"
#include "../../Radar/SystemAPI.h"
#include "Tar.h"
#include <cmath>
#include <stdio.h>
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

void Tile::SetCallback(std::function<void()> callback){
	if(isReady){
		callback();
	}else{
		readyCallback = callback;
	}
}

Tile::~Tile(){
	if(isAlive) {
		*isAlive = false;
	}
	if(data != NULL){
		delete[] data;
	}
}

TileProvider::TileProvider(std::string name, std::string url, std::string imageType, int maxZoom) {
	this->name = name;
	this->url = url;
	this->imageType = imageType;
	this->maxZoom = maxZoom;
}

Tile *TileProvider::GetTile(int zoom, int y, int x){
	Tile* tile = new Tile();
	tile->isAlive = std::make_shared<bool>(true);
	tile->tileProvider = this;
	if(zoom > maxZoom){
		return tile;
	}
	int numSize = std::to_string((int)pow(2, zoom)).length();
	std::string nameX = std::to_string(x);
	nameX = std::string(numSize - nameX.length(), '0') + nameX;
	std::string nameY = std::to_string(y);
	nameY = std::string(numSize - nameY.length(), '0') + nameY;
	std::string nameZ = std::to_string(zoom);
	nameZ = std::string(2 - nameZ.length(), '0') + nameZ;
	std::string extension;
	if(imageType == "image/jpeg"){
		extension = ".jpg";
	}else if(imageType == "image/png"){
		extension = ".png";
	}
	tile->fileName = "z" + nameZ + "y" + nameY + "x" + nameX + extension;
	if(staticCacheTar != NULL && staticCacheTar->valid){
		Tar::TarFile* tarFile = staticCacheTar->GetFile(tile->fileName);
		if(tarFile != NULL){
			tile->data = tarFile->data;
			tile->dataSize = tarFile->size;
			// moved to tile
			tarFile->data = NULL;
			delete tarFile;
			tile->isReady = true;
			return tile;
		}
	}
	if(staticCache != ""){
		std::string path = staticCache + tile->fileName;
		//fprintf(stderr, "%s", path.c_str());
		SystemAPI::FileStats stats = SystemAPI::GetFileStats(path);
		if(stats.exists){
			FILE* file = fopen(path.c_str(), "r");
			if(file != NULL){
				uint8_t* fileData = new uint8_t[stats.size];
				fread(fileData, 1, stats.size, file);
				tile->data = fileData;
				tile->dataSize = stats.size;
				fclose(file);
				tile->isReady = true;
				return tile;
			}
		}
	}
	if(dynamicCache != ""){
		std::string path = dynamicCache + tile->fileName;
		//fprintf(stderr, "%s", path.c_str());
		SystemAPI::FileStats stats = SystemAPI::GetFileStats(path);
		if(stats.exists){
			FILE* file = fopen(path.c_str(), "r");
			if(file != NULL){
				uint8_t* fileData = new uint8_t[stats.size];
				fread(fileData, 1, stats.size, file);
				tile->data = fileData;
				tile->dataSize = stats.size;
				fclose(file);
				tile->isReady = true;
				return tile;
			}
		}
	}
	// HTTP download via Unreal Engine
	FString fUrl(url.c_str());
	fUrl = fUrl.Replace(TEXT("{x}"), *FString::Printf(TEXT("%d"), x));
	fUrl = fUrl.Replace(TEXT("{y}"), *FString::Printf(TEXT("%d"), y));
	fUrl = fUrl.Replace(TEXT("{z}"), *FString::Printf(TEXT("%d"), zoom));

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb("GET");
	HttpRequest->SetURL(fUrl);
	HttpRequest->SetHeader("User-Agent", "OpenStormVR/1.0");

	std::shared_ptr<bool> alive = tile->isAlive;
	HttpRequest->OnProcessRequestComplete().BindLambda([tile, alive, this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) {
		if (!alive || !(*alive)) {
			// Tile was deleted before request completed
			return;
		}
		if (bWasSuccessful && Response.IsValid() && Response->GetResponseCode() == 200) {
			const TArray<uint8>& ResponseData = Response->GetContent();
			if (ResponseData.Num() > 0) {
				tile->dataSize = ResponseData.Num();
				tile->data = new uint8_t[tile->dataSize];
				FMemory::Memcpy(tile->data, ResponseData.GetData(), tile->dataSize);
				
				// Save to dynamic cache
				if(dynamicCache != ""){
					std::string path = dynamicCache + tile->fileName;
					FILE* file = fopen(path.c_str(), "wb");
					if(file != NULL){
						fwrite(tile->data, 1, tile->dataSize, file);
						fclose(file);
					}
				}
			}
		}
		tile->isReady = true;
		if (tile->readyCallback) {
			tile->readyCallback();
		}
	});

	HttpRequest->ProcessRequest();
	tile->isReady = false;
	return tile;
}

void TileProvider::SetCache(std::string staticCacheFolder, std::string dynamicCacheFolder) {
	staticCache = staticCacheFolder;
	if(staticCache != "" && staticCache.back() != '/' && staticCache.back() != '\\'){
		staticCache = staticCache + "/";
	}
	dynamicCache = dynamicCacheFolder;
	if(dynamicCache != "" && dynamicCache.back() != '/' && dynamicCache.back() != '\\'){
		dynamicCache = dynamicCache + "/";
	}
	if(dynamicCache != ""){
		SystemAPI::CreateDirectory(dynamicCache);
	}
	
}

void TileProvider::SetTarCache(std::string staticCacheTarFile) {
	Tar* tar = new Tar(staticCacheTarFile);
	if(staticCacheTar != NULL){
		delete staticCacheTar;
		staticCacheTar = NULL;
	}
	staticCacheTar = tar;
}

void TileProvider::EventLoop(){
}

TileProvider::~TileProvider(){
	if(staticCacheTar != NULL){
		delete staticCacheTar;
		staticCacheTar = NULL;
	}
}

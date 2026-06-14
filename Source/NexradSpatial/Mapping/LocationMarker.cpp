#include "LocationMarker.h"


#include "UObject/Object.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Camera/CameraComponent.h"
#include "../EngineHelpers/StringUtils.h"
#include "../Objects/RadarGameStateBase.h"

ALocationMarker::ALocationMarker()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	
	meshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sphere"));
	UStaticMesh* mesh = ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("StaticMesh'/Engine/BasicShapes/Sphere.Sphere'")).Object;
	meshMaterial = ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("Material'/Game/Materials/UnlitColor.UnlitColor'")).Object;
	meshComponent->SetStaticMesh(mesh);
	meshComponent->SetMaterial(0, meshMaterial);
	meshComponent->SetRelativeScale3D(FVector(0.05, 0.05, 0.05));
	
	textComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Text"));
	textMaterial = ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("Material'/Game/Materials/UnlitColorText.UnlitColorText'")).Object;
	textComponent->SetTextMaterial(textMaterial);
	textComponent->SetHorizontalAlignment(EHTA_Center);
	textComponent->SetWorldSize(20);
	textComponent->SetRelativeRotation(FRotator(0, 180, 0));
	
	
	collisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Text Collision"));
	collisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	collisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	collisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	collisionComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
	collisionComponent->SetActive(false);
	collisionComponent->SetBoxExtent(FVector(0, 0, 0));

	textComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	meshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	collisionComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

}


void ALocationMarker::BeginPlay() {
	PrimaryActorTick.bCanEverTick = true;
	Super::BeginPlay();
	//SetColor(FVector(0,1.0f,0));
}

void ALocationMarker::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
	auto camera = Cast<UCameraComponent>(GetWorld()->GetFirstPlayerController()->GetPawn()->GetComponentByClass(UCameraComponent::StaticClass()));
	if (camera != NULL) {
		FVector playerLocation = camera->GetComponentLocation();
		FVector selfLocation = GetActorLocation();
		FRotator rotation = FRotationMatrix::MakeFromX(selfLocation - playerLocation).Rotator();
		SetActorRotation(rotation);
	}
}

void ALocationMarker::SetText(std::string text) {
	textComponent->SetText(FText::FromString(StringUtils::STDStringToFString(text)));
}

void ALocationMarker::SetColor(FVector color) {
	// initialize material instances if not already done and the color is not white
	if(meshMaterialInstance == NULL && (color.X != 1 || color.Y != 1 || color.Z != 1)){
		meshMaterialInstance = UMaterialInstanceDynamic::Create(meshMaterial, this);
		textMaterialInstance = UMaterialInstanceDynamic::Create(textMaterial, this);
		meshComponent->SetMaterial(0, meshMaterialInstance);
		textComponent->SetTextMaterial(textMaterialInstance);
	}
	if(meshMaterialInstance != NULL){
		meshMaterialInstance->SetVectorParameterValue("Color", color);
		textMaterialInstance->SetVectorParameterValue("Color", color);
	}
	originalColor = color;
}

void ALocationMarker::EnableCollision() {
	FBoxSphereBounds textBounds = textComponent->GetLocalBounds();
	collisionComponent->SetBoxExtent(textBounds.BoxExtent + FVector(50.0f, 50.0f, 50.0f));
	collisionComponent->SetRelativeLocation(-textBounds.Origin);
	collisionComponent->SetActive(true);
}

void ALocationMarker::SetHovered(bool bHovered) {
	if (textComponent) {
		float scale = bHovered ? 1.5f : 1.0f;
		textComponent->SetRelativeScale3D(FVector(scale, scale, scale));
		if (textMaterialInstance) {
			if (bHovered) {
				textMaterialInstance->SetVectorParameterValue("Color", FVector(1.0f, 1.0f, 0.0f)); // Yellow highlight
			} else {
				textMaterialInstance->SetVectorParameterValue("Color", originalColor);
			}
		}
	}
}

void ALocationMarker::OnClick(){
	if(markerType == MarkerTypeRadarSite){
		ARadarGameStateBase* gameMode = GetWorld()->GetGameState<ARadarGameStateBase>();
		if(gameMode != NULL){
			GlobalState* globalState = &gameMode->globalState;
			UE_LOG(LogTemp, Warning, TEXT("Set radar site to %s"), *FString(data.c_str()));
			globalState->downloadSiteId = data;
			globalState->downloadData = true;
			globalState->pollData = true;
			
			// DO NOT trigger TeleportCamera anymore!
		}
	}
}
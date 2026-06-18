// Fill out your copyright notice in the Description page of Project Settings.

// ReSharper disable CppExpressionWithoutSideEffects
#include "Items/FlowPickupActor.h"

#include "GameplayMessageSubsystem.h"
#include "Inventory/FlowItemInstance.h"
#include "Inventory/FlowItemsHelperLibrary.h"
#include "Inventory/ItemDefinitionAsset.h"

AFlowPickupActor::AFlowPickupActor()
{
    PrimaryActorTick.bCanEverTick = true;
    
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(Mesh);
    
    FCollisionResponseContainer ResponseContainer;
    ResponseContainer.SetAllChannels(ECR_Ignore);
    ResponseContainer.SetResponse(ECC_Visibility, ECR_Block);
    ResponseContainer.SetResponse(ECC_WorldStatic, ECR_Block);
    ResponseContainer.SetResponse(ECC_WorldDynamic, ECR_Block);
    Mesh->SetCollisionResponseToChannels(ResponseContainer);
}

bool AFlowPickupActor::StartInteract_Implementation(AActor* QueryFromActor, const FHitResult& HitResult, const FGameplayTag& InteractTypeTag, FInteractCallbackInfo& InteractCallbackInfo)
{
	if (ItemInstance)
	{
		FItemPickupMessage Message;
		Message.Instigator = QueryFromActor;
		Message.ItemInstance = ItemInstance;
		Message.PickupActor = this;
		
		UGameplayMessageSubsystem* MessageSubsystem = GetWorld()->GetSubsystem<UGameplayMessageSubsystem>();
		if (!MessageSubsystem) return false;
		
		MessageSubsystem->BroadcastMessage(FlowGear_Items_Pickup, FInstancedStruct::Make(Message));
		
		InteractCallbackInfo.InteractCallbackTag = FlowGear_Items_Pickup;
		InteractCallbackInfo.InteractCallbackPayload = FInstancedStruct::Make(FItemActorPayload(this));
        
		return true;
	}
	return false;
}

void AFlowPickupActor::BeginPlay()
{
    Super::BeginPlay();
    
    if (!ItemInstance)
    {
       if (!ItemsTable) return;
    
       const FItemRow* ItemRow = ItemsTable->FindRow<FItemRow>(ItemInfo.RowHandle.RowName, nullptr);
       if (!ItemRow) return;
       
       FLoadSoftObjectPathAsyncDelegate LoadSoftObjectPathDelegate;
       LoadSoftObjectPathDelegate.BindUObject(this, &AFlowPickupActor::OnAssetDefinitionLoaded);
       ItemRow->ItemDefinitionAsset.LoadAsync(LoadSoftObjectPathDelegate);
    }
}

void AFlowPickupActor::ConstructItem(UFlowItemInstance* InItemInstance)
{
    Super::ConstructItem(InItemInstance);
    
    ItemInstance = InItemInstance;
    ItemInstance->Rename(nullptr, this);
    
    SetActorEnableCollision(true);
    
    if (!Mesh->GetStaticMesh())
    {
       if (UStaticMesh* StaticMesh = ItemInstance->GetDefaultPickupMesh().LoadSynchronous())
       {
          Mesh->SetStaticMesh(StaticMesh);
       }
    }
    
    if (Mesh->GetStaticMesh())
    {
       Mesh->SetSimulatePhysics(true);
       Mesh->OnComponentHit.AddDynamic(this, &AFlowPickupActor::OnComponentHit);
    }
}

void AFlowPickupActor::OnAssetDefinitionLoaded(const FSoftObjectPath& SoftObjectPath, UObject* Object)
{
    UItemDefinitionAsset* LoadedDefinitionAsset = Cast<UItemDefinitionAsset>(Object);
    if (!LoadedDefinitionAsset)
    {
       return;
    }
    
   
    ItemInstance = UFlowItemsHelperLibrary::CreateFlowItemInstanceFromDefinition(this, LoadedDefinitionAsset);
}

void AFlowPickupActor::OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    GetWorld()->GetTimerManager().SetTimer(DeactivatePhysicsHandle, this, &AFlowPickupActor::DisableMeshPhysics, 1.0f, false);
}

void AFlowPickupActor::DisableMeshPhysics() const
{
    if (Mesh)
    {
        Mesh->SetSimulatePhysics(false);
    }
}

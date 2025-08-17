// Fill out your copyright notice in the Description page of Project Settings.


#include "Managers/NiagaraManager.h"

#include "SkeletalMeshComponentBudgeted.h"
#include "Async/Async.h"
#include "Subsystem//NiagaraCrowdSubsystemSettings.h"
#include "GameFramework/Pawn.h"
#include "Interface/PawnInteractInterface.h"
#include "Pawns/MassAIPawn.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystem/NiagaraCrowdSubsystem.h"

FMassCrowdInstanceCustomData::FMassCrowdInstanceCustomData(const FMassCrowdPackedInstanceCustomData& PackedCustomData)
{
	constexpr float InvMax10Bit = 1.0f / 1023.0f;
	
	const uint32 PackedColor1 = reinterpret_cast<const uint32&>(PackedCustomData.PackedParamColorRGB);
	Color.R = static_cast<float>(PackedColor1 & 0x3FF) * InvMax10Bit;     
	Color.G = static_cast<float>((PackedColor1 >> 10) & 0x3FF) * InvMax10Bit; 
	Color.B = static_cast<float>((PackedColor1 >> 20) & 0x3FF) * InvMax10Bit; 
	
	const uint32 PackedColor2 = reinterpret_cast<const uint32&>(PackedCustomData.PackedParam2ColorRGB);
	Color2.R = static_cast<float>(PackedColor2 & 0x3FF) * InvMax10Bit;
	Color2.G = static_cast<float>((PackedColor2 >> 10) & 0x3FF) * InvMax10Bit;
	Color2.B = static_cast<float>((PackedColor2 >> 20) & 0x3FF) * InvMax10Bit;
	
	const uint32 PackedColor3 = reinterpret_cast<const uint32&>(PackedCustomData.PackedParam3ColorRGB);
	Color3.R = static_cast<float>(PackedColor3 & 0x3FF) * InvMax10Bit;
	Color3.G = static_cast<float>((PackedColor3 >> 10) & 0x3FF) * InvMax10Bit;
	Color3.B = static_cast<float>((PackedColor3 >> 20) & 0x3FF) * InvMax10Bit;
}

FMassCrowdInstanceCustomData FMassCrowdInstanceCustomData::MakeCrowdCustomData(const FLinearColor Color, const FLinearColor Color2, const FLinearColor Color3)
{
	FMassCrowdInstanceCustomData CustomData;

	CustomData.Color = Color;
	CustomData.Color2 = Color2;
	CustomData.Color3 = Color3;
	
	return MoveTemp(CustomData);
}

FMassCrowdPackedInstanceCustomData::FMassCrowdPackedInstanceCustomData(const FMassCrowdInstanceCustomData& UnpackedCustomData)
{
	auto PackColor = [](const FLinearColor& Color) -> uint32 
	{
		const uint32 R = static_cast<uint32>(FMath::Clamp(Color.R, 0.0f, 1.0f) * 1023.0f) & 0x3FF;
		const uint32 G = static_cast<uint32>(FMath::Clamp(Color.G, 0.0f, 1.0f) * 1023.0f) & 0x3FF;
		const uint32 B = static_cast<uint32>(FMath::Clamp(Color.B, 0.0f, 1.0f) * 1023.0f) & 0x3FF;
		return R | (G << 10) | (B << 20);
	};
	
	reinterpret_cast<uint32&>(PackedParamColorRGB)  = PackColor(UnpackedCustomData.Color);
	reinterpret_cast<uint32&>(PackedParam2ColorRGB) = PackColor(UnpackedCustomData.Color2);
	reinterpret_cast<uint32&>(PackedParam3ColorRGB) = PackColor(UnpackedCustomData.Color3);
}

// Sets default values
ANiagaraManager::ANiagaraManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>("NiagaraComponent");
	NiagaraComponent->SetupAttachment(GetRootComponent());
	NiagaraComponent->bAutoActivate = true;

	IDCount = 0;
	bSystemActivated = false;

	//Setup user settings
	if (const auto NiagaraCrowdSettings = GetDefault<UNiagaraCrowdSubsystemSettings>())
	{
		bEnableDebug = NiagaraCrowdSettings->bEnableDebug;
		
		LocationArrayName = NiagaraCrowdSettings->NiagaraLocationArrayParameterName;
		ForwardVectorArrayName = NiagaraCrowdSettings->NiagaraForwardVectorArrayParameterName;
		VisibilityArrayName = NiagaraCrowdSettings->NiagaraVisibilityArrayParameterName;
		VelocityArrayName = NiagaraCrowdSettings->NiagaraVelocityArrayParameterName;
		MeshIndexArrayName = NiagaraCrowdSettings->NiagaraMeshIndexesParameterName;
		ColorsArrayName = NiagaraCrowdSettings->NiagaraColorsArrayParameterName;
		OffsetArrayName = NiagaraCrowdSettings->NiagaraOffsetArrayParameterName;
		LerpSpeed = NiagaraCrowdSettings->LerpSpeed;
		MeshesCount = NiagaraCrowdSettings->NiagaraMeshesCount;
		LerpSpeedName = NiagaraCrowdSettings->LerpSpeedParameterName;
	}
}

// Called when the game starts or when spawned
void ANiagaraManager::BeginPlay()
{
	Super::BeginPlay();

	UNiagaraCrowdSubsystem* NiagaraSubsystem = GetWorld()->GetSubsystem<UNiagaraCrowdSubsystem>();
	NiagaraSubsystem->OnNiagaraManagerPrepared.Broadcast(this);
}

// Called every frame
void ANiagaraManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateParticlesInfoTranslated();
}

//Setup dependencies

//Setup niagara system asset
void ANiagaraManager::SetNiagaraSystem(UNiagaraSystem* System)
{
	NiagaraSystem = System;

	if (!NiagaraSystem) Destroy();

	if (!NiagaraComponent->GetAsset())
	{
		NiagaraComponent->SetAsset(NiagaraSystem);
		NiagaraComponent->SetCastShadow(true);
		NiagaraComponent->SetFloatParameter(LerpSpeedName, LerpSpeed);
	}
}

//Setup pawns map for control niagara particles
void ANiagaraManager::SetPawnInstance(const FPawnMeshData& MeshData)
{
	PawnList.Add(IDCount, MeshData);
	IDCount++;
	
	UpdateSystem();
}

//Update timer to activate when new pawn was added
void ANiagaraManager::UpdateSystem()
{
	//Clear timer when pawns continue spawning, activate system on last spawned pawn
	if (GetWorld()->GetTimerManager().IsTimerActive(UpdateSystemHandle)) GetWorld()->GetTimerManager().ClearTimer(UpdateSystemHandle);
	GetWorld()->GetTimerManager().SetTimer(UpdateSystemHandle, this, &ANiagaraManager::ActivateSystem, 5.0f, false);
}

//Activate system when last pawn was added
void ANiagaraManager::ActivateSystem()
{
	if (IsValid(NiagaraComponent))
	{
		SetupNiagaraMesh();
		NiagaraComponent->ReinitializeSystem();

		if (bEnableDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, "Activating NiagaraSystem");
		bSystemActivated = true;
	}
}

void ANiagaraManager::SetupNiagaraMesh()
{
	if (!IsValid(NiagaraComponent) || MeshesCount == 0) return;

	TArray<int32> MeshIDs;

	for (const TPair<int32, FPawnMeshData>& Pair : PawnList)
	{
		MeshIDs.Add(Pair.Value.MeshIndex);
		FMassCrowdPackedInstanceCustomData CustomData = FMassCrowdInstanceCustomData::MakeCrowdCustomData(Pair.Value.ParticleColor1, Pair.Value.ParticleColor2, Pair.Value.ParticleColor3);

		const FLinearColor PackedColor = FLinearColor(CustomData.PackedParamColorRGB, CustomData.PackedParam2ColorRGB,
													  CustomData.PackedParam3ColorRGB, 0);
		USkeletalMeshComponent* SKM = Pair.Value.Pawn->FindComponentByClass<USkeletalMeshComponent>();
		if (SKM)
		{
			SKM->SetCustomPrimitiveDataFloat(1, CustomData.PackedParamColorRGB);
			SKM->SetCustomPrimitiveDataFloat(2, CustomData.PackedParam2ColorRGB);
			SKM->SetCustomPrimitiveDataFloat(3, CustomData.PackedParam3ColorRGB);
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, "Setup Packed Custom Data");
		}
		SetNiagaraArrayColor(PackedColor);
		SetNiagaraArrayAnimOffset(Pair.Value.AnimOffset);
	}
	
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayInt32(NiagaraComponent, MeshIndexArrayName, MeshIDs);
}

void ANiagaraManager::UpdateParticlesInfoTranslated() const
{
	if (!IsValid(NiagaraComponent))
	{
		if (bEnableDebug) GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Green, "Not valid niagara");
		return;
	}
	
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this]()
	{
		if (IsValid(NiagaraComponent))
		{
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(NiagaraComponent, LocationArrayName, TranslatedLocations);
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(NiagaraComponent, ForwardVectorArrayName, TranslatedForwardVectors);
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(NiagaraComponent, VelocityArrayName, TranslatedVelocity);
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayInt32(NiagaraComponent, VisibilityArrayName, TranslatedVisibility);
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(NiagaraComponent, LerpSpeedName, TranslatedLerpSpeedArray);
		}
	});
}


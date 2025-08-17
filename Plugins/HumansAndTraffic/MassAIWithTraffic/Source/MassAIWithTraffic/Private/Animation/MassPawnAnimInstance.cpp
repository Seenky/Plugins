// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/MassPawnAnimInstance.h"
#include "KismetAnimationLibrary.h"
#include "Pawns/MassAIPawn.h"
#include "Async/Async.h"
#include "GameFramework/PawnMovementComponent.h"

//Initialize anim with references
void UMassPawnAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	if (AActor* Owner = GetOwningActor())
	{
		CachedCharacter = Cast<APawn>(Owner);
		if (IsValid(CachedCharacter))
		{
			//Get all references for animation
			MovementComponent = CachedCharacter->GetMovementComponent();
			
			SkeletalMeshComponent = Cast<USkeletalMeshComponent>
									(CachedCharacter.Get()->FindComponentByClass(USkeletalMeshComponent::StaticClass()));
		}
	}
}

void UMassPawnAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();
}

void UMassPawnAnimInstance::OnPrepared_Implementation()
{
	IPrepareEventsInterface::OnPrepared_Implementation();
	SetSafeBlendSpace();
}

void UMassPawnAnimInstance::SetupAnimOffset_Implementation(const float& Offset)
{
	IPrepareEventsInterface::SetupAnimOffset_Implementation(Offset);
	StartOffset = Offset;
}

//Thread safe update for anim
void UMassPawnAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (IsValid(CachedCharacter))
	{
		FScopeLock Lock(&MovementMutex);
		if (IsValid(MovementComponent))
		{
			Speed = MovementComponent->Velocity.Length();
			bIsWalk = Speed >= 0.3f;
			Direction = UKismetAnimationLibrary::CalculateDirection(MovementComponent->Velocity,CachedCharacter->GetActorRotation());
		}
	}
}

//Safe getter for speed
float UMassPawnAnimInstance::GetSafeSpeed() const
{
	FScopeLock Lock(&MovementMutex);
	return Speed;
}

//Safe setter for blendspace reference
void UMassPawnAnimInstance::SetSafeBlendSpace()
{
	FScopeLock Lock(&Animmutex);

	if (!SkeletalMeshComponent.IsValid()) return;

	USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset();
		
	if (IsValid(SkeletalMesh) && !CharacterStyles.IsEmpty())
	{
		TArray<TSoftObjectPtr<UBlendSpace>> LocalBlendSpaces = CharacterStyles.Find(SkeletalMesh)->WalkBlendSpaces;
		if (!LocalBlendSpaces.IsEmpty())
		{
			const int8 Rand = FMath::RandRange(0, LocalBlendSpaces.Num() - 1);
			WalkBlendSpace = LocalBlendSpaces[Rand].LoadSynchronous();
		}
	}
}





// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassActorSubsystem.h"
#include "MassNavigationFragments.h"
#include "MassProcessor.h"
#include "Subsystem/NiagaraCrowdSubsystem.h"
#include "Subsystem/NiagaraCrowdSubsystemSettings.h"
#include "MassCrowdNiagaraProcessor.generated.h"

UCLASS()
class MASSAIWITHTRAFFIC_API UMassCrowdNiagaraProcessor : public UMassProcessor
{
	GENERATED_BODY()
	
public:
	UMassCrowdNiagaraProcessor();

protected:
	virtual void ConfigureQueries() override;
	virtual void Initialize(UObject& Owner) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;

private:
	void CalculateVisibilityAndRepresentation(
		const UNiagaraCrowdSubsystemSettings* NiagaraSettings,
		const APlayerController* Controller,
		const FVector& EntityLocation,
		const FVector& PlayerLocation,
		const FRotator& PlayerRotation,
		FMassActorFragment& ActorInfo,
		FMassVisibilityFragment& VisibilityFragment,
		FCurrentRepresentationFragment& Representation,
		float& LerpSpeed) const;

	void SnapEntitiesToGround(
		FTransformFragment& TransformFragment,
		const FCurrentRepresentationFragment& Representation,
		const FMassActorFragment& ActorInfo,
		const UNiagaraCrowdSubsystemSettings* NiagaraSettings,
		FVector& EntityLocation,
		const float& DistToPlayer,
		const USceneComponent* AsComponent) const;

	void PromoteSpecialCases(
		const UNiagaraCrowdSubsystem* NiagaraSubsystem,
		const UNiagaraCrowdSubsystemSettings* NiagaraSettings,
		const FTransformFragment& TransformFragment,
		FCurrentRepresentationFragment& RepresentationFragment,
		float& LerpSpeed,
		const FVector& EntityLocation,
		const FRotator& PlayerRotation) const;

	void CheckVisibilityRepresentation(const FCurrentRepresentationFragment& Representation, int32& VisibilityIndex);

	void ChangeAvoidanceFragments(
		FMassShouldUseAvoidanceFragment& ShouldUseAvoidanceFragment,
		FTranslatorUsageFragment& TranslatorUsageFragment,
		const FCurrentRepresentationFragment& CurrentRepresentationFragment,
		const FMassActorFragment& ActorFragment);

	void DrawDebugInfo(
		const FCurrentRepresentationFragment& Representation,
		const FTransformFragment& TransformFragment,
		const FVector& EntityLocation,
		const USceneComponent* AsComponent) const;
	
	bool IsOccluded(
		const TArray<AActor*>& ActorsToIgnore,
		const FVector& EntityLocation,
		const FVector& PlayerLocation) const;
	
	bool IsInFOV(
		const APlayerController* Controller,
		const FVector& EntityLocation,
		const FVector& PlayerLocation,
		const FRotator& PlayerRotation) const;

};

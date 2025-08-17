// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZoneShapeComponent.h"
#include "Components/SplineComponent.h"
#include "ZoneGraphActorCopy.generated.h"

UCLASS(hidecategories = (Input))
class MASSAIWITHTRAFFIC_API AZoneGraphActorCopy : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AZoneGraphActorCopy(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual bool IsEditorOnly() const override { return true; }

	const UZoneShapeComponent* GetShape() const { return ZoneShapeComponent; }

#if WITH_EDITOR
	virtual bool CanChangeIsSpatiallyLoadedFlag() const override { return false; }
#endif

	UFUNCTION(BlueprintCallable)
	void CopySplinePoints(USplineComponent* SplineComponent);

	UPROPERTY(Category = Zone,VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UZoneShapeComponent> ZoneShapeComponent;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};

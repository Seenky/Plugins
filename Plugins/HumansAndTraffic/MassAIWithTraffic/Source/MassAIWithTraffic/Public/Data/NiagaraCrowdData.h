// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/UserDefinedStruct.h"
#include "NiagaraCrowdData.generated.h"

USTRUCT(BlueprintType)
struct FPawnMeshData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<APawn> Pawn;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MeshIndex;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 VisibilityIndex;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor ParticleColor1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor ParticleColor2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor ParticleColor3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AnimOffset;

	FPawnMeshData()
		: MeshIndex(0)
		, VisibilityIndex(0)
		, ParticleColor1(FColor::White)
		, ParticleColor2(FColor::White)
		, ParticleColor3(FColor::White)
		, AnimOffset(0.0f)
	{
	}
};

USTRUCT(BlueprintType)
struct FColorData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UStaticMesh> StaticMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ColorIndex;

	FColorData()
		: ColorIndex(0)
	{
	}
};

USTRUCT(BlueprintType)
struct FPallete
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Color0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Color1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Color2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Color3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Color4;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor NiagaraColor;

	FPallete()
		: Color0(FLinearColor::White),
		  Color1(FLinearColor::White),
		  Color2(FLinearColor::White),
		  Color3(FLinearColor::White),
		  Color4(FLinearColor::White),
		  NiagaraColor(FLinearColor::White)
	{
	}
};

USTRUCT(BlueprintType)
struct FColorsSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPallete> Colors;
};


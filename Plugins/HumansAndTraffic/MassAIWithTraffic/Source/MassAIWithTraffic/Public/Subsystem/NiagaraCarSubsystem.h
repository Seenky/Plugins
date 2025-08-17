// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NiagaraCarSubsystem.generated.h"


UCLASS()
class MASSAIWITHTRAFFIC_API UNiagaraCarSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UNiagaraCarSubsystem();
	virtual ~UNiagaraCarSubsystem() override;
};

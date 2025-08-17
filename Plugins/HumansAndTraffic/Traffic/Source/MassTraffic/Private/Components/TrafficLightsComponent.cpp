// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/TrafficLightsComponent.h"

UTrafficLightsComponent::UTrafficLightsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTrafficLightsComponent::BeginPlay()
{
	Super::BeginPlay();

	SetComponentTickEnabled(false);
}

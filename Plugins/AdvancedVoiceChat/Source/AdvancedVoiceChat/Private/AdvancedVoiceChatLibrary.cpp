// Fill out your copyright notice in the Description page of Project Settings.


#include "AdvancedVoiceChatLibrary.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

void UAdvancedVoiceChatLibrary::ClearVoicePackets(UObject* WorldContextObject)
{
	UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	
	if (!World) return;

	const IOnlineVoicePtr VoiceInterface = Online::GetVoiceInterface(World);

	if (!VoiceInterface) return;

	VoiceInterface->ClearVoicePackets();
}

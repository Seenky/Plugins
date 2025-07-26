// Fill out your copyright notice in the Description page of Project Settings.


#include "AdvancedSessionLibrary.h"

FName UAdvancedSessionLibrary::GetSessionNameAdvanced(FBlueprintSessionResult SearchResult)
{
	FString Name;
	SearchResult.OnlineResult.Session.SessionSettings.Settings.Find("CustomSessionName")->Data.GetValue(Name);
	
	if(Name.IsEmpty())
		return FName(TEXT("Custom Session"));
	
	return FName(*Name);
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "UCreateSessionCallbackProxyAdvanced.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Interfaces/OnlineSessionInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UCreateSessionCallbackProxyAdvanced)

struct FOnlineSubsystemCallHelper
{
public:
	FOnlineSubsystemCallHelper(const TCHAR* CallFunctionContext, UObject* WorldContextObject, FName SystemName = NAME_None)
		: OnlineSub(Online::GetSubsystem(GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull), SystemName))
		, FunctionContext(CallFunctionContext)
	{
		if (OnlineSub == nullptr)
		{
			FFrame::KismetExecutionMessage(*FString::Printf(TEXT("%s - Invalid or uninitialized OnlineSubsystem"), FunctionContext), ELogVerbosity::Warning);
		}
	}

	void QueryIDFromPlayerController(APlayerController* PlayerController)
	{
		UserID.Reset();

		APlayerState* PlayerState = nullptr;
		if (PlayerController != nullptr)
		{
			PlayerState = ToRawPtr(PlayerController->PlayerState);
		}

		if (PlayerState != nullptr)
		{
			UserID = PlayerState->GetUniqueId().GetUniqueNetId();
			if (!UserID.IsValid())
			{
				FFrame::KismetExecutionMessage(*FString::Printf(TEXT("%s - Cannot map local player to unique net ID"), FunctionContext), ELogVerbosity::Warning);
			}
		}
		else
		{
			FFrame::KismetExecutionMessage(*FString::Printf(TEXT("%s - Invalid player state"), FunctionContext), ELogVerbosity::Warning);
		}
	}

	bool IsValid() const
	{
		return UserID.IsValid() && (OnlineSub != nullptr);
	}

public:
	FUniqueNetIdPtr UserID;
	IOnlineSubsystem* const OnlineSub;
	const TCHAR* FunctionContext;
};

//////////////////////////////////////////////////////////////////////////
// UCreateSessionCallbackProxyAdvanced

UCreateSessionCallbackProxyAdvanced::UCreateSessionCallbackProxyAdvanced(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, CreateCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateCompleted))
	, StartCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartCompleted))
	, NumPublicConnections(1)
{
}

UCreateSessionCallbackProxyAdvanced* UCreateSessionCallbackProxyAdvanced::CreateSessionAdvanced(
	UObject* WorldContextObject,
	class APlayerController* PlayerController,
	FName SessionName,
	const int32 PublicConnections,
	const bool bUseLAN, const bool bUseLobbiesIfAvailable)
{
	UCreateSessionCallbackProxyAdvanced* Proxy = NewObject<UCreateSessionCallbackProxyAdvanced>();
	Proxy->PlayerControllerWeakPtr = PlayerController;
	Proxy->CustomSessionName = SessionName;
	Proxy->NumPublicConnections = PublicConnections;
	Proxy->bUseLAN = bUseLAN;
	Proxy->bUseLobbiesIfAvailable = bUseLobbiesIfAvailable;
	Proxy->WorldContextObject = WorldContextObject;
	return Proxy;
}

void UCreateSessionCallbackProxyAdvanced::Activate()
{
	FOnlineSubsystemCallHelper Helper(TEXT("CreateSession"), WorldContextObject);
	Helper.QueryIDFromPlayerController(PlayerControllerWeakPtr.Get());

	if (Helper.IsValid())
	{
		auto Sessions = Helper.OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			CreateCompleteDelegateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(CreateCompleteDelegate);

			FOnlineSessionSettings Settings;
			Settings.NumPublicConnections = NumPublicConnections;
			Settings.bShouldAdvertise = true;
			Settings.bAllowJoinInProgress = true;
			Settings.bIsLANMatch = bUseLAN;
			Settings.bUsesPresence = true;
			Settings.bAllowJoinViaPresence = true;
			Settings.bUseLobbiesIfAvailable = bUseLobbiesIfAvailable;
			Settings.Set(TEXT("CustomSessionName"), CustomSessionName.ToString(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

			Sessions->CreateSession(*Helper.UserID, NAME_GameSession, Settings);

			// OnCreateCompleted will get called, nothing more to do now
			return;
		}
		else
		{
			FFrame::KismetExecutionMessage(TEXT("Sessions not supported by Online Subsystem"), ELogVerbosity::Warning);
		}
	}

	// Fail immediately
	OnFailure.Broadcast();
}

void UCreateSessionCallbackProxyAdvanced::OnCreateCompleted(FName SessionName, bool bWasSuccessful)
{
	FOnlineSubsystemCallHelper Helper(TEXT("CreateSessionCallback"), WorldContextObject);
	Helper.QueryIDFromPlayerController(PlayerControllerWeakPtr.Get());

	if (Helper.IsValid())
	{
		auto Sessions = Helper.OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateCompleteDelegateHandle);
			
			if (bWasSuccessful)
			{
				StartCompleteDelegateHandle = Sessions->AddOnStartSessionCompleteDelegate_Handle(StartCompleteDelegate);
				Sessions->StartSession(NAME_GameSession);

				// OnStartCompleted will get called, nothing more to do now
				return;
			}
		}
	}

	if (!bWasSuccessful)
	{
		OnFailure.Broadcast();
	}
}

void UCreateSessionCallbackProxyAdvanced::OnStartCompleted(FName SessionName, bool bWasSuccessful)
{
	FOnlineSubsystemCallHelper Helper(TEXT("StartSessionCallback"), WorldContextObject);
	Helper.QueryIDFromPlayerController(PlayerControllerWeakPtr.Get());

	if (Helper.IsValid())
	{
		auto Sessions = Helper.OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnStartSessionCompleteDelegate_Handle(StartCompleteDelegateHandle);
		}
	}

	if (bWasSuccessful)
	{
		OnSuccess.Broadcast();
	}
	else
	{
		OnFailure.Broadcast();
	}
}
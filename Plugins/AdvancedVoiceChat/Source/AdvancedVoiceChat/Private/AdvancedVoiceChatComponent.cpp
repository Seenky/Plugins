// Fill out your copyright notice in the Description page of Project Settings.


// Sets default values for this component's properties

#include "AdvancedVoiceChatComponent.h"

#include "AdvancedVoiceChatLibrary.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY(AdvancedVoiceChat);

bool GBEnableVoiceDebug = false;
FAutoConsoleVariableRef CVarEnableVoiceDebug(
	TEXT("AdvancedVoice.EnableDebug"),    
	GBEnableVoiceDebug,                               
	TEXT("Enable debug for advanced voice system\n") 
	" 0: Disable debug\n"
	" 1: Enable debug");  

UAdvancedVoiceChatComponent::UAdvancedVoiceChatComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, VoiceMode(EVoiceMode::Always)
	, VoiceState(ECurrentVoiceState::NoVoiceState)
	, Player(nullptr)
	, VoiceDebugWidget(nullptr)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAdvancedVoiceChatComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UAdvancedVoiceChatComponent, Player);
	DOREPLIFETIME(UAdvancedVoiceChatComponent, VoiceState);
	DOREPLIFETIME(UAdvancedVoiceChatComponent, VoiceLevel);
}

void UAdvancedVoiceChatComponent::BeginPlay()
{
	Super::BeginPlay();

#if !UE_BUILD_SHIPPING
	BindCVars();
#endif
}

void UAdvancedVoiceChatComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GBEnableVoiceDebug)
	{
		Server_SetVoiceLevel(GetVoiceLevel());
	}
}

void UAdvancedVoiceChatComponent::Multicast_InitVoiceComponent_Implementation()
{
	ACharacter* PlayerCharacter = Cast<ACharacter>(GetOwner());

	if (!PlayerCharacter)
	{
		UE_LOG(AdvancedVoiceChat, Error,
			TEXT("%s is not a character, can't init advanced voice chat component"), *GetNameSafe(GetOwner()));
		DestroyComponent();
		return;
	}
	
	Player = PlayerCharacter;
	APlayerState* PS = PlayerCharacter->GetPlayerState();

	if (!PS)
	{
		UE_LOG(AdvancedVoiceChat, Error,
			TEXT("%s can't get player state, can't init advanced voice chat component"), *GetNameSafe(GetOwner()));
		return;
	}

	const IOnlineVoicePtr VoiceInterface = Online::GetVoiceInterface(GetWorld());
	if (!VoiceInterface)
	{
		UE_LOG(AdvancedVoiceChat, Error,
			TEXT("%s can't get voice interface, can't init advanced voice chat component"), *GetNameSafe(GetOwner()));
		return;
	}
	
	VoiceInterface->RemoveAllRemoteTalkers();
	RegisterWithPlayerState(PS);
	ChangeVoiceStateSettings();
	UVOIPStatics::SetMicThreshold(MicThreshold);

	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Voice chat component initialized"));
}

void UAdvancedVoiceChatComponent::Client_CloseVoiceLine_Implementation()
{
	APlayerState* PS = Player->GetPlayerState();
	if (!PS || PS->GetPlayerController()) return;
	
	if (VoiceState != ECurrentVoiceState::NoVoiceState)
	{
		EndVoiceSpeach();
		UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), "ToggleSpeaking 0", PS->GetPlayerController());
	}
	
	const IOnlineVoicePtr VoiceInterface = Online::GetVoiceInterface(GetWorld());
	if (!VoiceInterface) return;
	
	VoiceInterface->MuteRemoteTalker(0, *PS->GetUniqueId().GetUniqueNetId(), true);
	VoiceInterface->RemoveAllRemoteTalkers();
	UAdvancedVoiceChatLibrary::ClearVoicePackets(GetWorld());
}

void UAdvancedVoiceChatComponent::StartVoiceSpeach_Implementation(const ECurrentVoiceState SpeachVoiceState)
{
	const IOnlineVoicePtr VoiceInterface = Online::GetVoiceInterface(GetWorld());
	if (!VoiceInterface) return;
	
	Server_SetVoiceState(SpeachVoiceState);
	if (GetOwner()->HasAuthority())
		OnRep_VoiceState();
	VoiceInterface->StartNetworkedVoice(0);

	if (GBEnableVoiceDebug)
	{
		const APlayerState* PS = Player->GetPlayerState();
		if (!PS && !PS->GetPlayerController())
		{
			return;  
		}

		UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), "OSS.VoiceLoopback 1", PS->GetPlayerController());
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Emerald, "Debug Start Voice Speach"); 
	}
}

void UAdvancedVoiceChatComponent::EndVoiceSpeach_Implementation()
{
	const IOnlineVoicePtr VoiceInterface = Online::GetVoiceInterface(GetWorld());
	if (!VoiceInterface) return;

	VoiceInterface->StopNetworkedVoice(0);
	Server_SetVoiceState(ECurrentVoiceState::NoVoiceState);
	OnRep_VoiceState();

	if (GBEnableVoiceDebug)
	{
		const APlayerState* PS = Player->GetPlayerState();
		if (!PS || PS->GetPlayerController()) return;

		UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), "OSS.VoiceLoopback 0", PS->GetPlayerController());
	}
}

//Private
void UAdvancedVoiceChatComponent::ChangeVoiceStateSettings()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, "Replicated Voice State"); 
	switch (VoiceState)
	{
	case ECurrentVoiceState::NoVoiceState:
		{
			Settings.ComponentToAttachTo = nullptr;
			Settings.AttenuationSettings = nullptr;
			Settings.SourceEffectChain = nullptr;
			break;
		}
	case ECurrentVoiceState::Positional:
		{
			Settings.ComponentToAttachTo = Player->GetMesh();
			Settings.AttenuationSettings = Attenuation;
			Settings.SourceEffectChain = nullptr;
			break;
		}
	case ECurrentVoiceState::Radio:
		{
			Settings.ComponentToAttachTo = Player->GetMesh();
			Settings.AttenuationSettings = Attenuation;
			Settings.SourceEffectChain = SourceEffectChain;
			break;
		}
	default:
		break;
	}
}

#if !UE_BUILD_SHIPPING
void UAdvancedVoiceChatComponent::BindCVars()
{
	const FConsoleVariableDelegate CheckCvarValues = FConsoleVariableDelegate::CreateUObject(this, &UAdvancedVoiceChatComponent::FetchCVars);
	
	CVarEnableVoiceDebug->SetOnChangedCallback(CheckCvarValues);
}

void UAdvancedVoiceChatComponent::FetchCVars(IConsoleVariable* Var)
{
	if (Var->GetBool())
    {
        if (VoiceDebugWidget)
        {
            VoiceDebugWidget->RemoveFromParent();
            VoiceDebugWidget = nullptr;
        }

        APlayerController* Controller = nullptr;
		
        if (GetWorld()->GetNetMode() == NM_ListenServer)
        {
            Controller = GetWorld()->GetFirstPlayerController();
        }
        else
        {
            Controller = UGameplayStatics::GetPlayerController(this, 0);
        }
		
        if (Controller && Controller->IsLocalController())
        {
            const FString WidgetPath = "/AdvancedVoiceChat/Widget/W_VoiceDebug.W_VoiceDebug_C";
            const FSoftClassPath SoftCanvasPath(WidgetPath);
            const TSubclassOf<UUserWidget> WidgetClass = SoftCanvasPath.TryLoadClass<UUserWidget>();

            if (!WidgetClass) return;
            
            VoiceDebugWidget = CreateWidget<UVoiceDebugWidget>(Controller, WidgetClass);
            if (VoiceDebugWidget)
            {
                VoiceDebugWidget->SetVoiceComp(this);
                VoiceDebugWidget->AddToViewport();
                GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, 
                    FString::Printf(TEXT("Debug Voice Widget Created Locally for %s"), 
                    *Controller->GetName()));
            }
        }
        else
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, 
                "Controller not found or not local");
        }
    }
    else
    {
        if (VoiceDebugWidget)
        {
            VoiceDebugWidget->RemoveFromParent();
            VoiceDebugWidget = nullptr; 
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, 
                "Debug Voice Widget Destroyed");
        }
    }
}
#endif



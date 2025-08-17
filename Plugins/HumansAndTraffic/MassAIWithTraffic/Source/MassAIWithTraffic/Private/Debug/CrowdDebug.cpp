// Fill out your copyright notice in the Description page of Project Settings.


#include "Debug/CrowdDebug.h"

#if !UE_BUILD_SHIPPING
// CVars
int32 GDebugNiagaraCrowdVisibility = 0;
FAutoConsoleVariableRef CVarDebugNiagaraCrowd(
	TEXT("NiagaraCrowd.DebugVisibility"),
	GDebugNiagaraCrowdVisibility,
	TEXT("Niagara crowd debug visibility mode.\n")
	TEXT("0 = Off (default.)\n")
	TEXT("1 = Debug draw humans visibility"),
	ECVF_Cheat
	);
#endif

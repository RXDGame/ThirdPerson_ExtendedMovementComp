// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterAdventureGameMode.h"
#include "ShooterAdventureCharacter.h"
#include "UObject/ConstructorHelpers.h"

AShooterAdventureGameMode::AShooterAdventureGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonTemplate/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

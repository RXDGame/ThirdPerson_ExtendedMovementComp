// Fill out your copyright notice in the Description page of Project Settings.


#include "AdventureCameraManager.h"

#include "Components/CapsuleComponent.h"
#include "ShooterAdventure/ShooterAdventureCharacter.h"
#include "ShooterAdventure/Public/AdventureMovementComponent.h"

AAdventureCameraManager::AAdventureCameraManager()
{
}

void AAdventureCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	Super::UpdateViewTarget(OutVT, DeltaTime);

	if (AShooterAdventureCharacter* Character = Cast<AShooterAdventureCharacter>(GetOwningPlayerController()->GetPawn()))
	{
		UAdventureMovementComponent* MovementComponent = Character->GetAdventureMovementComponent();
		ACharacter* DefaultCharacterObj = Character->GetClass()->GetDefaultObject<ACharacter>();
		FVector TargetCrouchOffset = FVector(
			0,
			0,
			MovementComponent->GetCrouchedHalfHeight() - DefaultCharacterObj->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
			);

		FVector Offset = FMath::Lerp(FVector::ZeroVector, TargetCrouchOffset, FMath::Clamp(CrouchBlendTime/ CrouchBlendDuration, 0.f,1.f));
		
		if(MovementComponent->IsCrouching())
		{
			CrouchBlendTime = FMath::Clamp(CrouchBlendTime + DeltaTime, 0.f, CrouchBlendDuration);
			Offset -= TargetCrouchOffset;
		}
		else
		{			
			CrouchBlendTime = FMath::Clamp(CrouchBlendTime - DeltaTime, 0.f, CrouchBlendDuration);
		}

		if(MovementComponent->IsCustomMovementMode(CMOVE_Roll))
		{
			Offset = -TargetCrouchOffset;
			CrouchBlendTime = 0;
		}

		if(MovementComponent->IsMovingOnGround())
		{
			OutVT.POV.Location += Offset;
		}
	}
}

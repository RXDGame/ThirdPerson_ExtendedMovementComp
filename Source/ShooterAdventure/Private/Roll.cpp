// Fill out your copyright notice in the Description page of Project Settings.


#include "Roll.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"

void URoll::Setup(UCharacterMovementComponent* CharMovementComponent)
{
	Super::Setup(CharMovementComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(GetOwner()->InputComponent))
	{
		// Walking
		EnhancedInputComponent->BindAction(RollAction, ETriggerEvent::Started, this, &URoll::RollPressed);
		EnhancedInputComponent->BindAction(RollAction, ETriggerEvent::Completed, this, &URoll::RollCanceled);
	}
}

void URoll::Enter()
{
	bOrientRotationCached = MovementComponent->bOrientRotationToMovement;
	InputDirection = MovementComponent->GetLastInputVector();

	FRotator targetRotation = InputDirection.Rotation();
	GetOwner()->SetActorRotation(targetRotation);

	MovementComponent->MovementMode = EMovementMode::MOVE_Custom;
	MovementComponent->MaxCustomMovementSpeed = RollSpeed;

	TimeCounter = RollDuration;
}

void URoll::Update(float DeltaTime)
{
	if (MovementComponent->IsFalling())
	{
		MovementComponent->MovementMode = EMovementMode::MOVE_Falling;
		OnStopAbility.Broadcast();
		return;
	}

	TimeCounter -= DeltaTime;
	if (TimeCounter <= 0)
	{
		MovementComponent->MovementMode = EMovementMode::MOVE_Walking;
		OnStopAbility.Broadcast();
	}

	MovementComponent->UpdateBasedMovement(DeltaTime);
}

void URoll::Exit()
{
}

bool URoll::Ready()
{
	return bRollPressed && !MovementComponent->IsFalling();
}

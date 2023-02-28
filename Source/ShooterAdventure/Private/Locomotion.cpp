// Fill out your copyright notice in the Description page of Project Settings.


#include "Locomotion.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <EnhancedInputComponent.h>

ULocomotion::ULocomotion()
{
}

void ULocomotion::Setup(UCharacterMovementComponent* movementComponent)
{
	Super::Setup(movementComponent);
	BindActions();
}

void ULocomotion::Enter()
{
	MovementComponent->MaxWalkSpeed = JogSpeed;
}

void ULocomotion::Update(float DeltaTime)
{
	if (SprintPressed)
	{
		MovementComponent->MaxWalkSpeed = SprintSpeed;
	}
	else if (WalkingPressed)
	{
		MovementComponent->MaxWalkSpeed = WalkSpeed;
	}
	else
	{
		MovementComponent->MaxWalkSpeed = JogSpeed;
	}
}

void ULocomotion::Exit()
{
}

bool ULocomotion::Ready()
{
	return true;
}

void ULocomotion::BindActions()
{
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(GetOwner()->InputComponent))
	{
		// Walking
		EnhancedInputComponent->BindAction(WalkAction, ETriggerEvent::Triggered, this, &ULocomotion::StartWalk);
		EnhancedInputComponent->BindAction(WalkAction, ETriggerEvent::Completed, this, &ULocomotion::StopWalk);

		// Sprint
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Triggered, this, &ULocomotion::StartSprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ULocomotion::StopSprint);
	}
}

void ULocomotion::StartWalk()
{
	WalkingPressed = true;
}

void ULocomotion::StopWalk()
{
	WalkingPressed = false;
}

void ULocomotion::StartSprint()
{
	SprintPressed = true;
}

void ULocomotion::StopSprint()
{
	SprintPressed = false;
}



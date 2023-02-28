#include "Air.h"
// Fill out your copyright notice in the Description page of Project Settings.


#include "Air.h"
#include "GameFramework/CharacterMovementComponent.h"

void UAir::Enter()
{
	bOrientRotation = MovementComponent->bOrientRotationToMovement;
	MovementComponent->bOrientRotationToMovement = false;
}

void UAir::Update(float DeltaTime)
{
	if (!MovementComponent->IsFalling())
	{
		OnStopAbility.Broadcast();
	}
}

bool UAir::Ready()
{
	return MovementComponent->IsFalling();
}

void UAir::Exit()
{
	MovementComponent->bOrientRotationToMovement = bOrientRotation;
}

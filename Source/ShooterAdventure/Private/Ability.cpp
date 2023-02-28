// Fill out your copyright notice in the Description page of Project Settings.


#include "Ability.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values for this component's properties
UAbility::UAbility()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAbility::Setup(UCharacterMovementComponent* CharMovementComp)
{
	MovementComponent = CharMovementComp;
}

void UAbility::Enter()
{
}

void UAbility::Update(float DeltaTime)
{
}

void UAbility::Exit()
{
}

bool UAbility::Ready()
{
	return false;
}


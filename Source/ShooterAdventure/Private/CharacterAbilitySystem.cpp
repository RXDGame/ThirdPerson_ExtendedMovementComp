// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterAbilitySystem.h"
#include "Ability.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values for this component's properties
UCharacterAbilitySystem::UCharacterAbilitySystem()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UCharacterAbilitySystem::BeginPlay()
{
	Super::BeginPlay();
	FindAndSetupAbilities();
}

// Called every frame
void UCharacterAbilitySystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	CheckReadyAbilities();

	if (ActiveAbility != nullptr)
	{
		ActiveAbility->Update(DeltaTime);
	}
}

void UCharacterAbilitySystem::CheckReadyAbilities()
{
	UAbility* nextAbility = ActiveAbility;
	for (auto& Ability : Abilities)
	{
		if (!Ability->Ready())
		{
			continue;
		}

		if (nextAbility == nullptr)
		{
			nextAbility = Ability;
		}
		else
		{
			if (Ability->Priority > nextAbility->Priority)
			{
				nextAbility = Ability;
			}
		}
	}

	if (nextAbility != ActiveAbility)
	{
		SwitchAbility(nextAbility);
	}
}

void UCharacterAbilitySystem::SwitchAbility(UAbility* newAbility)
{
	if (ActiveAbility != nullptr)
	{
		StopActiveAbility();
	}

	ActiveAbility = newAbility;
	ActiveAbility->OnStopAbility.AddDynamic(this, &UCharacterAbilitySystem::StopActiveAbility);
	ActiveAbility->Enter();

	UE_LOG(LogEngine, Warning, TEXT("Changed ability to: %s"), *ActiveAbility->GetName());
}

void UCharacterAbilitySystem::StopActiveAbility()
{
	UE_LOG(LogEngine, Warning, TEXT("Active ability will be stopped: %s"), *ActiveAbility->GetName());

	ActiveAbility->Exit();
	ActiveAbility->OnStopAbility.RemoveDynamic(this, &UCharacterAbilitySystem::StopActiveAbility);
	ActiveAbility = nullptr;
}

void UCharacterAbilitySystem::FindAndSetupAbilities()
{
	TSet<UActorComponent*> components = GetOwner()->GetComponents();
	UCharacterMovementComponent* movementComponent = GetOwner()->FindComponentByClass<UCharacterMovementComponent>();
	for (auto& component : components)
	{
		if (component->IsA(UAbility::StaticClass()))
		{
			UAbility* ability = Cast<UAbility>(component);
			ability->Setup(movementComponent);
			Abilities.Add(ability);
		}
	}
}

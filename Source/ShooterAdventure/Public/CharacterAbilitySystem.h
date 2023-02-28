// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CharacterAbilitySystem.generated.h"

class UAbility;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), config = Game)
class SHOOTERADVENTURE_API UCharacterAbilitySystem : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCharacterAbilitySystem();

private:
	TArray<UAbility*> Abilities;
	UAbility* ActiveAbility;

	void FindAndSetupAbilities();
	void CheckReadyAbilities();
	void SwitchAbility(UAbility* newAbility);

	UFUNCTION()
	void StopActiveAbility();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};

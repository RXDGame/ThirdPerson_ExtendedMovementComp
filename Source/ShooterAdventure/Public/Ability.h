// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Ability.generated.h"

class UCharacterMovementComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAbilityAction);

UCLASS(Abstract, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SHOOTERADVENTURE_API UAbility : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAbility();

public:	

	virtual void Setup(UCharacterMovementComponent* CharMovementComp);

	virtual void Enter();
	virtual void Update(float DeltaTime);
	virtual void Exit();
	virtual bool Ready();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int Priority;

	UPROPERTY(BlueprintAssignable, Category = "Attributes")
	FAbilityAction OnStopAbility;

protected:
	UCharacterMovementComponent* MovementComponent;
};

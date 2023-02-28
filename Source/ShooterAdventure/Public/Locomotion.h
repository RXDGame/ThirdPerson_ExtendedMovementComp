// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Ability.h"
#include "Locomotion.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = (Abilities), meta = (BlueprintSpawnableComponent), config = Game)
class SHOOTERADVENTURE_API ULocomotion : public UAbility
{
	GENERATED_BODY()

	/** Walk Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* WalkAction;

	/** Sprint Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SprintAction;

public:
	// Sets default values for this component's properties
	ULocomotion();

	virtual void Setup(UCharacterMovementComponent* movementComponent) override;

	virtual void Enter() override;
	virtual void Update(float DeltaTime) override;
	virtual void Exit() override;
	virtual bool Ready() override;

	UPROPERTY(EditAnywhere)
	float WalkSpeed = 230.0;

	UPROPERTY(EditAnywhere)
	float JogSpeed = 500.0;

	UPROPERTY(EditAnywhere)
	float SprintSpeed = 900.0;

private:
	void BindActions();
	void StartWalk();
	void StopWalk();
	void StartSprint();
	void StopSprint();

	bool WalkingPressed;
	bool SprintPressed;
};

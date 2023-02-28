// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Ability.h"
#include "Roll.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = (Abilities), meta = (BlueprintSpawnableComponent))
class SHOOTERADVENTURE_API URoll : public UAbility
{
	GENERATED_BODY()
	
public:

	virtual void Setup(class UCharacterMovementComponent* CharMovementComponent) override;
	virtual void Enter() override;
	virtual void Update(float DeltaTime) override;
	virtual void Exit() override;
	virtual bool Ready() override;

	UPROPERTY(EditDefaultsOnly)
	float RollSpeed;

	UPROPERTY(EditDefaultsOnly)
	float RollDuration;

	/** Roll Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* RollAction;

private:
	bool bOrientRotationCached;
	bool bRollPressed;
	float TimeCounter;
	FVector InputDirection;

	FORCEINLINE void RollPressed() { bRollPressed = true; }
	FORCEINLINE void RollCanceled() { bRollPressed = false; }
};

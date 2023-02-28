// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Ability.h"
#include "Air.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = (Abilities), meta = (BlueprintSpawnableComponent))
class SHOOTERADVENTURE_API UAir : public UAbility
{
	GENERATED_BODY()
	
public:
	virtual void Enter() override;
	virtual void Update(float DeltaTime) override;
	virtual bool Ready() override;
	virtual void Exit() override;

private:
	bool bOrientRotation;
};

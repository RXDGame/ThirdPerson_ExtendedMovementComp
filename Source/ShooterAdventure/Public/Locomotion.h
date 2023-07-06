// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "State.h"
#include "Locomotion.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTERADVENTURE_API ULocomotion : public UState
{
	GENERATED_BODY()

public:
    void Enter() override;
    void Tick() override;
    void Exit() override;
};

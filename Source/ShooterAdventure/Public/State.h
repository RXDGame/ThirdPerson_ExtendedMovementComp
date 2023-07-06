// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StateMachine.h"
#include "State.generated.h"


UCLASS()
class SHOOTERADVENTURE_API UState : public UDataAsset
{
	GENERATED_BODY()

public:
	virtual void Setup(UStateMachine* stateMachine);
	virtual void Enter() PURE_VIRTUAL(UState::Enter,);
	virtual void Tick() PURE_VIRTUAL(UState::Tick,);
	virtual void Exit() PURE_VIRTUAL(UState::Exit,);

protected:
	UStateMachine* StateMachine;
};

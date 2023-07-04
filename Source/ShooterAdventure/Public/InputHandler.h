// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "InputHandler.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FJumpPressed);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SHOOTERADVENTURE_API UInputHandler : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInputHandler();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	UPROPERTY(BlueprintAssignable)
	FJumpPressed OnJumpPressed;

	void JumpPressed() const { OnJumpPressed.Broadcast();}
};

// Fill out your copyright notice in the Description page of Project Settings.


#include "StateMachine.h"

#include "State.h"
#include "Locomotion.h"

// Sets default values for this component's properties
UStateMachine::UStateMachine()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UStateMachine::BeginPlay()
{
	Super::BeginPlay();

	// ...
	for (UState* state : States)
	{
		state->Setup(this);
	}

	SetState(ULocomotion::StaticClass());
}


// Called every frame
void UStateMachine::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UStateMachine::SetState(const UClass* state)
{
	for (UState* myState : States)
	{
		if(myState->GetClass() == state)
		{
			if(CurrentState != nullptr)
			{
				CurrentState->Exit();
				UE_LOG(LogTemp, Warning, TEXT("Current State Exit: %s"), *CurrentState->GetClass()->GetName());
			}

			CurrentState = myState;
			CurrentState->Enter();
			UE_LOG(LogTemp, Warning, TEXT("New State Enter: %s"), *CurrentState->GetClass()->GetName());
			return;
		}
	}
}


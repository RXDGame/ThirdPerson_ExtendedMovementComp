// Fill out your copyright notice in the Description page of Project Settings.


#include "ClimbingComponent.h"

#include "Kismet/KismetSystemLibrary.h"

// Sets default values for this component's properties
UClimbingComponent::UClimbingComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UClimbingComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
}

FHitResult UClimbingComponent::GetForwardHit() const
{
	FHitResult Hit;

	const FVector StartTrace = GetOwner()->GetActorLocation() + TraceOrigin;
	const FVector EndTrace = StartTrace + GetOwner()->GetActorForwardVector() * MaxTraceDistance;
	UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(), StartTrace, EndTrace, CapsuleTraceRadius, MaxTraceHeight, TraceChannel, true,
		TArray<AActor*>(), EDrawDebugTrace::ForOneFrame, Hit, true);

	return Hit;
}

FHitResult UClimbingComponent::GetTopHit(FHitResult forwardHit) const
{
	FHitResult TopHit;
	FCollisionQueryParams Params;

	const FVector OwnerForward = GetOwner()->GetActorForwardVector();
	const FVector StartTrace = GetOwner()->GetActorLocation() + TraceOrigin + FVector::UpVector * MaxTraceHeight;
	const float step = MaxTopTraceDepth / TopTraceIterations;
	
	for (int i=0; i< TopTraceIterations; i++)
	{
		FVector TraceStartIteration = StartTrace + OwnerForward * (i * step);
		FVector EndTrace = StartTrace + FVector::DownVector * MaxTraceHeight * 2.f;
		TArray<FHitResult> Hits;

		if(GetWorld()->LineTraceMultiByChannel(Hits, TraceStartIteration, EndTrace, static_cast<ECollisionChannel>(TraceChannel.GetValue())))
		{
			for (auto Hit : Hits)
			{
				if(Hit.GetActor() == forwardHit.GetActor())
				{
					return Hit;
				}
			}
		}
	}
	
	return TopHit;
}


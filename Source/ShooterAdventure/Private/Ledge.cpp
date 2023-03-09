// Fill out your copyright notice in the Description page of Project Settings.


#include "Ledge.h"

// Sets default values
ALedge::ALedge()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void ALedge::BeginPlay()
{
	Super::BeginPlay();
	
}

USceneComponent* ALedge::GetClosestPoint(FVector Origin) const
{
	USceneComponent* ClosestPoint = nullptr;
	float ClosestDistance = 100000000000.0f;
	for (USceneComponent* GrabPoint : GrabPoints)
	{
		if(ClosestPoint == nullptr)
		{
			ClosestPoint = GrabPoint;
		}
		else
		{
			const float distance = FVector::Distance(Origin, GrabPoint->GetComponentLocation());
			if(distance < ClosestDistance)
			{
				ClosestDistance = distance;
				ClosestPoint = GrabPoint;
			}
		}
	}

	return ClosestPoint;
}


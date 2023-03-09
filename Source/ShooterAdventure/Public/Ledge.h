// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Ledge.generated.h"

UCLASS()
class SHOOTERADVENTURE_API ALedge : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ALedge();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	UPROPERTY(BlueprintReadWrite) TArray<USceneComponent*> GrabPoints;

	USceneComponent* GetClosestPoint(FVector Origin) const;
};

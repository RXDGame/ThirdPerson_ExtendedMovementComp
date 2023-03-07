// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ClimbingComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SHOOTERADVENTURE_API UClimbingComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UClimbingComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditDefaultsOnly) TEnumAsByte<ETraceTypeQuery> TraceChannel;
	UPROPERTY(EditDefaultsOnly) FVector TraceOrigin;	
	UPROPERTY(EditDefaultsOnly) float CapsuleTraceRadius;
	UPROPERTY(EditDefaultsOnly) float MaxTraceDistance;
	UPROPERTY(EditDefaultsOnly) float MaxTraceHeight;
	UPROPERTY(EditDefaultsOnly) int TopTraceIterations;
	UPROPERTY(EditDefaultsOnly) float MaxTopTraceDepth;
	UPROPERTY(EditDefaultsOnly) float MinAllowedDepthToClimbUp;
	UPROPERTY(EditDefaultsOnly, Category=Character) FVector OffsetFromLedge;
	
public:
	FHitResult GetForwardHit() const;
	FHitResult GetTopHit(FHitResult forwardHit) const;
	bool FoundLedge(FHitResult &FwdHit, FHitResult &TopHit) const;
	FVector GetCharacterLocationOnLedge(FHitResult FwdHit, FHitResult TopHit) const;
	FRotator GetCharacterRotationOnLedge(FHitResult FwdHit);
};

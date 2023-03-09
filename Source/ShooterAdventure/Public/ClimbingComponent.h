// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ClimbingComponent.generated.h"


class UCapsuleComponent;
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
	FVector GetTraceOrigin() const;
	
	UPROPERTY(EditDefaultsOnly) TEnumAsByte<ETraceTypeQuery> TraceChannel;
	UPROPERTY(EditDefaultsOnly) FVector TraceOrigin;	
	UPROPERTY(EditDefaultsOnly) float CapsuleTraceRadius;
	UPROPERTY(EditDefaultsOnly) float CapsuleTraceHeight;
	UPROPERTY(EditDefaultsOnly) float MaxTraceDistance;
	UPROPERTY(EditDefaultsOnly) float MaxTraceHeight;
	UPROPERTY(EditDefaultsOnly) int TopTraceIterations;
	UPROPERTY(EditDefaultsOnly) float MaxTopTraceDepth;
	UPROPERTY(EditDefaultsOnly) float MinAllowedDepthToClimbUp;
	UPROPERTY(EditDefaultsOnly, Category=Character) FVector OffsetFromLedge;
	UPROPERTY(EditDefaultsOnly, Category=Character) FVector ClimbUpOffset;
	UPROPERTY(EditDefaultsOnly, Category=SideCasting) int SideIterations;
	UPROPERTY(EditDefaultsOnly, Category=SideCasting) float MinSideDistance;
	UPROPERTY(EditDefaultsOnly) float MaxRangeToFindLedge;
	UPROPERTY(EditDefaultsOnly) float MaxJumpSpeed;

	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	bool IsPossibleToReach(const USceneComponent* Candidate, FVector& TossVelocity, float Gravity) const;
	
public:
	UPROPERTY(EditAnywhere, Category=Debugging) bool DebugTrace;
	
	FHitResult GetForwardHit() const;
	FHitResult GetTopHit(FHitResult forwardHit) const;
	bool FoundLedge(FHitResult &FwdHit, FHitResult &TopHit) const;
	FVector GetCharacterLocationOnLedge(FHitResult FwdHit, FHitResult TopHit) const;
	FRotator GetCharacterRotationOnLedge(FHitResult FwdHit);

	TArray<USceneComponent*> GetReachableGrabPoints() const;
	bool GetValidLaunchVelocity(TArray<USceneComponent*> GrabPoints,FVector& LaunchVelocity, float Gravity) const;
	bool CanClimbUp(FVector& TargetClimbLocation) const;
	bool CanMoveInDirection(float HorizontalDirection, AActor* CurrentLedgeActor, FVector& TargetEdgeLocation) const;
};

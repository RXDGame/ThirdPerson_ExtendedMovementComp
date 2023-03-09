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
	UPROPERTY(EditDefaultsOnly) FVector TraceOrigin = FVector(0,0,60);	
	UPROPERTY(EditDefaultsOnly) float CapsuleTraceRadius = 30;
	UPROPERTY(EditDefaultsOnly) float CapsuleTraceHeight = 50;
	UPROPERTY(EditDefaultsOnly) float MaxTraceDistance = 100;
	UPROPERTY(EditDefaultsOnly) float MaxTraceHeight = 50;
	UPROPERTY(EditDefaultsOnly) int TopTraceIterations = 20;
	UPROPERTY(EditDefaultsOnly) float MaxTopTraceDepth = 100;
	UPROPERTY(EditDefaultsOnly) float MinAllowedDepthToClimbUp = 70;
	UPROPERTY(EditDefaultsOnly, Category=Character) float ForwardOffsetFromLedge = 47.0f;
	UPROPERTY(EditDefaultsOnly, Category=Character) float VerticalOffsetFromLedge = 52.0f;
	UPROPERTY(EditDefaultsOnly, Category=Character) FVector ClimbUpOffset;
	UPROPERTY(EditDefaultsOnly, Category=SideCasting) int SideIterations;
	UPROPERTY(EditDefaultsOnly, Category=SideCasting) float MinSideDistance;
	UPROPERTY(EditDefaultsOnly, Category=Corner) float CornerOutDepth;
	UPROPERTY(EditDefaultsOnly, Category=Corner) float CornerInDepth;
	UPROPERTY(EditDefaultsOnly) float MaxRangeToFindLedge = 700;
	UPROPERTY(EditDefaultsOnly) float MaxJumpSpeed = 1000;

	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	bool IsPossibleToReach(const USceneComponent* Candidate, FVector& TossVelocity, float Gravity) const;
	
public:
	UPROPERTY(EditAnywhere, Category=Debugging) bool DebugTrace;
	
	FHitResult GetForwardHit() const;
	FHitResult GetTopHit(FHitResult forwardHit, FVector TraceDirection, FVector TraceStartOrigin) const;
	bool FoundLedge(FHitResult &FwdHit, FHitResult &TopHit) const;
	FVector GetCharacterLocationOnLedge(FHitResult FwdHit, FHitResult TopHit) const;
	FRotator GetCharacterRotationOnLedge(FHitResult FwdHit) const;

	TArray<USceneComponent*> GetReachableGrabPoints(FVector MoveDirection) const;
	bool GetValidLaunchVelocity(TArray<USceneComponent*> GrabPoints,FVector& LaunchVelocity, float Gravity) const;
	bool CanClimbUp(FVector& TargetClimbLocation) const;
	bool CanMoveInDirection(float HorizontalDirection, AActor* CurrentLedgeActor, FVector& TargetEdgeLocation) const;
	bool CanCornerOut(float MoveDirection, FVector& CornerLocation, FRotator& CornerRotation) const;
};

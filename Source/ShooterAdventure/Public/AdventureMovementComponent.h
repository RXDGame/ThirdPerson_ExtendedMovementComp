// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AdventureMovementComponent.generated.h"

class UClimbingComponent;
class AShooterAdventureCharacter;

UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_None		UMETA(Hidden),
	CMOVE_Slide		UMETA(DisplayName = "Slide"),
	CMOVE_Roll		UMETA(DisplayName = "Roll"),
	CMOVE_Climbing	UMETA(DisplayName = "Climbing"),
	CMOVE_Max		UMETA(Hidden),
};

/**
 * 
 */
UCLASS()
class SHOOTERADVENTURE_API UAdventureMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
#pragma region Custom Saved Move Class
	class FSavedMove_Adventure : public FSavedMove_Character
	{		
	public:
		enum CompressedFlags
		{
			FLAG_Sprint			= 0x10,
			FLAG_Custom_1		= 0x20,
			FLAG_Custom_2		= 0x40,
			FLAG_Custom_3		= 0x80,			
		};
		
		//Flag
		uint8 Saved_bWantsToSprint : 1;

		// Not flag
		uint8 Saved_bPreviousWantstoCrouch:1;
		uint8 Saved_bWantstoRoll:1;
		
		virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
		virtual void Clear() override;
		virtual uint8 GetCompressedFlags() const override; 
		virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData) override;
		virtual void PrepMoveFor(ACharacter* C) override;
	};

	class FNetworkPredictionData_Client_Adventure : public FNetworkPredictionData_Client_Character
	{
	public:
		FNetworkPredictionData_Client_Adventure(const UCharacterMovementComponent& ClientMovement);

		typedef FNetworkPredictionData_Client_Character Super;

		virtual FSavedMovePtr AllocateNewMove() override;
	};
#pragma endregion
	
	// Network and Saved Move Methods
protected:
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
public:
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	
private:
	// Transient
	UPROPERTY(Transient) AShooterAdventureCharacter* AdventureCharacterOwner;
	
	// Safe Variables
	bool Safe_bWantsToSprint;
	bool Safe_bPreviousWantsToCrouch;
	bool Safe_bWantsToRoll;	

public:
	UAdventureMovementComponent();
	virtual void InitializeComponent() override;
	
// SPRINT
private:
	UPROPERTY(EditDefaultsOnly, Category=Sprint) float MaxSprintSpeed;

// SLIDE
private:
	UPROPERTY(EditDefaultsOnly, Category=Slide) float MaxSlideSpeed = 300.f;
	UPROPERTY(EditDefaultsOnly, Category=Slide) float Slide_EnterImpulse = 500.f;
	UPROPERTY(EditDefaultsOnly, Category=Slide) float Slide_GravityForce = 5000.f;
	UPROPERTY(EditDefaultsOnly, Category=Slide) float Slide_Friction = 1.3f;	
	UPROPERTY(EditDefaultsOnly, Category=Slide) float BrakingDecelerationSliding = 500.f;
	
	void EnterSlide();
	void ExitSlide();
	void PhysSlide(float deltaTime, int32 Iterations);
	bool GetSlideSurface(FHitResult& Hit) const;

// ROLL
public:
	void TryEnterRoll(){ Safe_bWantsToRoll = true;}
	void EnterRoll(EMovementMode PreviousMovementMode, ECustomMovementMode PreviousCustomMode);

// CLIMBING
private:
	UPROPERTY(EditDefaultsOnly, Category=Climbing) float InterpSpeed = 100.f;
	
	bool bIsInterpolating;
	FVector TargetInterpolateLocation;
	FRotator TargetInterpolateRotation;
	TObjectPtr<UClimbingComponent> ClimbingComponent;
	
	void PhysClimbing(float deltaTime, int32 Iterations);
	
public:
	UPROPERTY(BlueprintReadOnly, Category=Climbing) float HorizontalDirection;
	void TryClimb(FVector InitialLocation, FRotator InitialRotation);

private:
	UPROPERTY(EditDefaultsOnly, Category=Roll) float RollTimeDuration = 1.3f;
	UPROPERTY(EditDefaultsOnly, Category=Roll) float RollDelayBetweenRolls = 0.25f;
	UPROPERTY(EditDefaultsOnly, Category=Roll) float MaxRollSpeed = 600.f;
	UPROPERTY(EditDefaultsOnly, Category=Roll) float BrakingDecelerationRolling = 2000.f;
	UPROPERTY(EditDefaultsOnly, Category=Roll) bool bCanWalkOffLedgeWhenRolling = true;
	
	float RollTime;
	FVector RollDirection;
	bool bHasRollRecently;
	FTimerHandle RollResetTimerHandle;
	
	void ExitRoll();
	void PhysRoll(float deltaTime, int32 Iterations);
	bool CanRoll() const;
	void ResetRoll();

	UFUNCTION(Server, Reliable) void Server_EnterRoll();

public:
	virtual bool IsMovingOnGround() const override;
	virtual bool CanCrouchInCurrentState() const override;
	virtual bool CanWalkOffLedges() const override;
	virtual float GetMaxSpeed() const override;
	virtual float GetMaxBrakingDeceleration() const override;
	virtual void PhysicsRotation(float DeltaTime) override;

protected:
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;	
	virtual void UpdateCharacterStateAfterMovement(float DeltaSeconds) override;
	
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

public:
	UFUNCTION(BlueprintCallable) void Sprint();
	UFUNCTION(BlueprintCallable) void StopSprint();
	UFUNCTION(BlueprintCallable) void ToggleCrouch();

	UFUNCTION(BlueprintCallable) bool IsCustomMovementMode(ECustomMovementMode InCustomMovementMode) const;
};

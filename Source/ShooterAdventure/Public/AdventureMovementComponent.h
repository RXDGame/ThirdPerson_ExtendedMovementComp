// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AdventureMovementComponent.generated.h"

class AShooterAdventureCharacter;

UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_None		UMETA(Hidden),
	CMOVE_Slide		UMETA(DisplayName = "Slide"),
	CMOVE_Roll		UMETA(DisplayName = "Roll"),
	CMOVE_Max		UMETA(Hidden),
};

/**
 * 
 */
UCLASS()
class SHOOTERADVENTURE_API UAdventureMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

	class FSavedMove_Adventure : public FSavedMove_Character
	{
		typedef FSavedMove_Character Super;
		
		//Flag
		uint8 Saved_bWantsToSprint : 1;

		// Not flag
		uint8 Saved_bPreviousWantstoCrouch:1;
		
	public:
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

	UPROPERTY(EditDefaultsOnly) float Sprint_MaxWalkSpeed;
	UPROPERTY(EditDefaultsOnly) float Walk_MaxWalkSpeed;

	UPROPERTY(EditDefaultsOnly, Category=Slide) float Slide_MinSpeed = 300.f;
	UPROPERTY(EditDefaultsOnly, Category=Slide) float Slide_EnterImpulse = 500.f;
	UPROPERTY(EditDefaultsOnly, Category=Slide) float Slide_GravityForce = 5000.f;
	UPROPERTY(EditDefaultsOnly, Category=Slide) float Slide_Friction = 1.3f;	
	UPROPERTY(EditDefaultsOnly, Category=Slide) float Slide_MinAngleToSlide = 60.f;

	// Transient
	UPROPERTY(Transient) AShooterAdventureCharacter* AdventureCharacterOwner;
	
	bool Safe_bWantsToSprint;
	bool Safe_bPreviousWantsToCrouch;
	bool bInSlide;

public:
	UAdventureMovementComponent();

	virtual void InitializeComponent() override;
	void EnterRoll();
	
private:
	void EnterSlide();
	void ExitSlide();
	void PhysSlide(float deltaTime, int32 Iterations);
	bool GetSlideSurface(FHitResult& Hit) const;

	// Roll
	UPROPERTY(EditDefaultsOnly, Category=Roll) float RollTimeDuration = 1.3f;
	UPROPERTY(EditDefaultsOnly, Category=Roll) float RollSpeed = 600.f;
	UPROPERTY(EditDefaultsOnly, Category=Roll) float RollGravityForce = 8000.f;
	float RollTime;
	FVector RollDirection;
	
	void ExitRoll();
	void PhysRoll(float deltaTime, int32 Iterations);

public:
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual bool IsMovingOnGround() const override;
	virtual bool CanCrouchInCurrentState() const override;

protected:
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;
	
	virtual void UpdateCharacterStateAfterMovement(float DeltaSeconds) override;
	virtual  void PhysCustom(float deltaTime, int32 Iterations) override;

public:
	UFUNCTION(BlueprintCallable) void Sprint();
	UFUNCTION(BlueprintCallable) void StopSprint();
	UFUNCTION(BlueprintCallable) void ToggleCrouch();

	UFUNCTION(BlueprintCallable) bool IsCustomMovementMode(ECustomMovementMode InCustomMovementMode) const;
};

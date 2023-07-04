// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "InputHandler.h"
#include "ShooterAdventureCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FClimbingDelegate);

UENUM(BlueprintType)
enum EClimbingState
{
	CLIMB_NONE,
	CLIMB_INTERPOLATING,
	CLIMB_HANGING,
	CLIMB_LAUNCHING,
	CLIMB_WARPING,
	CLIMB_LEAVING
};

class UClimbingComponent;
UCLASS(config=Game)
class AShooterAdventureCharacter : public ACharacter
{
	GENERATED_BODY()

private:

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	/** Sprint Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SprintAction;
	
	/** Crouch Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* CrouchAction;
	
	/** Roll Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* RollAction;
	
	/** Climb Up Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* ClimbUpAction;
	
	/** Drop Climb Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* DropClimbAction;	

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Movement)
	class UAdventureMovementComponent* AdventureMovementComponent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Climbing)
	UClimbingComponent* ClimbingComponent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Climbing)
	UInputHandler* InputHandler;
	
public:
	AShooterAdventureCharacter(const FObjectInitializer& ObjectInitializer);
		
protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);
	
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// To add mapping context
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaSeconds) override;
	
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	/** Return Adventure movement component **/
	FORCEINLINE class UAdventureMovementComponent* GetAdventureMovementComponent() const { return AdventureMovementComponent; }
	
	FCollisionQueryParams GetIgnoreCharacterParam() const;
	
private:
	void ClimbingUpdate(float DeltaTime);
	UFUNCTION() void ResetLedge();
	AActor* CurrentLedge;

	UPROPERTY(EditDefaultsOnly, Category=Climbing) float InterpSpeed = 15.f;	
	FVector TargetInterpolateLocation;
	FRotator TargetInterpolateRotation;
	FTimerHandle WarpTimerHandle;
	EClimbingState ClimbingState;
	
	void LaunchToLedge();
	void StartClimb(FVector InitialLocation, FRotator InitialRotation);
	void InterpolateToTarget(FVector Location, FRotator Rotation);
	void DoClimbJump();
	void DropClimb();
	void StopShimmy();
	bool TryCornerOut(float Direction);
	void ClimbUp();
	void JumpUp();
	void JumpSide(float HorDirection);

	void SetClimbingTimer(float Duration, EClimbingState TimerState);
	void FinishClimbingTimer();
	void ProccessInterpolation(float DeltaTime);
	
public:			
	UPROPERTY(BlueprintReadOnly, Category=Climbing)
	bool bCanShimmy = true;
	
	UPROPERTY(BlueprintReadOnly, Category=Climbing)
	float HorizontalDirection;
	
	UPROPERTY(BlueprintReadOnly, Category=Climbing)
	FVector MotionWarpLocation;
	
	UPROPERTY(BlueprintReadOnly, Category=Climbing)
	FRotator MotionWarpRotation;	
		
	UPROPERTY(BlueprintAssignable)
	FClimbingDelegate OnClimbUp;
	
	UPROPERTY(BlueprintAssignable)
	FClimbingDelegate OnCornerStart;
	
	UPROPERTY(BlueprintAssignable)
	FClimbingDelegate OnClimbJumpStart;
	
	UFUNCTION(BlueprintCallable)
	void ExitClimbing();

	bool IsClimbingState(EClimbingState State) const {return  ClimbingState == State;}
	void UpdateClimbingMovement();
};


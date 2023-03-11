// Fill out your copyright notice in the Description page of Project Settings.


#include "AdventureMovementComponent.h"

#include "ClimbingComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "ShooterAdventure/ShooterAdventureCharacter.h"


#pragma region Saved Variables - Server-Client

bool UAdventureMovementComponent::FSavedMove_Adventure::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
	FSavedMove_Adventure* NewSaveMove = static_cast<FSavedMove_Adventure*>(NewMove.Get());

	if (Saved_bWantsToSprint != NewSaveMove->Saved_bWantsToSprint)
	{
		return false;
	}

	return FSavedMove_Character::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void UAdventureMovementComponent::FSavedMove_Adventure::Clear()
{
	FSavedMove_Character::Clear();
	Saved_bWantsToSprint = 0;
	Saved_bWantstoRoll = 0;
	Saved_bPreviousWantstoCrouch = 0;
}

uint8 UAdventureMovementComponent::FSavedMove_Adventure::GetCompressedFlags() const
{
	uint8 Result = FSavedMove_Character::GetCompressedFlags();

	if (Saved_bWantsToSprint) Result |= FLAG_Sprint;

	return Result;
}

void UAdventureMovementComponent::FSavedMove_Adventure::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	FSavedMove_Character::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	const UAdventureMovementComponent* CharacterMovement = Cast<UAdventureMovementComponent>(C->GetCharacterMovement());
	Saved_bWantsToSprint = CharacterMovement->Safe_bWantsToSprint;
	Saved_bPreviousWantstoCrouch = CharacterMovement->Safe_bPreviousWantsToCrouch;
	Saved_bWantstoRoll = CharacterMovement->Safe_bWantsToRoll;
}

void UAdventureMovementComponent::FSavedMove_Adventure::PrepMoveFor(ACharacter* C)
{
	FSavedMove_Character::PrepMoveFor(C);

	UAdventureMovementComponent* CharacterMovement = Cast<UAdventureMovementComponent>(C->GetCharacterMovement());
	CharacterMovement->Safe_bWantsToSprint = Saved_bWantsToSprint;
	CharacterMovement->Safe_bPreviousWantsToCrouch = Saved_bPreviousWantstoCrouch;
	CharacterMovement->Safe_bWantsToRoll = Saved_bWantstoRoll;
}

UAdventureMovementComponent::FNetworkPredictionData_Client_Adventure::FNetworkPredictionData_Client_Adventure(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{

}

FSavedMovePtr UAdventureMovementComponent::FNetworkPredictionData_Client_Adventure::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_Adventure());
}


void UAdventureMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	Safe_bWantsToSprint = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
}

FNetworkPredictionData_Client* UAdventureMovementComponent::GetPredictionData_Client() const
{
	check(PawnOwner != nullptr);

	if (ClientPredictionData == nullptr)
	{
		UAdventureMovementComponent* MutableThis = const_cast<UAdventureMovementComponent*>(this);

		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Adventure(*this);
		MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.f;
		MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.f;

	}

	return ClientPredictionData;
}

#pragma endregion 

UAdventureMovementComponent::UAdventureMovementComponent()
{
	NavAgentProps.bCanCrouch = true;
}

void UAdventureMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();
	AdventureCharacterOwner = Cast<AShooterAdventureCharacter>(GetOwner());
	ClimbingComponent = AdventureCharacterOwner->FindComponentByClass<UClimbingComponent>();
}

bool UAdventureMovementComponent::DoJump(bool bReplayingMoves)
{
	if(ClimbingComponent != nullptr && !IsCustomMovementMode(CMOVE_Climbing))
	{
		LaunchToLedge();
	}
	
	return Super::DoJump(bReplayingMoves);
}

#pragma region Slide

void UAdventureMovementComponent::EnterSlide()
{
	bWantsToCrouch = true;
	bCrouchMaintainsBaseLocation = true;
	
	Velocity += Velocity.GetSafeNormal2D() * Slide_EnterImpulse;
	SetMovementMode(MOVE_Custom, CMOVE_Slide);
}

void UAdventureMovementComponent::ExitSlide()
{
	bWantsToCrouch = false;

	const FQuat NewRotation = FRotationMatrix::MakeFromXZ(UpdatedComponent->GetForwardVector().GetSafeNormal2D(),
															FVector::UpVector).ToQuat();
	FHitResult Hit;
	SafeMoveUpdatedComponent(FVector::ZeroVector, NewRotation, true, Hit);
	SetMovementMode(MOVE_Walking);
}

void UAdventureMovementComponent::PhysSlide(float deltaTime, int32 Iterations)
{
	if(deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	RestorePreAdditiveRootMotionVelocity();

	FHitResult SurfaceHit;
	if(!GetSlideSurface(SurfaceHit))
	{
		ExitSlide();
		StartNewPhysics(deltaTime, Iterations);
		return;
	}

	// Surface gravity
	Velocity += Slide_GravityForce * deltaTime * FVector::DownVector;

	// Strafe
	// check if player has pressed horizontal input enough to make character moves right or left
	// Acceleration is like the world input vector
	if(FMath::Abs(FVector::DotProduct(Acceleration.GetSafeNormal(), UpdatedComponent->GetRightVector())) > 0.5f)
	{
		Acceleration = Acceleration.ProjectOnTo(UpdatedComponent->GetRightVector());
	}
	else
	{
		Acceleration = FVector::ZeroVector;
	}

	if(!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		CalcVelocity(deltaTime, Slide_Friction, false, GetMaxBrakingDeceleration());
	}
	ApplyRootMotionToVelocity(deltaTime);

	// Perform Move
	Iterations++;
	bJustTeleported = false;

	// cache old transform
	FVector OldLocation = UpdatedComponent->GetComponentLocation();
	FQuat OldRotation = UpdatedComponent->GetComponentRotation().Quaternion();

	FHitResult Hit(1.0f);
	FVector AdjustedLocation = Velocity * deltaTime;
	FVector VelPlaneDir = FVector::VectorPlaneProject(Velocity, SurfaceHit.Normal).GetSafeNormal();
	FQuat NewRotation = FRotationMatrix::MakeFromXZ(VelPlaneDir, SurfaceHit.Normal).ToQuat();

	SafeMoveUpdatedComponent(AdjustedLocation, NewRotation, true, Hit);

	if(Hit.Time < 1.0f)
	{
		HandleImpact(Hit, deltaTime, AdjustedLocation);
		SlideAlongSurface(AdjustedLocation, (1.f - Hit.Time), Hit.Normal,Hit, true);
	}

	FHitResult NewSurfaceHit;
	if(!GetSlideSurface(NewSurfaceHit))
	{
		ExitSlide();
	}

	// Update outgoing velocity and acceleration
	if(!bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime; // v = dx / dt
	}
}

bool UAdventureMovementComponent::GetSlideSurface(FHitResult& Hit) const
{
	const FVector Start = UpdatedComponent->GetComponentLocation();
	const FVector End = Start + AdventureCharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 3.f * FVector::DownVector;
	const FName ProfileName = TEXT("NoCollision");

	return false;//GetWorld()->LineTraceSingleByProfile(Hit, Start, End, ProfileName, AdventureCharacterOwner->GetIgnoreCharacterParam());
}

#pragma endregion

#pragma region Roll
void UAdventureMovementComponent::EnterRoll(EMovementMode PreviousMovementMode, ECustomMovementMode PreviousCustomMode)
{
	bWantsToCrouch = true;
	bCrouchMaintainsBaseLocation = true;
	
	RollDirection = Acceleration.GetSafeNormal2D().IsNearlyZero() ? CharacterOwner->GetActorForwardVector() : Acceleration.GetSafeNormal2D();
	RollTime = RollTimeDuration;

	FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, true, nullptr);
}

void UAdventureMovementComponent::ExitRoll()
{
	bWantsToCrouch = false;
	bHasRollRecently = true;

	const FQuat NewRotation = FRotationMatrix::MakeFromXZ(UpdatedComponent->GetForwardVector().GetSafeNormal2D(),
															FVector::UpVector).ToQuat();
	FHitResult Hit;
	SafeMoveUpdatedComponent(FVector::ZeroVector, NewRotation, true, Hit);

	GetWorld()->GetTimerManager().SetTimer(RollResetTimerHandle, this, &UAdventureMovementComponent::ResetRoll, RollDelayBetweenRolls);
}

void UAdventureMovementComponent::PhysRoll(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	RollTime -= deltaTime;
	if(RollTime <= 0)
	{
		SetMovementMode(MOVE_Walking);
		return;
	}

	if (!CharacterOwner || (!CharacterOwner->Controller && !bRunPhysicsWithNoController && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)))
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}

	bJustTeleported = false;
	bool bCheckedFall = false;
	bool bTriedLedgeMove = false;
	float remainingTime = deltaTime;

	// Perform the move
	while ( (remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations) && CharacterOwner && (CharacterOwner->Controller || bRunPhysicsWithNoController || HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity() || (CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)) )
	{
		Iterations++;
		bJustTeleported = false;
		const float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
		remainingTime -= timeTick;

		// Save current values
		UPrimitiveComponent * const OldBase = GetMovementBase();
		const FVector PreviousBaseLocation = (OldBase != nullptr) ? OldBase->GetComponentLocation() : FVector::ZeroVector;
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		const FFindFloorResult OldFloor = CurrentFloor;

		// Ensure velocity is horizontal.
		MaintainHorizontalGroundVelocity();
		const FVector OldVelocity = Velocity;
		Acceleration.Z = 0.f;

		// Apply acceleration
		if( !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() )
		{
			CalcVelocity(timeTick, GroundFriction, false, GetMaxBrakingDeceleration());
		}
		
		ApplyRootMotionToVelocity(timeTick);

		if( IsFalling() )
		{
			// Root motion could have put us into Falling.
			// No movement has taken place this movement tick so we pass on full time/past iteration count
			StartNewPhysics(remainingTime+timeTick, Iterations-1);
			return;
		}

		// Compute move parameters
		const FVector MoveVelocity = RollDirection * MaxRollSpeed;
		const FVector Delta = timeTick * MoveVelocity;
		const bool bZeroDelta = Delta.IsNearlyZero();
		FStepDownResult StepDownResult;

		if ( bZeroDelta )
		{
			remainingTime = 0.f;
		}
		else
		{
			// try to move forward
			MoveAlongFloor(MoveVelocity, timeTick, &StepDownResult);

			if ( IsFalling() )
			{
				// pawn decided to jump up
				const float DesiredDist = Delta.Size();
				if (DesiredDist > UE_KINDA_SMALL_NUMBER)
				{
					const float ActualDist = (UpdatedComponent->GetComponentLocation() - OldLocation).Size2D();
					remainingTime += timeTick * (1.f - FMath::Min(1.f,ActualDist/DesiredDist));
				}
				
				StartNewPhysics(remainingTime,Iterations);
				return;
			}
			
			if ( IsSwimming() ) //just entered water
			{
				StartSwimming(OldLocation, OldVelocity, timeTick, remainingTime, Iterations);
				return;
			}
		}

		// Update floor.
		// StepUp might have already done it for us.
		if (StepDownResult.bComputedFloor)
		{
			CurrentFloor = StepDownResult.FloorResult;
		}
		else
		{
			FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, bZeroDelta, NULL);
		}

		// check for ledges here
		const bool bCheckLedges = !CanWalkOffLedges();
		if ( bCheckLedges && !CurrentFloor.IsWalkableFloor() )
		{
			// calculate possible alternate movement
			const FVector GravDir = FVector(0.f,0.f,-1.f);
			const FVector NewDelta = bTriedLedgeMove ? FVector::ZeroVector : GetLedgeMove(OldLocation, Delta, GravDir);
			if ( !NewDelta.IsZero() )
			{
				// first revert this move
				RevertMove(OldLocation, OldBase, PreviousBaseLocation, OldFloor, false);

				// avoid repeated ledge moves if the first one fails
				bTriedLedgeMove = true;

				// Try new movement direction
				Velocity = NewDelta/timeTick;
				remainingTime += timeTick;
				continue;
			}
			else
			{
				// see if it is OK to jump
				// @todo collision : only thing that can be problem is that oldbase has world collision on
				bool bMustJump = bZeroDelta || (OldBase == NULL || (!OldBase->IsQueryCollisionEnabled() && MovementBaseUtility::IsDynamicBase(OldBase)));
				if ( (bMustJump || !bCheckedFall) && CheckFall(OldFloor, CurrentFloor.HitResult, Delta, OldLocation, remainingTime, timeTick, Iterations, bMustJump) )
				{
					return;
				}
				bCheckedFall = true;

				// revert this move
				RevertMove(OldLocation, OldBase, PreviousBaseLocation, OldFloor, true);
				remainingTime = 0.f;
				break;
			}
		}
		else
		{
			// Validate the floor check
			if (CurrentFloor.IsWalkableFloor())
			{
				AdjustFloorHeight();
				SetBase(CurrentFloor.HitResult.Component.Get(), CurrentFloor.HitResult.BoneName);
			}
			else if (CurrentFloor.HitResult.bStartPenetrating && remainingTime <= 0.f)
			{
				// The floor check failed because it started in penetration
				// We do not want to try to move downward because the downward sweep failed, rather we'd like to try to pop out of the floor.
				FHitResult Hit(CurrentFloor.HitResult);
				Hit.TraceEnd = Hit.TraceStart + FVector(0.f, 0.f, MAX_FLOOR_DIST);
				const FVector RequestedAdjustment = GetPenetrationAdjustment(Hit);
				ResolvePenetration(RequestedAdjustment, Hit, UpdatedComponent->GetComponentQuat());
				bForceNextFloorCheck = true;
			}

			// check if just entered water
			if ( IsSwimming() )
			{
				StartSwimming(OldLocation, Velocity, timeTick, remainingTime, Iterations);
				return;
			}

			// See if we need to start falling.
			if (!CurrentFloor.IsWalkableFloor() && !CurrentFloor.HitResult.bStartPenetrating)
			{
				const bool bMustJump = bJustTeleported || bZeroDelta || (OldBase == NULL || (!OldBase->IsQueryCollisionEnabled() && MovementBaseUtility::IsDynamicBase(OldBase)));
				if ((bMustJump || !bCheckedFall) && CheckFall(OldFloor, CurrentFloor.HitResult, Delta, OldLocation, remainingTime, timeTick, Iterations, bMustJump) )
				{
					return;
				}
				bCheckedFall = true;
			}
		}


		// Allow overlap events and such to change physics state and velocity
		if (IsMovingOnGround())
		{
			// Make velocity reflect actual move
			if( !bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && timeTick >= MIN_TICK_TIME)
			{
				// TODO-RootMotionSource: Allow this to happen during partial override Velocity, but only set allowed axes?
				Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / timeTick;
				MaintainHorizontalGroundVelocity();
			}
		}

		// If we didn't move at all this iteration then abort (since future iterations will also be stuck).
		if (UpdatedComponent->GetComponentLocation() == OldLocation)
		{
			remainingTime = 0.f;
			break;
		}	
	}

	if (IsMovingOnGround())
	{
		MaintainHorizontalGroundVelocity();
	}
	
	FHitResult Hit;
	FQuat NewRotation = FRotationMatrix::MakeFromXZ(RollDirection, FVector::UpVector).ToQuat();
	SafeMoveUpdatedComponent(FVector::ZeroVector, NewRotation, false, Hit);
}

bool UAdventureMovementComponent::CanRoll() const
{
	return !bHasRollRecently && MovementMode == MOVE_Walking;
}

void UAdventureMovementComponent::ResetRoll()
{
	bHasRollRecently = false;
}

void UAdventureMovementComponent::Server_EnterRoll_Implementation()
{
	Safe_bWantsToRoll = true;
}

#pragma endregion

#pragma region Climbing

void UAdventureMovementComponent::StopShimmy()
{
	HorizontalDirection = 0;
	CurrentRootMotion.Clear();
	Velocity = FVector::ZeroVector;
	bCanShimmy = false;
}

void UAdventureMovementComponent::PhysClimbing(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}	

	FVector OldLocation = UpdatedComponent->GetComponentLocation();
	if(bIsInterpolating)
	{
		FHitResult Hit;
		if(UpdatedComponent->GetComponentLocation().Equals(TargetInterpolateLocation, 0.001f))
		{			
			SafeMoveUpdatedComponent(TargetInterpolateLocation - UpdatedComponent->GetComponentLocation(), TargetInterpolateRotation.Quaternion(), false, Hit);
			bIsInterpolating = false;
			return;
		}
		
		FVector Location = FMath::VInterpTo(UpdatedComponent->GetComponentLocation(), TargetInterpolateLocation, deltaTime, InterpSpeed);
		FRotator Rotation = FMath::RInterpTo(UpdatedComponent->GetComponentRotation(), TargetInterpolateRotation, deltaTime, InterpSpeed);

		SafeMoveUpdatedComponent(Location - UpdatedComponent->GetComponentLocation(), Rotation.Quaternion(), false, Hit);
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime;
		HorizontalDirection = 0;
		return;
	}

	RestorePreAdditiveRootMotionVelocity();
	
	if( !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() )
	{
		if( bCheatFlying && Acceleration.IsZero() )
		{
			Velocity = FVector::ZeroVector;
		}
		const float Friction = 0.5f;
		CalcVelocity(deltaTime, Friction, true, GetMaxBrakingDeceleration());
	}
	
		
	ApplyRootMotionToVelocity(deltaTime);

	Iterations++;
	bJustTeleported = false;

	OldLocation = UpdatedComponent->GetComponentLocation();
	const FVector Adjusted = Velocity * deltaTime;
	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent(Adjusted, UpdatedComponent->GetComponentQuat(), true, Hit);

	if (Hit.Time < 1.f)
	{
		const FVector GravDir = FVector(0.f, 0.f, -1.f);
		const FVector VelDir = Velocity.GetSafeNormal();
		const float UpDown = GravDir | VelDir;

		bool bSteppedUp = false;
		if ((FMath::Abs(Hit.ImpactNormal.Z) < 0.2f) && (UpDown < 0.5f) && (UpDown > -0.2f) && CanStepUp(Hit))
		{
			float stepZ = UpdatedComponent->GetComponentLocation().Z;
			bSteppedUp = StepUp(GravDir, Adjusted * (1.f - Hit.Time), Hit);
			if (bSteppedUp)
			{
				OldLocation.Z = UpdatedComponent->GetComponentLocation().Z + (OldLocation.Z - stepZ);
			}
		}

		if (!bSteppedUp)
		{
			//adjust and try again
			HandleImpact(Hit, deltaTime, Adjusted);
			SlideAlongSurface(Adjusted, (1.f - Hit.Time), Hit.Normal, Hit, true);
		}
	}

	if( !bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() )
	{
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime;
	}
	
	if(bLeavingClimbing || bInMotionWarping)
	{
		return;
	}
	
	UpdateClimbingAfterMovement();
	/*FString Message = FString::Printf(TEXT("Horizontal Direction: %s"), *FString::SanitizeFloat(HorizontalDirection));
	GEngine->AddOnScreenDebugMessage(1, 0, FColor::Blue, Message);*/
}

void UAdventureMovementComponent::UpdateClimbingAfterMovement()
{
	FHitResult FwdHit, TopHit;
	if(ClimbingComponent->FoundLedge(FwdHit, TopHit))
	{
		CurrentLedge = FwdHit.GetActor();
		FVector Location = ClimbingComponent->GetCharacterLocationOnLedge(FwdHit, TopHit);
		FRotator Rotation = ClimbingComponent->GetCharacterRotationOnLedge(FwdHit);
			
		FVector Delta = Location - UpdatedComponent->GetComponentLocation();
		FHitResult MoveHit;
		SafeMoveUpdatedComponent(Delta, Rotation.Quaternion(), false, MoveHit);
		
		if(Acceleration.SizeSquared() <= 10.f)
		{
			HorizontalDirection = 0;
			return;
		}
		
		float targetDirection = FVector::DotProduct(Acceleration.GetSafeNormal2D(), CharacterOwner->GetActorRightVector());
		FVector TargetLocationOnEdge;
		if(ClimbingComponent->CanMoveInDirection(targetDirection, TopHit.GetActor(), TargetLocationOnEdge))
		{
			HorizontalDirection = targetDirection;
			bCanShimmy = true;
		}
		else
		{
			// check to corner out or in
			if(TryCornerOut(targetDirection))
			{
				return;
			}				
				
			StopShimmy();
		}
	}
}


void UAdventureMovementComponent::LaunchToLedge()
{
	FVector LaunchVelocity;
	if(ClimbingComponent->GetValidLaunchVelocity(ClimbingComponent->GetReachableGrabPoints(Acceleration.GetSafeNormal2D()), LaunchVelocity, GetGravityZ()))
	{
		CharacterOwner->LaunchCharacter(LaunchVelocity, true, true);
		const FQuat Rotation = FRotationMatrix::MakeFromXZ(LaunchVelocity.GetSafeNormal2D(), FVector::UpVector).ToQuat();
		FHitResult Hit;
		SafeMoveUpdatedComponent(FVector::ZeroVector, Rotation, false, Hit);
	}
}

void UAdventureMovementComponent::FinishWarping()
{
	bInMotionWarping = false;

	if(bLeavingClimbing)
	{
		ExitClimbing();
	}
}

bool UAdventureMovementComponent::TryCornerOut(float Direction)
{
	if(ClimbingComponent->CanCornerOut(Direction, MotionWarpLocation, MotionWarpRotation))
	{
		bInMotionWarping = true;
		OnCornerStart.Broadcast();					
					
		UAnimMontage* MontageToPlay = Direction > 0 ? RightCornerOutMontage : LeftCornerOutMontage;
		const float Duration = CharacterOwner->PlayAnimMontage(MontageToPlay);
		SetMotionWarpingTimer(Duration);
		return true;
	}

	return false;
}

void UAdventureMovementComponent::ClimbUp()
{
	// climb up
	bLeavingClimbing = true;
	bCanShimmy = true;
		
	OnClimbUp.Broadcast();

	const float Duration = CharacterOwner->PlayAnimMontage(ClimbUpMontage);
	SetMotionWarpingTimer(Duration - 0.1f);
}

void UAdventureMovementComponent::JumpUp()
{
	if(ClimbingComponent->CanHopUp(MotionWarpLocation))
	{
		bInMotionWarping = true;
		
		OnClimbJumpStart.Broadcast();
		const float MontageDuration = CharacterOwner->PlayAnimMontage(HopUpMontage);
		SetMotionWarpingTimer(MontageDuration);		
	}
	else
	{
		const FVector LaunchVelocity = ClimbingComponent->GetJumpUpVelocity();
		CharacterOwner->LaunchCharacter(LaunchVelocity, true, true);
	}
}

void UAdventureMovementComponent::JumpSide(float HorDirection)
{
	const bool bIsRight = HorDirection > 0;
	const FVector Direction = GetOwner()->GetActorRightVector() * (bIsRight ? 1 : -1);

	
	if(ClimbingComponent->FoundSideLedge(CurrentLedge, Direction, MotionWarpLocation))
	{
		bInMotionWarping = true;
		MotionWarpRotation = CharacterOwner->GetActorRotation();
		OnClimbJumpStart.Broadcast();
	}
	else
	{
		bLeavingClimbing = true;
		bCanShimmy = true; 
	}

	UAnimMontage* Montage = bIsRight ? ClimbJumpRightMontage : ClimbJumpLeftMontage;
	const float Duration = CharacterOwner->PlayAnimMontage(Montage);
	SetMotionWarpingTimer(Duration);
}

void UAdventureMovementComponent::SetMotionWarpingTimer(float Duration)
{	
	GetWorld()->GetTimerManager().SetTimer(WarpTimerHandle, this, &UAdventureMovementComponent::FinishWarping, Duration);
}

void UAdventureMovementComponent::TryClimb(FVector InitialLocation, FRotator InitialRotation)
{
	if(!IsFalling() || IsCustomMovementMode(CMOVE_Climbing))
	{
		return;
	}

	InterpolateToTarget(InitialLocation, InitialRotation);

	SetMovementMode(MOVE_Custom, CMOVE_Climbing);
	CharacterOwner->StopAnimMontage(DropClimbMontage);
}

void UAdventureMovementComponent::DoClimbJump()
{
	if(!IsCustomMovementMode(CMOVE_Climbing) || bLeavingClimbing || bInMotionWarping)
	{
		return;
	}

	if(!bCanShimmy)
	{
		const float InputHorDirection = FVector::DotProduct(Acceleration.GetSafeNormal2D(), CharacterOwner->GetActorRightVector());
		if(FMath::Abs(InputHorDirection) > 0.5f)
		{
			JumpSide(InputHorDirection);
			return;
		}
	}
	
	if(FMath::Abs(HorizontalDirection) < 0.1f)
	{
		if(ClimbingComponent->CanClimbUp(MotionWarpLocation))
		{
			ClimbUp();
		}
		else
		{
			JumpUp();
		}
	}
}

void UAdventureMovementComponent::DropClimb()
{
	if(!IsCustomMovementMode(CMOVE_Climbing) || bLeavingClimbing)
	{
		return;
	}

	bLeavingClimbing = true;
	bCanShimmy = true;
	CharacterOwner->PlayAnimMontage(DropClimbMontage);
}

void UAdventureMovementComponent::InterpolateToTarget(FVector Location, FRotator Rotation)
{
	bIsInterpolating = true;
	TargetInterpolateLocation = Location;
	TargetInterpolateRotation = Rotation;
}

void UAdventureMovementComponent::ExitClimbing()
{
	FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, true, nullptr);
		
	SetMovementMode(CurrentFloor.IsWalkableFloor() ?  MOVE_Walking : MOVE_Falling);
	bLeavingClimbing = false;
	HorizontalDirection = 0;

	GetWorld()->GetTimerManager().ClearTimer(WarpTimerHandle);
}

#pragma endregion

#pragma region Movement overwritten Helper Functions
bool UAdventureMovementComponent::IsMovingOnGround() const
{
	return Super::IsMovingOnGround() || IsCustomMovementMode(CMOVE_Slide) || IsCustomMovementMode(CMOVE_Roll);
}

bool UAdventureMovementComponent::CanCrouchInCurrentState() const
{
	return Super::CanCrouchInCurrentState() || IsMovingOnGround();
}

bool UAdventureMovementComponent::CanWalkOffLedges() const
{
	if(IsCrouching() && IsCustomMovementMode(CMOVE_Roll))
	{
		return bCanWalkOffLedgeWhenRolling;
	}
	
	return Super::CanWalkOffLedges();
}

float UAdventureMovementComponent::GetMaxSpeed() const
{
	if(MovementMode == MOVE_Walking && Safe_bWantsToSprint && !IsCrouching())
	{
		return MaxSprintSpeed;
	}

	if(MovementMode != MOVE_Custom)
	{
		return Super::GetMaxSpeed();
	}

	switch (CustomMovementMode)
	{
	case CMOVE_Slide:
		return MaxSlideSpeed;
	case  CMOVE_Roll:
		return MaxRollSpeed;
	case CMOVE_Climbing:
		return 0.f;
	default:
		UE_LOG(LogTemp, Fatal, TEXT("Invalid custom movement mode"));
		return -1.f;
	}
}

float UAdventureMovementComponent::GetMaxBrakingDeceleration() const
{
	if(MovementMode != MOVE_Custom)
	{
		return Super::GetMaxBrakingDeceleration();
	}

	switch (CustomMovementMode)
	{
	case CMOVE_Slide:
		return BrakingDecelerationSliding;
	case  CMOVE_Roll:
		return BrakingDecelerationRolling;
	case CMOVE_Climbing:
		return BrakingDecelerationFlying;
	default:
		UE_LOG(LogTemp, Fatal, TEXT("Invalid custom movement mode"));
		return -1.f;
	}
}

void UAdventureMovementComponent::PhysicsRotation(float DeltaTime)
{
	if(IsCustomMovementMode(CMOVE_Climbing)|| MovementMode == MOVE_Falling)
	{
		return;
	}
	
	Super::PhysicsRotation(DeltaTime);
}

#pragma endregion

void UAdventureMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);

	if(Safe_bWantsToRoll)
	{
		if(CanRoll())
		{
			SetMovementMode(MOVE_Custom, CMOVE_Roll);
			if(!CharacterOwner->HasAuthority()) Server_EnterRoll();
		}
		Safe_bWantsToRoll = false;
	}
	
	FHitResult HitResult;
	if(GetSlideSurface(HitResult))
	{
		EnterSlide();
	}
}

void UAdventureMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);

	Safe_bPreviousWantsToCrouch = bWantsToCrouch;
}

void UAdventureMovementComponent::UpdateCharacterStateAfterMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateAfterMovement(DeltaSeconds);
}

void UAdventureMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	Super::PhysCustom(deltaTime, Iterations);

	switch (CustomMovementMode)
	{
	case CMOVE_Slide:
		PhysSlide(deltaTime, Iterations);
		break;
	case CMOVE_Roll:
		PhysRoll(deltaTime, Iterations);
		break;
	case CMOVE_Climbing:
		PhysClimbing(deltaTime, Iterations);
		break;;
	default:
		UE_LOG(LogTemp, Fatal, TEXT("Invalid Custom Movement Mode"));
	}
}

void UAdventureMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{	
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
	
	if(PreviousMovementMode == MOVE_Custom && PreviousCustomMode == CMOVE_Roll)
	{
		ExitRoll();
	}

	if(IsCustomMovementMode(CMOVE_Roll))
	{
		EnterRoll(PreviousMovementMode, static_cast<ECustomMovementMode>(PreviousCustomMode));
	}
}

void UAdventureMovementComponent::Sprint()
{
	Safe_bWantsToSprint = true;
}

void UAdventureMovementComponent::StopSprint()
{
	Safe_bWantsToSprint = false;
}

void UAdventureMovementComponent::ToggleCrouch()
{
	bWantsToCrouch = ~bWantsToCrouch;
}

bool UAdventureMovementComponent::IsCustomMovementMode(ECustomMovementMode InCustomMovementMode) const
{
	return  MovementMode == MOVE_Custom && CustomMovementMode == InCustomMovementMode;
}

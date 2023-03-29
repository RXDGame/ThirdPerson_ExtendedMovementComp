// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterAdventureCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "AdventureMovementComponent.h"
#include "ClimbingComponent.h"


//////////////////////////////////////////////////////////////////////////
// AShooterAdventureCharacter

AShooterAdventureCharacter::AShooterAdventureCharacter(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer.SetDefaultSubobjectClass<UAdventureMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	AdventureMovementComponent = Cast<UAdventureMovementComponent>(GetCharacterMovement());

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
	ClimbingComponent = CreateDefaultSubobject<UClimbingComponent>(TEXT("Climbing Component"));	
}

void AShooterAdventureCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AShooterAdventureCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ClimbingUpdate(DeltaSeconds);
	FString Message = FString::Printf(TEXT("Velocity: %s"), *GetVelocity().ToString());
	GEngine->AddOnScreenDebugMessage(10, 5, FColor::Yellow, Message);
}

FCollisionQueryParams AShooterAdventureCharacter::GetIgnoreCharacterParam() const
{
	FCollisionQueryParams Params;

	TArray<AActor*> CharacterChildren;
	GetAllChildActors(CharacterChildren);
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActors(CharacterChildren);

	return  Params;
}

void AShooterAdventureCharacter::ClimbingUpdate(float DeltaTime)
{
	if(!ensure(ClimbingComponent))
	{
		return;
	}

	FString Message = FString::Printf(TEXT("Climbing State: %s"), *FString::FromInt(ClimbingState));
	GEngine->AddOnScreenDebugMessage(1, 0, FColor::Blue, Message);

	switch (ClimbingState)
	{
	case CLIMB_NONE:
		if(AdventureMovementComponent->IsFalling() && AdventureMovementComponent->Velocity.Z <= 0)
		{
			FHitResult FwdHit;
			FHitResult TopHit;
			if(ClimbingComponent->FoundLedge(FwdHit, TopHit))
			{
				if(CurrentLedge == TopHit.GetActor())
				{
					return;
				}
				
				CurrentLedge = TopHit.GetActor();
				FVector Location = ClimbingComponent->GetCharacterLocationOnLedge(FwdHit, TopHit);
				FRotator Rotation = ClimbingComponent->GetCharacterRotationOnLedge(FwdHit);
				StartClimb(Location, Rotation);
			}
		}
		else
		{
			ResetLedge();
		}
		break;
	case CLIMB_INTERPOLATING:
		ProccessInterpolation(DeltaTime);
		break;
	case CLIMB_HANGING:
		UpdateClimbingMovement();
		break;
	case CLIMB_LAUNCHING:
		break;
	case CLIMB_WARPING:
		break;
	case CLIMB_LEAVING:
		break;
	default:
		break;
	}
	
	if(AdventureMovementComponent->MovementMode == MOVE_Walking)
	{
		ClimbingState = CLIMB_NONE;
	}
}

void AShooterAdventureCharacter::ResetLedge()
{
	CurrentLedge = nullptr;
}

void AShooterAdventureCharacter::StartClimb(FVector InitialLocation, FRotator InitialRotation)
{
	InterpolateToTarget(InitialLocation, InitialRotation);

	AdventureMovementComponent->SetMovementMode(MOVE_Custom, CMOVE_Climbing);
	StopAnimMontage(ClimbingComponent->DropClimbMontage);
}

void AShooterAdventureCharacter::InterpolateToTarget(FVector Location, FRotator Rotation)
{
	TargetInterpolateLocation = Location;
	TargetInterpolateRotation = Rotation;

	ClimbingState = CLIMB_INTERPOLATING;
}

void AShooterAdventureCharacter::DoClimbJump()
{
	if(!AdventureMovementComponent->IsCustomMovementMode(CMOVE_Climbing) || ClimbingState != CLIMB_HANGING)
	{
		return;
	}

	const FVector Acceleration = AdventureMovementComponent->GetCurrentAcceleration();
	if(!bCanShimmy)
	{
		const float InputHorDirection = FVector::DotProduct(Acceleration.GetSafeNormal2D(), GetActorRightVector());
		if(FMath::Abs(InputHorDirection) > 0.15f)
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

void AShooterAdventureCharacter::DropClimb()
{	
	if(!AdventureMovementComponent->IsCustomMovementMode(CMOVE_Climbing) || ClimbingState != CLIMB_HANGING)
	{
		return;
	}

	PlayAnimMontage(ClimbingComponent->DropClimbMontage);
	ClimbingState = CLIMB_LEAVING;
}

void AShooterAdventureCharacter::UpdateClimbingMovement()
{
	FHitResult FwdHit, TopHit;
	if(ClimbingComponent->FoundLedge(FwdHit, TopHit))
	{
		CurrentLedge = FwdHit.GetActor();
		FVector Location = ClimbingComponent->GetCharacterLocationOnLedge(FwdHit, TopHit);
		FRotator Rotation = ClimbingComponent->GetCharacterRotationOnLedge(FwdHit);
		FVector Acceleration = AdventureMovementComponent->GetCurrentAcceleration();
			
		FVector Delta = Location - GetActorLocation();
		FHitResult MoveHit;
		AdventureMovementComponent->SafeMoveUpdatedComponent(Delta, Rotation.Quaternion(), false, MoveHit);
		
		if(Acceleration.SizeSquared() <= 10.f)
		{
			HorizontalDirection = 0;
			return;
		}
		
		float shimmyDirection = FVector::DotProduct(Acceleration.GetSafeNormal2D(), GetActorRightVector());
		FVector TargetLocationOnEdge;
		bCanShimmy = ClimbingComponent->CanMoveInDirection(shimmyDirection, TopHit.GetActor(), TargetLocationOnEdge);
		if(bCanShimmy)
		{
			HorizontalDirection = shimmyDirection;
		}
		else
		{
			// check to corner out or in
			if(TryCornerOut(shimmyDirection))
			{
				return;
			}				
				
			StopShimmy();
		}
	}
}

void AShooterAdventureCharacter::StopShimmy()
{
	HorizontalDirection = 0;
	AdventureMovementComponent->Velocity = FVector::ZeroVector;
}

void AShooterAdventureCharacter::LaunchToLedge()
{
	FVector LaunchVelocity;
	const FVector Acceleration = AdventureMovementComponent->GetCurrentAcceleration();
	if(ClimbingComponent->GetValidLaunchVelocity(ClimbingComponent->GetReachableGrabPoints(Acceleration.GetSafeNormal2D()), LaunchVelocity, AdventureMovementComponent->GetGravityZ()))
	{
		LaunchCharacter(LaunchVelocity, true, true);
		
		FHitResult Hit;
		const FQuat Rotation = FRotationMatrix::MakeFromXZ(LaunchVelocity.GetSafeNormal2D(), FVector::UpVector).ToQuat();
		AdventureMovementComponent->SafeMoveUpdatedComponent(FVector::ZeroVector, Rotation, false, Hit);
		SetClimbingTimer(0.5f, CLIMB_LAUNCHING);
	}
}

void AShooterAdventureCharacter::FinishClimbingTimer()
{
	if(ClimbingState == CLIMB_LEAVING || ClimbingState == CLIMB_LAUNCHING)
	{
		ExitClimbing();
	}
	else if(ClimbingState == CLIMB_WARPING)
	{
		ClimbingState = CLIMB_HANGING;
	}
}

void AShooterAdventureCharacter::ProccessInterpolation(float DeltaTime)
{
	FHitResult Hit;
	const FVector ActorLocation = GetActorLocation();
	if(ActorLocation.Equals(TargetInterpolateLocation, 0.001f))
	{			
		AdventureMovementComponent->SafeMoveUpdatedComponent(TargetInterpolateLocation - ActorLocation, TargetInterpolateRotation.Quaternion(), false, Hit);
		ClimbingState = CLIMB_HANGING;
		return;
	}

	const FVector Location = FMath::VInterpTo(ActorLocation, TargetInterpolateLocation, DeltaTime, InterpSpeed);
	const FRotator Rotation = FMath::RInterpTo(GetActorRotation(), TargetInterpolateRotation, DeltaTime, InterpSpeed);

	AdventureMovementComponent->SafeMoveUpdatedComponent(Location - ActorLocation, Rotation.Quaternion(), false, Hit);
	HorizontalDirection = 0;
}

bool AShooterAdventureCharacter::TryCornerOut(float Direction)
{
	if(ClimbingComponent->CanCornerOut(Direction, MotionWarpLocation, MotionWarpRotation))
	{
		OnCornerStart.Broadcast();					
					
		UAnimMontage* MontageToPlay = Direction > 0 ? ClimbingComponent->RightCornerOutMontage : ClimbingComponent->LeftCornerOutMontage;
		const float Duration = PlayAnimMontage(MontageToPlay);
		SetClimbingTimer(Duration, CLIMB_WARPING);
		return true;
	}

	return false;
}

void AShooterAdventureCharacter::ClimbUp()
{	
	OnClimbUp.Broadcast();

	const float Duration = PlayAnimMontage(ClimbingComponent->ClimbUpMontage);
	SetClimbingTimer(Duration - 0.1f, CLIMB_LEAVING);
}

void AShooterAdventureCharacter::JumpUp()
{
	if(ClimbingComponent->CanHopUp(MotionWarpLocation))
	{
		OnClimbJumpStart.Broadcast();
		const float MontageDuration = PlayAnimMontage(ClimbingComponent->HopUpMontage);
		SetClimbingTimer(MontageDuration, CLIMB_WARPING);		
	}
	else
	{
		const FVector LaunchVelocity = ClimbingComponent->GetJumpUpVelocity(AdventureMovementComponent->GetGravityZ());
		LaunchCharacter(LaunchVelocity, true, true);
		const float Duration = -LaunchVelocity.Z / AdventureMovementComponent->GetGravityZ();
		SetClimbingTimer(Duration, CLIMB_LAUNCHING);
	}
	
	GEngine->AddOnScreenDebugMessage(3, 2, FColor::Blue, TEXT("Start Jump UP"));
}

void AShooterAdventureCharacter::JumpSide(float HorDirection)
{
	const bool bIsRight = HorDirection > 0;
	const FVector Direction = GetCapsuleComponent()->GetRightVector() * (bIsRight ? 1 : -1);

	FVector LaunchVelocity;
	float Duration = -1.f;
	if(!ClimbingComponent->FoundSideLedge(CurrentLedge, Direction, LaunchVelocity, AdventureMovementComponent->GetGravityZ(), Duration))
	{
		LaunchVelocity = Direction * 500.f + FVector::UpVector * 600.0f;
		Duration = -LaunchVelocity.Z / AdventureMovementComponent->GetGravityZ();
	}
	
	FString Message = FString::Printf(TEXT("Launch Velocity: %s"), *LaunchVelocity.ToString());
	GEngine->AddOnScreenDebugMessage(3, 5, FColor::Green, Message);
	LaunchCharacter(LaunchVelocity, true, true);
	
	/*UAnimMontage* Montage = bIsRight ? ClimbingComponent->ClimbJumpRightMontage : ClimbingComponent->ClimbJumpLeftMontage;	
	PlayAnimMontage(Montage);*/
	SetClimbingTimer(Duration, CLIMB_LAUNCHING);
	GEngine->AddOnScreenDebugMessage(2, 2, FColor::Blue, TEXT("Start Jump Side"));
}

void AShooterAdventureCharacter::SetClimbingTimer(float Duration, EClimbingState TimerState)
{
	ClimbingState = TimerState;
	GetWorld()->GetTimerManager().SetTimer(WarpTimerHandle, this, &AShooterAdventureCharacter::FinishClimbingTimer, Duration);
}

void AShooterAdventureCharacter::ExitClimbing()
{	
	AdventureMovementComponent->FindFloor(GetActorLocation(), AdventureMovementComponent->CurrentFloor, true, nullptr);		
	AdventureMovementComponent->SetMovementMode(AdventureMovementComponent->CurrentFloor.IsWalkableFloor() ?  MOVE_Walking : MOVE_Falling);
	HorizontalDirection = 0;
	ClimbingState = CLIMB_NONE;

	GetWorld()->GetTimerManager().ClearTimer(WarpTimerHandle);
}

//////////////////////////////////////////////////////////////////////////
// Input

void AShooterAdventureCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		//Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AShooterAdventureCharacter::Move);

		//Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AShooterAdventureCharacter::Look);

		//Sprinting
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, AdventureMovementComponent, &UAdventureMovementComponent::Sprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, AdventureMovementComponent, &UAdventureMovementComponent::StopSprint);
		
		//Crouch
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, AdventureMovementComponent, &UAdventureMovementComponent::ToggleCrouch);
		
		//Roll
		EnhancedInputComponent->BindAction(RollAction, ETriggerEvent::Started, AdventureMovementComponent, &UAdventureMovementComponent::TryEnterRoll);

		// Climbing
		EnhancedInputComponent->BindAction(ClimbUpAction, ETriggerEvent::Started, this, &AShooterAdventureCharacter::DoClimbJump);
		EnhancedInputComponent->BindAction(DropClimbAction, ETriggerEvent::Started, this, &AShooterAdventureCharacter::DropClimb);
	}
}

void AShooterAdventureCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AShooterAdventureCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}





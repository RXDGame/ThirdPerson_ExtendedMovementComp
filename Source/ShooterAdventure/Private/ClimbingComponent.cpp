// Fill out your copyright notice in the Description page of Project Settings.


#include "ClimbingComponent.h"

#include "Ledge.h"
#include "MathUtil.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

// Helper Macros
#if 1
float MacroDuration = 4.f;
#define SLOG(x) GEngine->AddOnScreenDebugMessage(-1, MacroDuration ? MacroDuration : -1.f, FColor::Yellow, x);
#define POINT(x, c) DrawDebugPoint(GetWorld(), x, 10, c, !MacroDuration, MacroDuration);
#define LINE(x1, x2, c) DrawDebugLine(GetWorld(), x1, x2, c, !MacroDuration, MacroDuration);
#define CAPSULE(x, c) DrawDebugCapsule(GetWorld(), x, 96, 42, FQuat::Identity, c, !MacroDuration, MacroDuration);
#else
#define SLOG(x)
#define POINT(x, c)
#define LINE(x1, x2, c)
#define CAPSULE(x, c)
#endif

// Sets default values for this component's properties
UClimbingComponent::UClimbingComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UClimbingComponent::BeginPlay()
{
	Super::BeginPlay();
	CapsuleComponent = GetOwner()->FindComponentByClass<UCapsuleComponent>();
}

FVector UClimbingComponent::GetTraceOrigin() const
{
	return GetOwner()->GetActorLocation() + TraceOrigin;
}

bool UClimbingComponent::IsPossibleToReach(const USceneComponent* Candidate, FVector& TossVelocity, float Gravity) const
{
	FHitResult HitResult;
	HitResult.ImpactPoint = Candidate->GetComponentLocation();
	HitResult.Normal = Candidate->GetForwardVector();
	HitResult.ImpactNormal = Candidate->GetForwardVector();
	const FVector EndLocation = GetCharacterLocationOnLedge(HitResult, HitResult);
	CAPSULE(EndLocation, FColor::Blue);
	const bool bHighArc = EndLocation.Z < GetOwner()->GetActorLocation().Z; 
	if(UGameplayStatics::SuggestProjectileVelocity(GetWorld(), TossVelocity, GetOwner()->GetActorLocation(),
		EndLocation,	MaxJumpSpeed, bHighArc, 0, Gravity, ESuggestProjVelocityTraceOption::DoNotTrace,
		FCollisionResponseParams::DefaultResponseParam, TArray<AActor*>(), DebugTrace))
	{
		return true;
	}

	return false;
}

FHitResult UClimbingComponent::GetForwardHit(FVector TraceStartOrigin, FVector TraceDirection, float TraceHeight) const
{
	FHitResult Hit;

	const FVector StartTrace = TraceStartOrigin;
	const FVector EndTrace = StartTrace + TraceDirection * MaxTraceDistance;
	UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(), StartTrace, EndTrace, CapsuleTraceRadius, TraceHeight, TraceChannel, false,
		TArray<AActor*>(), DebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None, Hit, true);

	return Hit;
}

FHitResult UClimbingComponent::GetTopHit(FHitResult forwardHit, FVector TraceDirection, FVector TraceStartOrigin) const
{
	FHitResult TopHit;
	FCollisionQueryParams Params;

	const FVector StartTrace = TraceStartOrigin + FVector::UpVector * MaxTraceHeight;
	const float step = MaxTopTraceDepth / TopTraceIterations;
	
	for (int i=0; i< TopTraceIterations; i++)
	{
		FVector TraceStartIteration = StartTrace + TraceDirection * (i * step);
		FVector EndTrace = TraceStartIteration + FVector::DownVector * MaxTraceHeight * 2.f;
		TArray<FHitResult> Hits;		
		if(UKismetSystemLibrary::LineTraceMulti(GetWorld(), TraceStartIteration, EndTrace, TraceChannel, true,
			TArray<AActor*>(), DebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None, Hits, true, FLinearColor::Yellow))
		{
			for (auto Hit : Hits)
			{
				if(Hit.bBlockingHit && Hit.GetActor() == forwardHit.GetActor())
				{
					return Hit;
				}
			}
		}
	}
	
	return TopHit;
}

bool UClimbingComponent::FoundLedge(FHitResult& FwdHit, FHitResult& TopHit) const
{	
	FwdHit = GetForwardHit(GetTraceOrigin(), GetOwner()->GetActorForwardVector(), CapsuleTraceHeight);
	if(!FwdHit.IsValidBlockingHit())
	{
		return false;
	}
	
	TopHit = GetTopHit(FwdHit, GetOwner()->GetActorForwardVector(), GetTraceOrigin());
	if(!TopHit.IsValidBlockingHit())
	{
		return false;
	}

	return true;
}

FVector UClimbingComponent::GetCharacterLocationOnLedge(FHitResult FwdHit, FHitResult TopHit) const
{
	FVector TargetLocation = FwdHit.ImpactPoint;
	TargetLocation.Z = TopHit.ImpactPoint.Z;
	TargetLocation += FwdHit.Normal.GetSafeNormal2D() * ForwardOffsetFromLedge + FVector::DownVector * VerticalOffsetFromLedge;
	
	return TargetLocation;
}

FRotator UClimbingComponent::GetCharacterRotationOnLedge(FHitResult FwdHit) const
{
	const FRotator TargetRotation = FRotationMatrix::MakeFromZX(FVector::UpVector, -FwdHit.Normal.GetSafeNormal2D()).Rotator();
	return TargetRotation;
}

TArray<USceneComponent*> UClimbingComponent::GetReachableGrabPoints(FVector MoveDirection) const
{
	TArray<USceneComponent*> ClosestPoints;
	TArray<FOverlapResult> Overlapped;
	const FCollisionShape SphereShape = FCollisionShape::MakeSphere(MaxRangeToFindLedge);
	if(GetWorld()->OverlapMultiByChannel(Overlapped, GetTraceOrigin(), FQuat::Identity, static_cast<ECollisionChannel>(TraceChannel.GetValue()), SphereShape))
	{
		const FVector CharLocation = GetOwner()->GetActorLocation();
		for (FOverlapResult OverlapResult : Overlapped)
		{
			const ALedge* Ledge = Cast<ALedge>(OverlapResult.GetActor());
			if(Ledge != nullptr)
			{
				USceneComponent* Candidate = Ledge->GetClosestPoint(GetOwner()->GetActorLocation());
				FVector LaunchDirection = Candidate->GetComponentLocation() - CharLocation;
				if(FVector::DotProduct(MoveDirection.GetSafeNormal2D(), LaunchDirection.GetSafeNormal2D()) > FMath::Cos(FMath::DegreesToRadians(MaxAngleToLaunch)))
				{
					ClosestPoints.Add(Candidate);
				}
			}
		}
	}
	
	return ClosestPoints;
}

bool UClimbingComponent::GetValidLaunchVelocity(TArray<USceneComponent*> GrabPoints, FVector& LaunchVelocity, const float Gravity) const
{
	const FVector GrabLocation = GetTraceOrigin();
	float ClosestDistance = 1000000000000.f;
	const USceneComponent* Destination = nullptr;
	for (const USceneComponent* Point : GrabPoints)
	{
		const float distance = FVector::Distance(GrabLocation, Point->GetComponentLocation());
		if(distance < ClosestDistance && distance > MinDistanceToSuggestVelocity)
		{
			FVector TossVelocity;
			if(IsPossibleToReach(Point, TossVelocity, Gravity))
			{
				LaunchVelocity = TossVelocity;
				Destination = Point;
				ClosestDistance = distance;
			}
		}
	}
	return Destination != nullptr;
}

bool UClimbingComponent::CanClimbUp(FVector& TargetClimbLocation) const
{
	FHitResult FwdHit, TopHit;
	if(FoundLedge(FwdHit, TopHit))
	{
		FVector Start = TopHit.ImpactPoint + FVector::UpVector * 90.f + GetOwner()->GetActorForwardVector().GetSafeNormal2D() * 42.f;
		FVector End = Start + FVector::DownVector * 130.f;
		FHitResult GroundHit;
		if(GetWorld()->LineTraceSingleByChannel(GroundHit, Start, End, ECC_Visibility))
		{			
			TargetClimbLocation = GroundHit.ImpactPoint;
			TargetClimbLocation += ClimbUpOffset.X * GetOwner()->GetActorForwardVector();
			TargetClimbLocation += FVector::UpVector * ClimbUpOffset.Z;

			float CapsuleHalfHeight = CapsuleComponent->GetScaledCapsuleHalfHeight();
			float CapsuleRadius = CapsuleComponent->GetScaledCapsuleRadius();
			
			return !GetWorld()->OverlapAnyTestByChannel(TargetClimbLocation + FVector::UpVector * CapsuleHalfHeight, FQuat::Identity,
				ECC_Visibility, FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight));
		}
	}

	return false;
}

bool UClimbingComponent::CanMoveInDirection(float HorizontalDirection, AActor* CurrentLedgeActor, FVector& TargetEdgeLocation) const
{
	if(FMath::Abs(HorizontalDirection) < 0.1f)
	{
		return false;
	}
	
	float Direction = HorizontalDirection > 0.f ? 1.f : -1.f;
	FVector ForwardVector = GetOwner()->GetActorForwardVector();
	FVector RightVector = GetOwner()->GetActorRightVector();

	FVector StartTrace = GetTraceOrigin() + RightVector * Direction * MinSideDistance ;
	FVector EndTrace = StartTrace + ForwardVector * MaxTraceDistance;

	TArray<FHitResult> Hits;
	if(UKismetSystemLibrary::CapsuleTraceMulti(GetWorld(), StartTrace, EndTrace, 1.f, CapsuleTraceHeight, TraceChannel, true,
		TArray<AActor*>(), DebugTrace ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None, Hits, true))
	{
		for (FHitResult Hit : Hits)
		{
			if(Hit.GetActor() == CurrentLedgeActor)
			{
				return true;
			}
		}
	}

	StartTrace += ForwardVector * (ForwardOffsetFromLedge + 1.f);
	EndTrace = StartTrace - RightVector * Direction * MinSideDistance;

	if(UKismetSystemLibrary::CapsuleTraceMulti(GetWorld(), StartTrace, EndTrace, 1.f, CapsuleTraceHeight, TraceChannel, true,
		TArray<AActor*>(), DebugTrace ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None, Hits, true, FLinearColor::Blue))
	{
		for (FHitResult Hit : Hits)
		{
			if(Hit.GetActor() == CurrentLedgeActor)
			{
				Hit.ImpactNormal = -ForwardVector;
				Hit.ImpactPoint = Hit.ImpactPoint - RightVector * Direction * MinSideDistance;
				TargetEdgeLocation = GetCharacterLocationOnLedge(Hit, Hit);				
				break;
			}
		}
	}
	
	return false;
}

bool UClimbingComponent::CanCornerOut(float MoveDirection, FVector& CornerLocation, FRotator& CornerRotation) const
{
	if(FMath::Abs(MoveDirection) < 0.1f)
	{
		return false;
	}
	
	float Direction = MoveDirection > 0.f ? 1.f : -1.f;
	FVector ForwardVector = GetOwner()->GetActorForwardVector();
	FVector RightVector = GetOwner()->GetActorRightVector();

	FVector StartTrace = GetTraceOrigin() + RightVector * Direction * MinSideDistance * 2.f;
	FVector EndTrace = StartTrace + ForwardVector * MaxTraceDistance;
	
	StartTrace += ForwardVector * (ForwardOffsetFromLedge + CornerOutDepth);
	EndTrace = StartTrace - RightVector * Direction * MinSideDistance * 2.f;

	TArray<FHitResult> Hits;
	if(UKismetSystemLibrary::CapsuleTraceMulti(GetWorld(), StartTrace, EndTrace, 1.f, CapsuleTraceHeight, TraceChannel, true,
		TArray<AActor*>(), DebugTrace ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None, Hits, true, FLinearColor::Blue))
	{
		float HeightDelta = 10000.0f;
		FVector CurrentTopHitLocation = GetOwner()->GetActorLocation() + FVector::UpVector * VerticalOffsetFromLedge;
		FHitResult SelectedTopHit;
		FHitResult SelectedFwdHit;
		for (FHitResult Hit : Hits)
		{
			FHitResult topHit = GetTopHit(Hit, - RightVector * Direction, StartTrace);
			if(!topHit.IsValidBlockingHit())
			{
				continue;
			}

			float newHeightDelta = FMath::Abs(topHit.ImpactPoint.Z - CurrentTopHitLocation.Z);
			if(newHeightDelta < HeightDelta)
			{
				SelectedTopHit = topHit;
				SelectedFwdHit = Hit;
				HeightDelta = newHeightDelta;
			}
		}

		if(SelectedTopHit.IsValidBlockingHit())
		{
			CornerLocation = GetCharacterLocationOnLedge(SelectedFwdHit, SelectedTopHit);
			CAPSULE(CornerLocation, FColor::Orange);
			CornerRotation = GetCharacterRotationOnLedge(SelectedFwdHit);
			return true;
		}
	}
	
	return false;
}

bool UClimbingComponent::CanHopUp(FVector& TargetLocation) const
{
	const float Height = MaxHopUpHeight * 0.5f;
	const FVector Origin = GetTraceOrigin() + FVector::UpVector * Height;
	const FVector Forward = GetOwner()->GetActorForwardVector();
	
	FHitResult FwdHit = GetForwardHit(Origin, Forward, Height);
	if(!FwdHit.IsValidBlockingHit())
	{
		return false;
	}

	FHitResult TopHit = GetTopHit(FwdHit, Forward, Origin);
	if(!TopHit.IsValidBlockingHit())
	{
		return false;
	}

	TargetLocation = GetCharacterLocationOnLedge(FwdHit, TopHit);
	
	return true;
}

bool UClimbingComponent::FoundSideLedge(AActor* CurrentLedge, FVector SideDirection, FVector& TargetLocation) const
{
	FVector Forward = GetOwner()->GetActorForwardVector();
	FVector FirstStartTrace = GetTraceOrigin() + SideDirection * CapsuleTraceRadius - Forward *10;
	const int Iterations = MaxSideJumpDistance / (CapsuleTraceRadius * 2);
	const float Step = MaxSideJumpDistance / Iterations;
	for (int i=0; i <= Iterations; i++)
	{
		FVector StartTrace = FirstStartTrace + SideDirection * i * Step;
		FHitResult FwdHit = GetForwardHit(StartTrace, Forward, MaxTraceHeight);
		if(!FwdHit.IsValidBlockingHit() || FwdHit.GetActor() == CurrentLedge)
		{
			continue;
		}

		FHitResult TopHit = GetTopHit(FwdHit, Forward, StartTrace);
		if(!TopHit.IsValidBlockingHit())
		{
			continue;
		}

		TargetLocation = GetCharacterLocationOnLedge(FwdHit, TopHit);
		CAPSULE(TargetLocation, FColor::Green)
		return true;
	}
	
	return false;
}

FVector UClimbingComponent::GetJumpUpVelocity() const
{
	FVector DefaultVelocity = FVector::UpVector * MaxJumpUpVelocity;
	
	const float Height = MaxHopUpHeight * 0.5f;
	const FVector Origin = GetTraceOrigin() + FVector::UpVector * Height;
	const FVector Forward = GetOwner()->GetActorForwardVector();
	
	FHitResult FwdHit = GetForwardHit(Origin, Forward, Height);
	if(!FwdHit.IsValidBlockingHit())
	{
		return DefaultVelocity;
	}

	FHitResult TopHit = GetTopHit(FwdHit, Forward, Origin);
	if(!TopHit.IsValidBlockingHit())
	{
		return DefaultVelocity;
	}

	FVector StartLocation = GetOwner()->GetActorLocation();
	FVector EndLocation = GetCharacterLocationOnLedge(FwdHit, TopHit);
	FVector TossVelocity;
	if(UGameplayStatics::SuggestProjectileVelocity(GetWorld(), TossVelocity, StartLocation, EndLocation, MaxJumpUpVelocity, false))
	{
		return TossVelocity;
	}
	
	return DefaultVelocity;
}



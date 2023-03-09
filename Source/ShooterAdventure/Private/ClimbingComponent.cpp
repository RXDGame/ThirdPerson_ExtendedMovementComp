// Fill out your copyright notice in the Description page of Project Settings.


#include "ClimbingComponent.h"

#include "Ledge.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

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
	if(UGameplayStatics::SuggestProjectileVelocity(GetWorld(), TossVelocity, GetOwner()->GetActorLocation(),
		GetCharacterLocationOnLedge(HitResult, HitResult),	MaxJumpSpeed, true, 0, Gravity, ESuggestProjVelocityTraceOption::DoNotTrace,
		FCollisionResponseParams::DefaultResponseParam, TArray<AActor*>(), true))
	{
		return true;
	}

	return false;
}

FHitResult UClimbingComponent::GetForwardHit() const
{
	FHitResult Hit;

	const FVector StartTrace = GetTraceOrigin();
	const FVector EndTrace = StartTrace + GetOwner()->GetActorForwardVector() * MaxTraceDistance;
	UKismetSystemLibrary::CapsuleTraceSingle(GetWorld(), StartTrace, EndTrace, CapsuleTraceRadius, CapsuleTraceHeight, TraceChannel, false,
		TArray<AActor*>(), DebugTrace ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None, Hit, true);

	return Hit;
}

FHitResult UClimbingComponent::GetTopHit(FHitResult forwardHit) const
{
	FHitResult TopHit;
	FCollisionQueryParams Params;

	const FVector OwnerForward = GetOwner()->GetActorForwardVector();
	const FVector StartTrace = GetTraceOrigin() + FVector::UpVector * MaxTraceHeight;
	const float step = MaxTopTraceDepth / TopTraceIterations;
	
	for (int i=0; i< TopTraceIterations; i++)
	{
		FVector TraceStartIteration = StartTrace + OwnerForward * (i * step);
		FVector EndTrace = TraceStartIteration + FVector::DownVector * MaxTraceHeight * 2.f;
		TArray<FHitResult> Hits;		
		if(UKismetSystemLibrary::LineTraceMulti(GetWorld(), TraceStartIteration, EndTrace, TraceChannel, true,
			TArray<AActor*>(), DebugTrace ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None, Hits, true, FLinearColor::Yellow))
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
	FwdHit = GetForwardHit();
	if(!FwdHit.IsValidBlockingHit())
	{
		return false;
	}
	
	TopHit = GetTopHit(FwdHit);
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
	TargetLocation += FwdHit.Normal.GetSafeNormal2D() * OffsetFromLedge.X + FVector::DownVector * OffsetFromLedge.Z;
	
	return TargetLocation;
}

FRotator UClimbingComponent::GetCharacterRotationOnLedge(FHitResult FwdHit)
{
	const FRotator TargetRotation = FRotationMatrix::MakeFromZX(FVector::UpVector, -FwdHit.Normal.GetSafeNormal2D()).Rotator();
	return TargetRotation;
}

TArray<USceneComponent*> UClimbingComponent::GetReachableGrabPoints() const
{
	TArray<USceneComponent*> ClosestPoints;
	TArray<FOverlapResult> Overlapped;
	const FCollisionShape SphereShape = FCollisionShape::MakeSphere(MaxRangeToFindLedge);
	if(GetWorld()->OverlapMultiByChannel(Overlapped, GetTraceOrigin(), FQuat::Identity, static_cast<ECollisionChannel>(TraceChannel.GetValue()), SphereShape))
	{
		for (FOverlapResult OverlapResult : Overlapped)
		{
			const ALedge* Ledge = Cast<ALedge>(OverlapResult.GetActor());
			if(Ledge != nullptr)
			{
				USceneComponent* Candidate = Ledge->GetClosestPoint(GetOwner()->GetActorLocation());
				ClosestPoints.Add(Candidate);
			}
		}
	}
	
	return ClosestPoints;
}

bool UClimbingComponent::GetValidLaunchVelocity(TArray<USceneComponent*> GrabPoints, FVector& LaunchVelocity, float Gravity) const
{
	const FVector CharacterLocation = GetOwner()->GetActorLocation();
	float ClosestDistance = 1000000000000.f;
	const USceneComponent* Destination = nullptr;
	for (USceneComponent* Point : GrabPoints)
	{
		const float distance = FVector::Distance(CharacterLocation, Point->GetComponentLocation());
		if(distance < ClosestDistance)
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
			TargetClimbLocation += ClimbUpOffset.Y * GetOwner()->GetActorForwardVector();
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

	StartTrace += ForwardVector * (OffsetFromLedge.X + 1.f);
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


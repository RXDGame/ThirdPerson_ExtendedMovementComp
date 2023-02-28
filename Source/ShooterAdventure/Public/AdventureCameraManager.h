// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "AdventureCameraManager.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTERADVENTURE_API AAdventureCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly) float CrouchBlendDuration = 0.5f;

	float CrouchBlendTime;
	
public:
	AAdventureCameraManager();

	virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;
};

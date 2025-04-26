// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SetupToolInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class USetupToolInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class BELINDAVPTOOL_API ISetupToolInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface
public:
	// Add the function signatures that need to be implemented in Blueprints here.
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "CameraManagerInterface")
	void AddCamera() ;

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "CameraManagerInterface")
	void SetupCameraPosition();
};

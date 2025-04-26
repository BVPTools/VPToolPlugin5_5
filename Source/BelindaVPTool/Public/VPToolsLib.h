// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VPToolsLib.generated.h"




/**
 * 
 */
UCLASS()
class BELINDAVPTOOL_API UVPToolsLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	

public :

	UFUNCTION(BlueprintCallable, Category = "VPTools")
	static void DisplayErrorMessage(FString message, bool succes);

	//UFUNCTION(BlueprintCallable, Category = "VPTools")
	//static void ApplyProjectSettings();


	UFUNCTION(BlueprintCallable, Category = "VPTools")
	static void SetLiveLink(ULiveLinkComponentController* freedFiz, UCameraComponent* camComp);
	
	//UFUNCTION(BlueprintCallable, Category = "VPTools")
	//static void RestartEditor();
};

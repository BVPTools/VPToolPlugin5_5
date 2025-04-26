// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VPEdtiorToolsLib.generated.h"


USTRUCT()
struct FConfigAssetDatas
{
	GENERATED_BODY()

	FString name;

	FString path;
};

USTRUCT()
struct FProjectSettingsDatas
{
	GENERATED_BODY()

	FString tcPath;
	FString tsPath;
	FString LLPath;
	bool genTC;
	FString framerateTC;
};


/**
 * 
 */
UCLASS()
class BELINDAVPTOOLEDITOR_API UVPEdtiorToolsLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "VPEditorTools")
	static void RestartEditor();

	UFUNCTION(BlueprintCallable, Category = "VPEditorTools")
	static void ApplyProjectSettings();

	//void ApplyProjectSettings(const FProjectSettingsDatas& UserDatas);


	static void ApplyProjectSettings(const FProjectSettingsDatas& UserDatas);

	static void SetCurrentMapToDefault();
	UFUNCTION(BlueprintCallable, Category = "VPEditorTools")
	static bool GetEditingViewportMouseRayStartAndEnd(FVector& Start, FVector& End);

	static FString ConvertToUnrealAssetPath(const FString& LevelFilePath) ;


};

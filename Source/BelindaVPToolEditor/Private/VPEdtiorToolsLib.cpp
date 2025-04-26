// Fill out your copyright notice in the Description page of Project Settings.


#include "VPEdtiorToolsLib.h"
#include "Misc/MessageDialog.h"
#include "Misc/FeedbackContext.h"
#include "Misc/Paths.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/ScopedSlowTask.h"
#include "UnrealEdMisc.h"
#include "Editor.h"
#include "Misc/ConfigCacheIni.h"
#include <Settings/EditorSettings.h>

static void EnsureSectionsAreSaveable(const FString& IniPath, const TArray<FString>& DesiredSections)
{
	const FString SectionsToSaveSection = TEXT("SectionsToSave");
	bool bNeedUpdate = false;

	// Toujours charger d'abord
	GConfig->LoadFile(IniPath);

	// 1. Vérifier si bCanSaveAllSections est déjà à true
	bool bCurrentCanSaveAllSections = false;
	GConfig->GetBool(*SectionsToSaveSection, TEXT("bCanSaveAllSections"), bCurrentCanSaveAllSections, *IniPath);

	if (!bCurrentCanSaveAllSections)
	{
		bNeedUpdate = true;
		GConfig->SetBool(*SectionsToSaveSection, TEXT("bCanSaveAllSections"), true, *IniPath);
	}

	// 2. Vérifier si toutes les sections sont déjà présentes
	TArray<FString> ExistingSections;
	GConfig->GetArray(*SectionsToSaveSection, TEXT("Section"), ExistingSections, *IniPath);

	for (const FString& DesiredSection : DesiredSections)
	{
		if (!ExistingSections.Contains(DesiredSection))
		{
			ExistingSections.Add(DesiredSection);
			bNeedUpdate = true;
		}
	}

	// 3. Si besoin, mettre à jour
	if (bNeedUpdate)
	{
		GConfig->SetArray(*SectionsToSaveSection, TEXT("Section"), ExistingSections, *IniPath);
		GConfig->Flush(false, *IniPath);

		UE_LOG(LogTemp, Log, TEXT("EnsureSectionsAreSaveable: Updated [%s]"), *IniPath);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("EnsureSectionsAreSaveable: No update needed for [%s]"), *IniPath);
	}
}




void UVPEdtiorToolsLib::RestartEditor()
{
	FText Title = NSLOCTEXT("UnrealEditor", "RestartTitle", "Restart Editor?");
	FText Message = NSLOCTEXT("UnrealEditor", "RestartMessage", "The editor needs to restart for changes to take effect. Do you want to restart now?");

	// Show the message dialog to the user
	EAppReturnType::Type ReturnType = FMessageDialog::Open(EAppMsgType::YesNo, Message, &Title);

	// Check if the user chose 'Yes' (restart)
	if (ReturnType == EAppReturnType::Yes)
	{
		// If the user chose to restart, do it
		FUnrealEdMisc::Get().RestartEditor(false);
	}
}

void UVPEdtiorToolsLib::ApplyProjectSettings()
{
#if WITH_EDITOR
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FString GameIniPath = FConfigCacheIni::NormalizeConfigIniPath(FPaths::ProjectConfigDir() / TEXT("DefaultGame.ini"));
	FString EngineIniPath = FConfigCacheIni::NormalizeConfigIniPath(FPaths::ProjectConfigDir() / TEXT("DefaultEngine.ini"));


	UE_LOG(LogTemp, Warning, TEXT("MyGameEditor: Log Started"));

	UE_LOG(LogTemp, Warning, TEXT("MyGameEditor: Log Started"));

	FString Section = TEXT("/Script/LiveLink.LiveLinkSettings");
	FString Key = TEXT("DefaultLiveLinkPreset");
	FString Value = TEXT("/BelindaVPTool/LivelinkConfigs/FreeD45.FreeD45");

	// Set and flush Game.ini
	GConfig->SetString(*Section, *Key, *Value, *GameIniPath);
	GConfig->Flush(false, *GameIniPath);
	UE_LOG(LogTemp, Warning, TEXT("Flushed to GameIni: %s"), *GameIniPath);

	// Update other settings
	Section = TEXT("/Script/Engine.Engine");
	Key = TEXT("CustomTimeStepClassName");
	Value = TEXT("/BelindaVPTool/Blueprints/TS_BlackMagic.TS_BlackMagic_C");

	GConfig->SetString(*Section, *Key, *Value, *EngineIniPath);
	GConfig->Flush(false, *EngineIniPath);
	UE_LOG(LogTemp, Warning, TEXT("Flushed to EngineIni: %s"), *EngineIniPath);

	// More settings if needed...
	Section = TEXT("/Script/Engine.Engine");
	Key = TEXT("TimecodeProviderClassName");
	Value = TEXT("/BelindaVPTool/Blueprints/TC_BlackMagic.TC_BlackMagic_C");

	GConfig->SetString(*Section, *Key, *Value, *EngineIniPath);
	GConfig->Flush(false, *EngineIniPath);

	Section = TEXT("/Script/Engine.Engine");
	Key = TEXT("bGenerateDefaultTimecode");

	GConfig->SetBool(*Section, *Key, false, *EngineIniPath);
	GConfig->Flush(false, *EngineIniPath);

	GConfig->LoadFile(GameIniPath);
	GConfig->LoadFile(EngineIniPath);


	UE_LOG(LogTemp, Warning, TEXT("GGameIni Path: %s"), *GGameIni);
	UE_LOG(LogTemp, Warning, TEXT("GEngineIni Path: %s"), *GEngineIni);
	UE_LOG(LogTemp, Warning, TEXT("GameIniPath Path: %s"), *GameIniPath);
	UE_LOG(LogTemp, Warning, TEXT("EngineIniPath Path: %s"), *EngineIniPath);

#endif
}

void UVPEdtiorToolsLib::ApplyProjectSettings(const FProjectSettingsDatas& UserDatas)
{
#if WITH_EDITOR
	const FString ProjectConfigDir = FPaths::ProjectConfigDir();
	const FString GameIniPath = FConfigCacheIni::NormalizeConfigIniPath(ProjectConfigDir / TEXT("DefaultGame.ini"));
	const FString EngineIniPath = FConfigCacheIni::NormalizeConfigIniPath(ProjectConfigDir / TEXT("DefaultEngine.ini"));


	EnsureSectionsAreSaveable(GameIniPath, { TEXT("StartupActions"), TEXT("/Script/LiveLink.LiveLinkSettings") });
	EnsureSectionsAreSaveable(EngineIniPath, { TEXT("/Script/Engine.Engine") });


	const TCHAR* SectionLL = TEXT("/Script/LiveLink.LiveLinkSettings");
	GConfig->SetString(SectionLL, TEXT("DefaultLiveLinkPreset"), *UserDatas.LLPath, *GameIniPath);

	const TCHAR* SectionEngine = TEXT("/Script/Engine.Engine");
	GConfig->SetString(SectionEngine, TEXT("CustomTimeStepClassName"), *(UserDatas.tsPath + TEXT("_C")), *EngineIniPath);
	GConfig->SetString(SectionEngine, TEXT("TimecodeProviderClassName"), *(UserDatas.tcPath + TEXT("_C")), *EngineIniPath);
	GConfig->SetBool(SectionEngine, TEXT("bGenerateDefaultTimecode"), UserDatas.genTC, *EngineIniPath);

	if (UserDatas.genTC)
	{
		FString FrameRate;
		if (UserDatas.framerateTC == "24 Fps") FrameRate = TEXT("(Numerator=24,Denominator=1)");
		else if (UserDatas.framerateTC == "25 Fps") FrameRate = TEXT("(Numerator=25,Denominator=1)");
		else if (UserDatas.framerateTC == "50 Fps") FrameRate = TEXT("(Numerator=50,Denominator=1)");

		GConfig->SetString(SectionEngine, TEXT("GenerateDefaultTimecodeFrameRate"), *FrameRate, *EngineIniPath);
	}


	GConfig->Flush(false, *GameIniPath);
	GConfig->Flush(false, *EngineIniPath);


	RestartEditor();
#endif
}



void UVPEdtiorToolsLib::SetCurrentMapToDefault()
{
#if WITH_EDITOR

	FString EngineIniPath = FConfigCacheIni::NormalizeConfigIniPath(FPaths::ProjectConfigDir() / TEXT("DefaultEngine.ini"));


	UE_LOG(LogTemp, Warning, TEXT("MyGameEditor: Log Started"));

	UE_LOG(LogTemp, Warning, TEXT("MyGameEditor: Log Started"));

	FString Section = TEXT("/Script/EngineSettings.GameMapsSettings");
	FString Key = TEXT("EditorStartupMap");
	FString NTLevelPath = "";
	if (GEditor && GEditor->GetEditorWorldContext().World())
	{
		UWorld* World = GEditor->GetEditorWorldContext().World();
		if (World)
		{
			// Get the persistent level (main level of the world)
			ULevel* CurrentLevel = World->GetCurrentLevel();
			if (CurrentLevel)
			{
				// Get the full path of the current level
				const FString LevelPath = CurrentLevel->GetOutermost()->GetName();

				///UE_LOG(LogTemp, Warning, TEXT("Current level path: %s"), *LevelPath);
				// Convert the path to a game-friendly format if needed (for use in config files, etc.)
				const FString NLevelPath = FPackageName::LongPackageNameToFilename(LevelPath, FPackageName::GetMapPackageExtension());


				NTLevelPath = UVPEdtiorToolsLib::ConvertToUnrealAssetPath(NLevelPath);

				UE_LOG(LogTemp, Warning, TEXT("Current level path: %s"), *NTLevelPath);
			}
		}
	}




	FString Value = NTLevelPath;

	GConfig->SetString(*Section, *Key, *Value, *EngineIniPath);
	GConfig->Flush(false, *EngineIniPath);

	UE_LOG(LogTemp, Warning, TEXT("Flushed to EngineIni: %s"), *EngineIniPath);

	GConfig->LoadFile(EngineIniPath);

	UE_LOG(LogTemp, Warning, TEXT("GGameIni Path: %s"), *GGameIni);
	UE_LOG(LogTemp, Warning, TEXT("GEngineIni Path: %s"), *GEngineIni);
	UE_LOG(LogTemp, Warning, TEXT("EngineIniPath Path: %s"), *EngineIniPath);

#endif
}

bool UVPEdtiorToolsLib::GetEditingViewportMouseRayStartAndEnd(FVector& Start, FVector& End)
{
	Start = End = FVector::ZeroVector;

	// Get the active viewport client from the editor
	if (GEditor && GEditor->GetActiveViewport())
	{
		FEditorViewportClient* ViewportClient = static_cast<FEditorViewportClient*>(GEditor->GetActiveViewport()->GetClient());

		if (ViewportClient)
		{
			// Only if we're not tracking (RMB looking)
			if (!ViewportClient->IsTracking())
			{
				FViewport* Viewport = ViewportClient->Viewport;

				// Make sure we have a valid viewport, otherwise we won't be able to construct an FSceneView. The first time we're ticked we might not be properly set up.
				if (Viewport != nullptr && Viewport->GetSizeXY().GetMin() > 0)
				{
					const int32 ViewportInteractX = Viewport->GetMouseX();
					const int32 ViewportInteractY = Viewport->GetMouseY();

					FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
						Viewport,
						ViewportClient->GetScene(),
						ViewportClient->EngineShowFlags)
						.SetRealtimeUpdate(ViewportClient->IsRealtime()));
					FSceneView* SceneView = ViewportClient->CalcSceneView(&ViewFamily);

					const FViewportCursorLocation MouseViewportRay(SceneView, ViewportClient, ViewportInteractX, ViewportInteractY);
					const FVector& MouseViewportRayOrigin = MouseViewportRay.GetOrigin();
					const FVector& MouseViewportRayDirection = MouseViewportRay.GetDirection();

					Start = MouseViewportRayOrigin;

					End = Start + WORLD_MAX * MouseViewportRayDirection;

					// If we're dealing with an orthographic view, push the origin of the ray backward along the viewport forward axis
					// to make sure that we can detect objects that are behind the origin!
					if (!ViewportClient->IsPerspective())
					{
						Start -= MouseViewportRayDirection * FLT_MAX;
					}

					return true;
				}
			}
		}
	}

	return false;
}



FString UVPEdtiorToolsLib::ConvertToUnrealAssetPath(const FString& LevelFilePath)
{
	// Step 1: Convert the absolute file path to the Unreal package path (e.g., from Content folder)
	FString PackagePath = FPackageName::FilenameToLongPackageName(LevelFilePath);

	// Step 2: Extract the map name from the file path (remove extension .umap and extract the base name)
	FString MapName = FPackageName::GetShortName(PackagePath);

	// Step 3: Format the path to match the asset path structure (e.g., /Game/Maps/MyMap.MyMap)
	FString FormattedPath = FString::Printf(TEXT("%s.%s"), *PackagePath, *MapName);

	return FormattedPath;
}
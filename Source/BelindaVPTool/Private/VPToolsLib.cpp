// Fill out your copyright notice in the Description page of Project Settings.


#include "VPToolsLib.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "LiveLinkController.h"
#include "LiveLinkComponentController.h"
#include "LiveLinkCameraController.h"
#include "Roles/LiveLinkCameraRole.h"


void UVPToolsLib::DisplayErrorMessage(FString message, bool succes)
{
    // Convert FString to FText
    FNotificationInfo Info(FText::FromString(message));

    // Use FAppStyle instead of FEditorStyle
    Info.Image = FAppStyle::GetBrush(TEXT("LevelEditor.RecompileGameCode"));
    Info.FadeInDuration = 0.1f;
    Info.FadeOutDuration = 0.5f;
    Info.ExpireDuration = 3.0f;
    Info.bUseThrobber = false;
    Info.bUseSuccessFailIcons = true;
    Info.bUseLargeFont = true;
    Info.bFireAndForget = false;
    Info.bAllowThrottleWhenFrameRateIsLow = false;

    // Create and configure notification
    TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);
    if (NotificationItem.IsValid())
    {
        NotificationItem->SetCompletionState(succes == true ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
        NotificationItem->ExpireAndFadeout();
    }

    //// Ensure CompileSuccessSound is declared and valid before using it
    //if (GEditor)
    //{
    //    //GEditor->PlayEditorSound(TEXT("/Game/Sounds/CompileSuccessSound.CompileSuccessSound")); // Replace with your actual sound path
    //}
}

//void UVPToolsLib::ApplyProjectSettings()
//{
//#if WITH_EDITOR
//    // This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
//    FString GameIniPath = FPaths::ProjectConfigDir() / TEXT("DefaultGame.ini");
//    FString EngineIniPath = FPaths::ProjectConfigDir() / TEXT("DefaultEngine.ini");
//
//
//    UE_LOG(LogTemp, Warning, TEXT("MyGameEditor: Log Started"));
//
//    UE_LOG(LogTemp, Warning, TEXT("MyGameEditor: Log Started"));
//
//    FString Section = TEXT("/Script/LiveLink.LiveLinkSettings");
//    FString Key = TEXT("DefaultLiveLinkPreset");
//    FString Value = TEXT("/BelindaVPTool/LivelinkConfigs/FreeD45.FreeD45");
//
//    // Set and flush Game.ini
//    GConfig->SetString(*Section, *Key, *Value, *GameIniPath);
//    GConfig->Flush(false, *GameIniPath);
//    UE_LOG(LogTemp, Warning, TEXT("Flushed to GameIni: %s"), *GameIniPath);
//
//    // Update other settings
//    Section = TEXT("/Script/Engine.Engine");
//    Key = TEXT("CustomTimeStepClassName");
//    Value = TEXT("/BelindaVPTool/Blueprints/TS_BlackMagic.TS_BlackMagic_C");
//
//    GConfig->SetString(*Section, *Key, *Value, *EngineIniPath);
//    GConfig->Flush(false, *EngineIniPath);
//    UE_LOG(LogTemp, Warning, TEXT("Flushed to EngineIni: %s"), *EngineIniPath);
//
//    // More settings if needed...
//    Section = TEXT("/Script/Engine.Engine");
//    Key = TEXT("TimecodeProviderClassName");
//    Value = TEXT("/BelindaVPTool/Blueprints/TC_BlackMagic.TC_BlackMagic_C");
//
//    GConfig->SetString(*Section, *Key, *Value, *EngineIniPath);
//    GConfig->Flush(false, *EngineIniPath);
//
//    Section = TEXT("/Script/Engine.Engine");
//    Key = TEXT("bGenerateDefaultTimecode");
//
//    GConfig->SetBool(*Section, *Key, false, *EngineIniPath);
//    GConfig->Flush(false, *EngineIniPath);
//
//    GConfig->LoadFile(GameIniPath);
//    GConfig->LoadFile(EngineIniPath);
//
//
//    UE_LOG(LogTemp, Warning, TEXT("GGameIni Path: %s"), *GGameIni);
//    UE_LOG(LogTemp, Warning, TEXT("GEngineIni Path: %s"), *GEngineIni);
//    UE_LOG(LogTemp, Warning, TEXT("GameIniPath Path: %s"), *GameIniPath);
//    UE_LOG(LogTemp, Warning, TEXT("EngineIniPath Path: %s"), *EngineIniPath);
//#endif
//}

void UVPToolsLib::SetLiveLink(ULiveLinkComponentController* freedFiz, UCameraComponent* camComp)
{
    ULiveLinkCameraController* cameraController = NewObject<ULiveLinkCameraController>(); 
    cameraController->bUseCameraRange = true;

    cameraController->SetAttachedComponent(camComp);

    freedFiz->ControllerMap.Add(ULiveLinkCameraRole::StaticClass(), cameraController);
}


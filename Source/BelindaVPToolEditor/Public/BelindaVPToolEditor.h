// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

//#include "Engine.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "VPEdtiorToolsLib.h"
//#include "UnrealEd.h"

class UMediaOutput;
class SEditableTextBox;

UENUM(BlueprintType)
enum class BrushType : uint8 {
	LOGO = 0 UMETA(DisplayName = "DOWN"),
	LOGO_TXT = 1  UMETA(DisplayName = "LEFT")
};

UENUM(BlueprintType)
enum class BtnType : uint8 {
	APPLY = 0 UMETA(DisplayName = "Apply project settings"),
	DLT_SETS = 1  UMETA(DisplayName = "Remove project settings"),
	ADD_CAM = 3  UMETA(DisplayName = "Add camera rig"),
	DLT_CAM = 4  UMETA(DisplayName = "Delete camera rig"),
	SLCT_CAM = 5  UMETA(DisplayName = "Select and focus on camera"),
	SLCT_CAMMAN = 6  UMETA(DisplayName = "Select and focus on camera manager"),
	PILOT_CAM = 7  UMETA(DisplayName = "Pilot camera"),
	SET_MC = 8  UMETA(DisplayName = "Pilot camera"),
	STP_CAM = 9  UMETA(DisplayName = "Pilot camera"),
	PLC_OBJ = 10  UMETA(DisplayName = "Pilot camera"),
	FCS_OBJ = 11  UMETA(DisplayName = "Pilot camera"),
	ADD_COMPCAM = 12  UMETA(DisplayName = "Add comp camera rig"),
	DLT_COMPCAM = 13  UMETA(DisplayName = "Delete comp camera rig"),
	SLCT_COMPCAM = 14  UMETA(DisplayName = "Select and focus on comp camera "),
	SLCT_COMPCAMMAN = 15  UMETA(DisplayName = "Select and focus on comp camera manager"),
	ADD_NDCAM = 16  UMETA(DisplayName = "Add comp camera rig"),
	DLT_NDCAM = 17  UMETA(DisplayName = "Delete comp camera rig"),
	SLCT_NDCAM = 18  UMETA(DisplayName = "Select and focus on comp camera "),
	SLCT_NDCAMMAN = 19  UMETA(DisplayName = "Select and focus on comp camera manager"),
};


class FBelindaVPToolEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static  void RunEditorUtilityWidget(FString WidgetPath);

	TSharedRef<SDockTab> OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs);

	void OnTextChanged(const FText& NewText);

	void OnTextCommitted(const FText& NewText, ETextCommit::Type CommitType);

	FText GetNodalValue() const;

	FText GetButtonName() const;

	FReply OnRCPClicked();

	FReply OnSetLevelClicked();

	void FindTCBlueprintsOfClass(UClass* ClassType);

	void FindTSBlueprintsOfClass(UClass* ClassType);

	void FindMOBlueprintsOfClass(UClass* ClassType);

	void FindPresetOfClass(UClass* ClassType);

	void FindPresetMOOfClass(UClass* ClassType);

	void OnTCBlueprintSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	FText GetSelectedTCBlueprintItem() const;

	void OnTSBlueprintSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	FText GetSelectedTSBlueprintItem() const;

	void OnMOBlueprintSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	FText GetSelectedMOBlueprintItem() const;

	void OnLLSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	FText GetSelectedLLItem() const;

	bool IsGenTCCheckboxEnabled() const;

	bool CheckSpawnCameraState() const;

	bool CheckSpawncompCameraState() const;

	bool CheckSpawnNDCameraState() const;

	UWorld* GetCurrentWorld() const;

	bool CheckCameraPresence() const;

	bool CheckCompCameraPresence() const;

	bool CheckNDCameraPresence() const;

	bool FindActorsOfClass(UWorld* World, UClass* ActorClass) const;

	AActor* GetFirstActorOfClass(UWorld* World, UClass* ActorClass) const;

	bool DestroyActorsOfClass(UWorld* World, UClass* ActorClass) const;

	UClass* LoadBlueprintClassByPath(const FString& BlueprintPath) const;

	void OnTCCheckboxStateChanged(ECheckBoxState NewState);

	void OnDropdownSelectionChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);

	TSharedRef<SWidget> GenerateDropdownItem(TSharedPtr<FString> InItem);

	TSharedRef<SWidget> GenerateDropdownItem2(TSharedPtr<FString> InItem);

	FText GetCurrentDropdownItem() const;

	const FSlateBrush* GetImageBrush(BrushType toUseType) const;

	FReply OnButtonClick(BtnType btnType);

	void FocusAndSelectCompCam();

	void FocusAndSelectCompCamMan();

	void FocusAndSelectNDCam();

	void FocusAndSelectNDCamMan();

	void AddMenuExtension(FMenuBuilder& Builder);

	void OnMenuButtonClicked();

	void OnFillArrays();

	FReply SpawnRig(BtnType btnType) ;

	AActor* SpawnActor(UClass* toSpawnClass,FVector location = FVector(0.0f,0.0f,100.0f));

	FReply SpawnAndSetupRig(BtnType btnType);

	void FocusAndSelect(AActor* objectToFocus);

	void CleanScene();

	void CleanCompScene();

	void CleanNDScene();

	bool CallFunctionByName(UObject* Object, FName FunctionName);

	bool CallFunctionByNameWWorld(UObject* Object, FName FunctionName);

	void PilotCamera();

	void EjectCamera();

	void ApplyUserProjectSettings();

	void ResetProjectSettings();

	void SetupCam();

	void PlaceAtObject();

	void FocusOnObject();

	void ApplyMediaCapture();

	void SpawnCompCam();

	void SpawnNDCam();

	void RemoveCompCam();

	void RemoveNDCam();

	void OnSliderValueChanged(float value);

	float GetDefaultSliderValue() const;

private : 
	void RegisterMenuExtensions();

	// Dropdown options
	TArray<TSharedPtr<FString>> DropdownOptions;

	// Currently selected dropdown item
	TSharedPtr<FString> SelectedDropdownItem;


	TArray<TSharedPtr<FString>> LLOptions;

	TSharedPtr<FString> SelectedLL;

	TArray<TSharedPtr<FString>> TCBlueprintOptions;

	TSharedPtr<FString> TCSelectedBlueprint;
	
	TArray<TSharedPtr<FString>> TSBlueprintOptions;

	TSharedPtr<FString> TSSelectedBlueprint;

	TArray<TSharedPtr<FString>> MOBlueprintOptions;

	TSharedPtr<FString> MOSelectedBlueprint;

	bool bGenerateDefaultTC = false;

	FSlateColorBrush brush = FSlateColorBrush(FLinearColor(0.017642f, 0.017642f, 0.017642f, 1.0f));

	TArray<FConfigAssetDatas> liveLinkAssetsDatas;
	TArray<FConfigAssetDatas> TCAssetsDatas;
	TArray<FConfigAssetDatas> TSAssetsDatas;
	TArray<FConfigAssetDatas> MOAssetsDatas;

	mutable bool bIsCameraPresent = false;
	mutable bool bIsCompCameraPresent = false;
	mutable bool bIsNDCameraPresent = false;

	AActor* spawnedCamMan = nullptr;
	AActor* spawnedCam = nullptr;
	AActor* spawnedProbe = nullptr;

	bool bPilotCam = false;

	FText pilotCamBtnTxt;

	TArray<UMediaOutput*> moFiles;

	TSharedPtr<SDockTab> BelindaVPToolkitTab;
	TSharedPtr<SEditableTextBox> NumericTextBox;
	public:

		// Functions to handle checkbox state change
		void OnFirstCheckboxChanged(ECheckBoxState NewState);
		void OnSecondCheckboxChanged(ECheckBoxState NewState);

		// Variables to store the state of checkboxes
		ECheckBoxState FirstCheckboxState = ECheckBoxState::Checked;
		ECheckBoxState SecondCheckboxState = ECheckBoxState::Unchecked;

		// Getters for checkbox states
		ECheckBoxState GetFirstCheckboxState() const { return FirstCheckboxState; }
		ECheckBoxState GetSecondCheckboxState() const { return SecondCheckboxState; }
};

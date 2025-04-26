// Copyright Epic Games, Inc. All Rights Reserved.

#include "BelindaVPToolEditor.h"
#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "EditorUtilitySubsystem.h"
#include "Blueprint/UserWidget.h"
#include "ToolMenus.h"
#include "MyEditorStyle.h"
#include "LevelEditor.h"
#include "SlateBasics.h"
#include "SlateExtras.h"
#include "EditorStyleSet.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Engine/Texture2D.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Images/SImage.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "GenlockedTimecodeProvider.h"
#include "GenlockedCustomTimeStep.h"
#include "LiveLinkPreset.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "EngineUtils.h"
#include "LevelEditorSubsystem.h"
#include "MediaOutput.h"
#include "Selection.h"
#include "MediaCapture.h"
#include "RemoteControlPreset.h" 
#include "MediaFrameworkUtilitiesEditor/Private/CaptureTab/MediaFrameworkCapturePanelBlueprintLibrary.h"



IMPLEMENT_MODULE(FBelindaVPToolEditorModule, BelindaVPToolEditor)


#define LOCTEXT_NAMESPACE "FBelindaVPToolEditorModule"

void FBelindaVPToolEditorModule::StartupModule()
{
	// Register a function to be called when menu system is initialized
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(
		this, &FBelindaVPToolEditorModule::RegisterMenuExtensions));

	FMyEditorStyle::Initialize();

	// Populate dropdown menus
	DropdownOptions.Add(MakeShareable(new FString("24 Fps")));
	DropdownOptions.Add(MakeShareable(new FString("25 Fps")));
	DropdownOptions.Add(MakeShareable(new FString("50 FPs")));

	if (DropdownOptions.Num() > 0)
	{
		SelectedDropdownItem = DropdownOptions[0];  // Default to the first item
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to populate DropdownOptions array."));
	}

	const FName TabName = "VPTools";

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TabName, FOnSpawnTab::CreateRaw(this, &FBelindaVPToolEditorModule::OnSpawnPluginTab))
		.SetDisplayName(NSLOCTEXT("VPToolConfig", "Config", "Belinda VPToolkit"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	// On retire toute instance précédente (au cas où le layout l’a gardé)
	FCoreDelegates::OnPostEngineInit.AddLambda([TabName]()
		{
			TSharedPtr<SDockTab> ExistingTab = FGlobalTabmanager::Get()->FindExistingLiveTab(TabName);
			if (ExistingTab.IsValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("Forcing closure of VPTools tab (layout cleanup)."));
				ExistingTab->RequestCloseTab();
			}

			// Supprimer la section de layout enregistrée pour ce tab
			FString LayoutIniPath = FPaths::ProjectSavedDir() / TEXT("Config/Windows/EditorLayout.ini");
			if (FPaths::FileExists(LayoutIniPath))
			{
				GConfig->EmptySection(*FString::Printf(TEXT("Tab(%s)"), *TabName.ToString()), *LayoutIniPath);
				GConfig->Flush(false, *LayoutIniPath);
				UE_LOG(LogTemp, Log, TEXT("Cleaned layout entry for tab: %s"), *TabName.ToString());
			}
		});

	// Add menu extension 
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
	MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, nullptr, FMenuExtensionDelegate::CreateRaw(this, &FBelindaVPToolEditorModule::AddMenuExtension));
	LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);


	// Initialize the custom style
	FMyEditorStyle::Initialize();

	//EditorViewportClient::hit
}

void FBelindaVPToolEditorModule::ShutdownModule()
{
	if (!liveLinkAssetsDatas.IsEmpty())
	{
		liveLinkAssetsDatas.Empty();
	}

	if (!TCAssetsDatas.IsEmpty())
	{
		TCAssetsDatas.Empty();
	}

	if (!TSAssetsDatas.IsEmpty())
	{
		TSAssetsDatas.Empty();
	}

	if (!MOAssetsDatas.IsEmpty())
	{
		MOAssetsDatas.Empty();
	}

	//if (BelindaVPToolkitTab.IsValid())
	//{
	//	BelindaVPToolkitTab->RequestCloseTab();
	//	UE_LOG(LogTemp, Warning, TEXT("MyGameEditor: Close Tab"));
	//}

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner("VPTools");

	TSharedPtr<SDockTab> ExistingTab = FGlobalTabmanager::Get()->FindExistingLiveTab(FName("VPTools"));
	if (ExistingTab.IsValid())
	{
		ExistingTab->RequestCloseTab();
		UE_LOG(LogTemp, Log, TEXT("VPTools Tab closed during shutdown."));
	}

	UE_LOG(LogTemp, Warning, TEXT("FBelindaVPToolEditorModule: Log Ended"));
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	FMyEditorStyle::Shutdown();
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner("VPTools");
	FMyEditorStyle::Shutdown();

}

TSharedRef<SDockTab> FBelindaVPToolEditorModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{

	pilotCamBtnTxt = bPilotCam ? FText::FromString("Eject camera") : FText::FromString("Pilot camera");



	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SVerticalBox)

				// --- Section 1: Header Section ---
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(10)
				[
					SNew(STextBlock)
						.Text(FText::FromString("Belinda Virtual Production Toolkit"))
						.Font(FAppStyle::GetFontStyle("BoldFont"))
						.Justification(ETextJustify::Center)
				]

				// --- Scrollable Area for Expandable Sections ---
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SNew(SScrollBox)
						.Orientation(Orient_Vertical)

						+ SScrollBox::Slot()
						[
							SNew(SVerticalBox)

								// Expandable Area Section 1
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(SExpandableArea)
										.InitiallyCollapsed(false)
										.Padding(8.0f)
										.HeaderContent()
										[
											SNew(STextBlock)
												.Text(NSLOCTEXT("VPTools", "Config", "Project Settings"))
										]
										.BodyContent()
										[
											SNew(SBorder)
												.Padding(0)
												.BorderImage(&brush)
												[
													SNew(SVerticalBox)

														// --- LiveLink Selection Section ---
														+ SVerticalBox::Slot()
														.AutoHeight()
														.Padding(5)
														[
															SNew(SBorder)
																.Padding(5)
																[
																	SNew(SVerticalBox)

																		// Section Title inside the Border
																		+ SVerticalBox::Slot()
																		.AutoHeight()
																		.Padding(5)
																		[
																			SNew(STextBlock)
																				.Text(FText::FromString("LiveLink preset"))
																				.Font(FAppStyle::GetFontStyle("BoldFont"))
																		]

																		// LiveLink ComboBox inside the same Border
																		+ SVerticalBox::Slot()
																		.AutoHeight()
																		.Padding(5)
																		[
																			SNew(SComboBox<TSharedPtr<FString>>)
																				.OptionsSource(&LLOptions)
																				.OnSelectionChanged_Raw(this, &FBelindaVPToolEditorModule::OnLLSelectionChanged)
																				.OnGenerateWidget_Raw(this, &FBelindaVPToolEditorModule::GenerateDropdownItem)
																				[
																					SNew(STextBlock)
																						.Text_Raw(this, &FBelindaVPToolEditorModule::GetSelectedLLItem)
																				]
																		]
																]
														]

														// --- TS Blueprint Selection Section ---
														+ SVerticalBox::Slot()
														.AutoHeight()
														.Padding(5)
														[
															SNew(SBorder)
																.Padding(5)
																[
																	SNew(SVerticalBox)

																		// Section Title inside the Border
																		+ SVerticalBox::Slot()
																		.AutoHeight()
																		.Padding(5)
																		[
																			SNew(STextBlock)
																				.Text(FText::FromString("Timestamp"))
																				.Font(FAppStyle::GetFontStyle("BoldFont"))
																		]

																		// TS Blueprint ComboBox inside the same Border
																		+ SVerticalBox::Slot()
																		.AutoHeight()
																		.Padding(5)
																		[
																			SNew(SComboBox<TSharedPtr<FString>>)
																				.OptionsSource(&TSBlueprintOptions)
																				.OnSelectionChanged_Raw(this, &FBelindaVPToolEditorModule::OnTSBlueprintSelectionChanged)
																				.OnGenerateWidget_Raw(this, &FBelindaVPToolEditorModule::GenerateDropdownItem)
																				[
																					SNew(STextBlock)
																						.Text_Raw(this, &FBelindaVPToolEditorModule::GetSelectedTSBlueprintItem)
																				]
																		]
																]
														]

														// --- TC Blueprint Selection Section ---
														+ SVerticalBox::Slot()
														.AutoHeight()
														.Padding(5)
														[
															SNew(SBorder)
																.Padding(5)
																[
																	SNew(SVerticalBox)

																		// Section Title inside the Border
																		+ SVerticalBox::Slot()
																		.AutoHeight()
																		.Padding(5)
																		[
																			SNew(STextBlock)
																				.Text(FText::FromString("Timecode"))
																				.Font(FAppStyle::GetFontStyle("BoldFont"))
																		]

																		// TC Blueprint ComboBox inside the same Border
																		+ SVerticalBox::Slot()
																		.AutoHeight()
																		.Padding(5)
																		[
																			SNew(SComboBox<TSharedPtr<FString>>)
																				.OptionsSource(&TCBlueprintOptions)
																				.OnSelectionChanged_Raw(this, &FBelindaVPToolEditorModule::OnTCBlueprintSelectionChanged)
																				.OnGenerateWidget_Raw(this, &FBelindaVPToolEditorModule::GenerateDropdownItem)
																				[
																					SNew(STextBlock)
																						.Text_Raw(this, &FBelindaVPToolEditorModule::GetSelectedTCBlueprintItem)
																				]
																		]
																		// Checkbox inside the same Border
																		+ SVerticalBox::Slot()
																		.AutoHeight()
																		.Padding(5)
																		[
																			SNew(SCheckBox)
																				.OnCheckStateChanged_Raw(this, &FBelindaVPToolEditorModule::OnTCCheckboxStateChanged)
																				.IsChecked(ECheckBoxState::Unchecked)
																				[
																					SNew(STextBlock)
																						.Text(FText::FromString("Generate default timecode"))
																						.Font(FAppStyle::GetFontStyle("RegularFont"))
																				]
																		]
																		// Checkbox inside the same Border
																		+ SVerticalBox::Slot()
																		.AutoHeight()
																		.Padding(5)
																		[
																			SNew(SHorizontalBox)
																				+ SHorizontalBox::Slot()
																				.AutoWidth()
																				.Padding(5)
																				.VAlign(EVerticalAlignment::VAlign_Center)
																				[
																					SNew(STextBlock)
																						.Text(FText::FromString("Generate default timecode framerate"))
																						.Font(FAppStyle::GetFontStyle("RegularFont"))
																						.Justification(ETextJustify::Left)
																						.IsEnabled_Raw(this, &FBelindaVPToolEditorModule::IsGenTCCheckboxEnabled)
																				]
																				+ SHorizontalBox::Slot()
																				.AutoWidth()
																				[
																					SNew(SSpacer)
																						.Size(FVector2D(10.0f, 1.0f))
																				]
																				+ SHorizontalBox::Slot()
																				.AutoWidth()
																				.Padding(5)
																				[
																					SNew(SComboBox<TSharedPtr<FString>>)
																						.OptionsSource(&DropdownOptions)
																						.OnSelectionChanged_Raw(this, &FBelindaVPToolEditorModule::OnDropdownSelectionChanged)
																						.OnGenerateWidget_Raw(this, &FBelindaVPToolEditorModule::GenerateDropdownItem)
																						.IsEnabled_Raw(this, &FBelindaVPToolEditorModule::IsGenTCCheckboxEnabled)
																						[
																							SNew(STextBlock)
																								.Text_Raw(this, &FBelindaVPToolEditorModule::GetCurrentDropdownItem)
																						]
																				]
																		]
																]
														]
														// --- Button Section ---
														+ SVerticalBox::Slot()
														.AutoHeight()
														.Padding(5)
														[
															SNew(SBorder)
																.Padding(5)
																[
																	SNew(SVerticalBox)

																		// Section Title inside the Border
																		+ SVerticalBox::Slot()
																		.AutoHeight()
																		.Padding(5)
																		[
																			SNew(STextBlock)
																				.Text(FText::FromString("Apply project settings"))
																				.Font(FAppStyle::GetFontStyle("BoldFont"))
																		]

																		// Buttons inside the same Border
																		+ SVerticalBox::Slot()
																		.AutoHeight()
																		.Padding(5)
																		[
																			SNew(SWrapBox)
																				.UseAllottedSize(true)
																				// Button 1
																				+ SWrapBox::Slot()
																				.Padding(5)
																				[
																					SNew(SButton)
																						.Text(FText::FromString("Apply"))
																						.OnClicked_Raw(this, &FBelindaVPToolEditorModule::OnButtonClick, BtnType::APPLY)
																				]

																				// Button 2
																				+ SWrapBox::Slot()
																				.Padding(5)
																				[
																					SNew(SButton)
																						.Text(FText::FromString("Remove"))
																						.OnClicked_Raw(this, &FBelindaVPToolEditorModule::OnButtonClick, BtnType::DLT_SETS)
																				]

																				+ SWrapBox::Slot()
																				.Padding(5)
																				[
																					SNew(SButton)
																						.Text(FText::FromString("Set current level as default"))
																						.OnClicked_Raw(this, &FBelindaVPToolEditorModule::OnSetLevelClicked)
																				]
																		]
																]
														]
												]
										]
								]
								// Expandable Area Section 2
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(SExpandableArea)
										.InitiallyCollapsed(false)
										.Padding(8.0f)
										.HeaderContent()
										[
											SNew(STextBlock)
												.Text(NSLOCTEXT("VPTools", "Config", "Camera Tools"))
										]
										.BodyContent()
										[
											SNew(SBorder)
												.Padding(0)
												.BorderImage(&brush)
												[
													SNew(SVerticalBox)
														// --- Camera Button Section ---
														+ SVerticalBox::Slot()
														.AutoHeight()
														.Padding(5)

														[
															SNew(SBorder)
																.Padding(5)
																[
																	SNew(SVerticalBox)

																		// Section Title inside the Border
																		+ SVerticalBox::Slot()
																		.AutoHeight()
																		.Padding(5)
																		[
																			SNew(STextBlock)
																				.Text(FText::FromString("Simple Camera Management"))
																				.Font(FAppStyle::GetFontStyle("BoldFont"))
																		]

																		// Buttons inside the same Border
																		+ SVerticalBox::Slot()
																		.AutoHeight()
																		[
																			SNew(SWrapBox)
																				.UseAllottedSize(true)
																				// Button add cam
																				+ SWrapBox::Slot()
																				.Padding(5)
																				[
																					SNew(SButton)
																						.Text(FText::FromString("Add Camera"))
																						.OnClicked_Raw(this, &FBelindaVPToolEditorModule::SpawnAndSetupRig, BtnType::ADD_CAM)
																						.IsEnabled_Raw(this, &FBelindaVPToolEditorModule::CheckSpawnCameraState)
																				]

																				// Button delete cam
																				+ SWrapBox::Slot()
																				.Padding(5)
																				[
																					SNew(SButton)
																						.Text(FText::FromString("Remove Camera"))
																						.OnClicked_Raw(this, &FBelindaVPToolEditorModule::OnButtonClick, BtnType::DLT_CAM)
																						.IsEnabled_Raw(this, &FBelindaVPToolEditorModule::CheckCameraPresence)
																				]
																				// Button select cam
																				+ SWrapBox::Slot()
																				.Padding(5)
																				[
																					SNew(SButton)
																						.Text(FText::FromString("Select Camera"))
																						.OnClicked_Raw(this, &FBelindaVPToolEditorModule::OnButtonClick, BtnType::SLCT_CAM)
																						.IsEnabled_Raw(this, &FBelindaVPToolEditorModule::CheckCameraPresence)
																				]
																				// Button select target
																				+ SWrapBox::Slot()
																				.Padding(5)
																				[
																					SNew(SButton)
																						.Text(FText::FromString("Select Target"))
																						.OnClicked_Raw(this, &FBelindaVPToolEditorModule::OnButtonClick, BtnType::SLCT_CAMMAN)
																						.IsEnabled_Raw(this, &FBelindaVPToolEditorModule::CheckCameraPresence)
																				]
																		]
																]


														]
														// --- Camera COMPOSURE Button Section ---
														+ SVerticalBox::Slot()
														.AutoHeight()
														.Padding(5)

														[
															SNew(SBorder)
																.Padding(5)
																[
																	SNew(SVerticalBox)

																		// Section Title inside the Border
																		+ SVerticalBox::Slot()
																		.AutoHeight()
																		.Padding(5)
																		[
																			SNew(STextBlock)
																				.Text(FText::FromString("Composure Camera Management"))
																				.Font(FAppStyle::GetFontStyle("BoldFont"))
																		]

																		// Buttons inside the same Border
																		+ SVerticalBox::Slot()
																		.AutoHeight()
																		[
																			SNew(SWrapBox)
																				.UseAllottedSize(true)
																				// Button add cam
																				+ SWrapBox::Slot()
																				.Padding(5)
																				[
																					SNew(SButton)
																						.Text(FText::FromString("Add Comp Camera"))
																						.OnClicked_Raw(this, &FBelindaVPToolEditorModule::SpawnAndSetupRig, BtnType::ADD_COMPCAM)
																						.IsEnabled_Raw(this, &FBelindaVPToolEditorModule::CheckSpawncompCameraState)
																				]

																				// Button delete cam
																				+ SWrapBox::Slot()
																				.Padding(5)
																				[
																					SNew(SButton)
																						.Text(FText::FromString("Remove Comp Camera"))
																						.OnClicked_Raw(this, &FBelindaVPToolEditorModule::OnButtonClick, BtnType::DLT_COMPCAM)
																						.IsEnabled_Raw(this, &FBelindaVPToolEditorModule::CheckCompCameraPresence)
																				]
																				// Button select cam
																				+ SWrapBox::Slot()
																				.Padding(5)
																				[
																					SNew(SButton)
																						.Text(FText::FromString("Select Comp Camera"))
																						.OnClicked_Raw(this, &FBelindaVPToolEditorModule::OnButtonClick, BtnType::SLCT_COMPCAM)
																						.IsEnabled_Raw(this, &FBelindaVPToolEditorModule::CheckCompCameraPresence)
																				]
																				// Button select target
																				+ SWrapBox::Slot()
																				.Padding(5)
																				[
																					SNew(SButton)
																						.Text(FText::FromString("Select Comp Target"))
																						.OnClicked_Raw(this, &FBelindaVPToolEditorModule::OnButtonClick, BtnType::SLCT_COMPCAMMAN)
																						.IsEnabled_Raw(this, &FBelindaVPToolEditorModule::CheckCompCameraPresence)
																				]
																		]
																]


														]
															// --- Camera NDISPLAY Button Section ---
															+ SVerticalBox::Slot()
															.AutoHeight()
															.Padding(5)

															[
																SNew(SBorder)
																	.Padding(5)
																	[
																		SNew(SVerticalBox)

																			// Section Title inside the Border
																			+ SVerticalBox::Slot()
																			.AutoHeight()
																			.Padding(5)
																			[
																				SNew(STextBlock)
																					.Text(FText::FromString("Ndisplay Camera Management"))
																					.Font(FAppStyle::GetFontStyle("BoldFont"))
																			]

																			// Buttons inside the same Border
																			+ SVerticalBox::Slot()
																			.AutoHeight()
																			[
																				SNew(SWrapBox)
																					.UseAllottedSize(true)
																					// Button add cam
																					+ SWrapBox::Slot()
																					.Padding(5)
																					[
																						SNew(SButton)
																							.Text(FText::FromString("Add NDisplay Camera"))
																							.OnClicked_Raw(this, &FBelindaVPToolEditorModule::SpawnAndSetupRig, BtnType::ADD_NDCAM)
																							.IsEnabled_Raw(this, &FBelindaVPToolEditorModule::CheckSpawnNDCameraState)
																					]

																					// Button delete cam
																					+ SWrapBox::Slot()
																					.Padding(5)
																					[
																						SNew(SButton)
																							.Text(FText::FromString("Remove NDisplay Camera"))
																							.OnClicked_Raw(this, &FBelindaVPToolEditorModule::OnButtonClick, BtnType::DLT_NDCAM)
																							.IsEnabled_Raw(this, &FBelindaVPToolEditorModule::CheckNDCameraPresence)
																					]
																					// Button select cam
																					+ SWrapBox::Slot()
																					.Padding(5)
																					[
																						SNew(SButton)
																							.Text(FText::FromString("Select NDisplay Camera"))
																							.OnClicked_Raw(this, &FBelindaVPToolEditorModule::OnButtonClick, BtnType::SLCT_NDCAM)
																							.IsEnabled_Raw(this, &FBelindaVPToolEditorModule::CheckNDCameraPresence)
																					]
																					// Button select target
																					+ SWrapBox::Slot()
																					.Padding(5)
																					[
																						SNew(SButton)
																							.Text(FText::FromString("Select NDisplay"))
																							.OnClicked_Raw(this, &FBelindaVPToolEditorModule::OnButtonClick, BtnType::SLCT_NDCAMMAN)
																							.IsEnabled_Raw(this, &FBelindaVPToolEditorModule::CheckNDCameraPresence)
																					]
																			]
																	]


															]

														// --- Camera Placement Section ---
														+SVerticalBox::Slot()
														.AutoHeight()
														.Padding(5)

														[
															SNew(SBorder)
																.Padding(5)
																.IsEnabled_Raw(this, &FBelindaVPToolEditorModule::CheckCameraPresence)
																[
																	SNew(SVerticalBox)
																		// Section Title inside the Border
																		+ SVerticalBox::Slot()
																		.AutoHeight()
																		.Padding(5)
																		[
																			SNew(STextBlock)
																				.Text(FText::FromString("Camera controls"))
																				.Font(FAppStyle::GetFontStyle("BoldFont"))
																		]

																		// Buttons inside the same Border
																		+ SVerticalBox::Slot()
																		.AutoHeight()
																		[
																			SNew(SWrapBox)
																				.UseAllottedSize(true)
																				// Button add cam
																				+ SWrapBox::Slot()
																				.Padding(5)
																				[
																					SNew(SButton)
																						.Text(FText::FromString("Setup Camera"))
																						.HAlign(EHorizontalAlignment::HAlign_Center)
																						.VAlign(EVerticalAlignment::VAlign_Center)
																						.OnClicked_Raw(this, &FBelindaVPToolEditorModule::OnButtonClick, BtnType::STP_CAM)
																						.IsEnabled_Raw(this, &FBelindaVPToolEditorModule::CheckCameraPresence)
																				]

																				// Button delete cam
																				+ SWrapBox::Slot()
																				.Padding(5)
																				[
																					SNew(SButton)
																						.Text(FText::FromString("Place target\nAt object"))
																						.HAlign(EHorizontalAlignment::HAlign_Center)
																						.VAlign(EVerticalAlignment::VAlign_Center)
																						.OnClicked_Raw(this, &FBelindaVPToolEditorModule::OnButtonClick, BtnType::PLC_OBJ)
																						.IsEnabled_Raw(this, &FBelindaVPToolEditorModule::CheckCameraPresence)
																				]
																				// Button select cam
																				+ SWrapBox::Slot()
																				.Padding(5)
																				[
																					SNew(SButton)
																						.Text(FText::FromString("Focus selected"))
																						.HAlign(EHorizontalAlignment::HAlign_Center)
																						.VAlign(EVerticalAlignment::VAlign_Center)
																						.OnClicked_Raw(this, &FBelindaVPToolEditorModule::OnButtonClick, BtnType::FCS_OBJ)
																						.IsEnabled_Raw(this, &FBelindaVPToolEditorModule::CheckCameraPresence)
																				]
																				+ SWrapBox::Slot()
																				.Padding(5)
																				[
																					SNew(SVerticalBox)

																						+ SVerticalBox::Slot()
																						.AutoHeight()
																						.Padding(10)
																						[
																							SNew(STextBlock)
																								.Text(FText::FromString("Target rotation"))
																						]

																						+ SVerticalBox::Slot()
																						.AutoHeight()
																						.Padding(10)
																						[
																							SNew(SSlider)
																								.OnValueChanged_Raw(this, &FBelindaVPToolEditorModule::OnSliderValueChanged)
																								.Value_Raw(this, &FBelindaVPToolEditorModule::GetDefaultSliderValue)
																						]
																				]
																				+ SWrapBox::Slot()
																				.Padding(5)
																				[
																					SNew(SVerticalBox)

																						+ SVerticalBox::Slot()
																						.AutoHeight()
																						.Padding(10)
																						[
																							SNew(STextBlock)
																								.Text(FText::FromString("Nodal offset :"))
																						]

																						+ SVerticalBox::Slot()
																						.AutoHeight()
																						.Padding(10)
																						[
																							SAssignNew(NumericTextBox, SEditableTextBox)
																								.OnTextChanged_Raw(this, &FBelindaVPToolEditorModule::OnTextChanged)
																								.OnTextCommitted_Raw(this, &FBelindaVPToolEditorModule::OnTextCommitted)
																								.HintText(FText::FromString("Nodal offset"))
																								.Text_Raw(this, &FBelindaVPToolEditorModule::GetNodalValue)

																						]
																				]
																		]
																]
														]
														// --- Media capture Section ---
														+ SVerticalBox::Slot()
														.AutoHeight()
														.Padding(5)
														[
															SNew(SBorder)
																.Padding(5)
																.IsEnabled_Raw(this, &FBelindaVPToolEditorModule::CheckCameraPresence)
																[
																	SNew(SVerticalBox)
																		// Section Title inside the Border
																		+ SVerticalBox::Slot()
																		.AutoHeight()
																		.Padding(5)
																		[
																			SNew(STextBlock)
																				.Text(FText::FromString("Media Capture"))
																				.Font(FAppStyle::GetFontStyle("BoldFont"))
																		]

																		// Buttons inside the same Border
																		+ SVerticalBox::Slot()
																		.AutoHeight()
																		[
																			SNew(SWrapBox)
																				.UseAllottedSize(true)
																				// Button add cam
																				+ SWrapBox::Slot()
																				.Padding(5)
																				[
																					SNew(SButton)
																						.Text_Raw(this, &FBelindaVPToolEditorModule::GetButtonName)
																						//.Text(FText::FromString("Pilot Camera"))
																						.HAlign(EHorizontalAlignment::HAlign_Center)
																						.VAlign(EVerticalAlignment::VAlign_Center)
																						.OnClicked_Raw(this, &FBelindaVPToolEditorModule::OnButtonClick, BtnType::PILOT_CAM)
																				]

																				// Button delete cam
																				+ SWrapBox::Slot()
																				.Padding(5)
																				[
																					SNew(SButton)
																						.Text(FText::FromString("Apply Media\nCapture Settings"))
																						.HAlign(EHorizontalAlignment::HAlign_Center)
																						.VAlign(EVerticalAlignment::VAlign_Center)
																						.OnClicked_Raw(this, &FBelindaVPToolEditorModule::OnButtonClick, BtnType::SET_MC)
																				]
																				+ SWrapBox::Slot()
																				.Padding(5)
																				[
																					SNew(SVerticalBox)

																						+ SVerticalBox::Slot()
																						.VAlign(EVerticalAlignment::VAlign_Center)
																						.FillHeight(1.0f)
																						.Padding(2)
																						[
																							SNew(SHorizontalBox)

																								+ SHorizontalBox::Slot()
																								.VAlign(EVerticalAlignment::VAlign_Center)
																								[
																									SNew(STextBlock)
																										.Text(FText::FromString("Select media output file "))
																										.Font(FAppStyle::GetFontStyle("RegularFont"))
																										.Justification(ETextJustify::Left)
																								]

																								+ SHorizontalBox::Slot()
																								.AutoWidth()
																								.Padding(5)
																								[
																									SNew(SComboBox<TSharedPtr<FString>>)
																										.OptionsSource(&MOBlueprintOptions)
																										.OnSelectionChanged_Raw(this, &FBelindaVPToolEditorModule::OnMOBlueprintSelectionChanged)
																										.OnGenerateWidget_Raw(this, &FBelindaVPToolEditorModule::GenerateDropdownItem2)
																										[
																											SNew(STextBlock)
																												.Text_Raw(this, &FBelindaVPToolEditorModule::GetSelectedMOBlueprintItem)
																										]
																								]

																						]
																						+ SVerticalBox::Slot()
																						.VAlign(EVerticalAlignment::VAlign_Center)
																						.Padding(2)
																						[
																							SNew(SHorizontalBox)

																								+ SHorizontalBox::Slot()
																								[
																									SNew(SCheckBox)
																										.IsChecked_Raw(this, &FBelindaVPToolEditorModule::GetFirstCheckboxState)
																										.OnCheckStateChanged_Raw(this, &FBelindaVPToolEditorModule::OnFirstCheckboxChanged)
																										[
																											SNew(STextBlock).Text(FText::FromString(TEXT("Method1")))
																										]
																								]
																								+ SHorizontalBox::Slot()
																								[
																									SNew(SCheckBox)
																										.IsChecked_Raw(this, &FBelindaVPToolEditorModule::GetSecondCheckboxState)
																										.OnCheckStateChanged_Raw(this, &FBelindaVPToolEditorModule::OnSecondCheckboxChanged)
																										[
																											SNew(STextBlock).Text(FText::FromString(TEXT("Method2")))
																										]
																								]


																						]
																				]
																		]
																]
														]
														+ SVerticalBox::Slot()
														.AutoHeight()
														.Padding(5)
														[
															SNew(SBorder)
																.Padding(5)
																.IsEnabled_Raw(this, &FBelindaVPToolEditorModule::CheckCameraPresence)
																[
																	SNew(SVerticalBox)
																		// Section Title inside the Border
																		+ SVerticalBox::Slot()
																		.AutoHeight()
																		.Padding(5)
																		[
																			SNew(STextBlock)
																				.Text(FText::FromString("Remote Control"))
																				.Font(FAppStyle::GetFontStyle("BoldFont"))
																		]

																		// Buttons inside the same Border
																		+ SVerticalBox::Slot()
																		.AutoHeight()
																		.Padding(5)
																		[
																			SNew(SWrapBox)
																				.UseAllottedSize(true)
																				// Button 1
																				+ SWrapBox::Slot()
																				.Padding(5)
																				[
																					SNew(SButton)
																						.Text(FText::FromString("Open Remote Control"))
																						.OnClicked_Raw(this, &FBelindaVPToolEditorModule::OnRCPClicked)
																				]
																		]
																		// // Buttons inside the same Border
																		// + SVerticalBox::Slot()
																		// .AutoHeight()
																		// .Padding(5)
																		// [
																		// 	SNew(SWrapBox)
																		// 		.UseAllottedWidth(true)
																		// 		// Button 1
																		// 		+ SWrapBox::Slot()
																		// 		.Padding(5)
																		// 		[
																		// 			SNew(SButton)
																		// 				.Text(FText::FromString("Set current level as default"))
																		// 				.OnClicked_Raw(this, &FBelindaVPToolEditorModule::OnSetLevelClicked)
																		// 		]
																		// ]
																]
														]
												]
										]
								]
						]
				]
				// Image Section at the very bottom, no spacer
				+ SVerticalBox::Slot()
				.AutoHeight()
				.VAlign(VAlign_Bottom)
				.HAlign(HAlign_Left)
				.Padding(5)
				[
					SNew(SHorizontalBox)
						// First Image (64x64)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(5)
						[
							SNew(SBox)
								.WidthOverride(16.f)
								.HeightOverride(16.f)
								[
									SNew(SImage)
										.Image_Raw(this, &FBelindaVPToolEditorModule::GetImageBrush, BrushType::LOGO)
								]
						]
						// Second Image (135x64)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(5)
						[
							SNew(SBox)
								.WidthOverride(84.8f)
								.HeightOverride(10.0f)
								.MinAspectRatio(8.48f)
								.MaxAspectRatio(8.48f)
								[
									SNew(SImage)
										.Image_Raw(this, &FBelindaVPToolEditorModule::GetImageBrush, BrushType::LOGO_TXT)
								]
						]
				]
		];
	//return BelindaVPToolkitTab.ToSharedRef();
}

void FBelindaVPToolEditorModule::OnTextChanged(const FText& NewText)
{
	FString InputString = NewText.ToString();

	// Validate the input to allow only numeric characters
	FString FilteredString;
	for (TCHAR Char : InputString)
	{
		if (FChar::IsDigit(Char)) // Allow only digits
		{
			FilteredString.AppendChar(Char);
		}
	}

	// If input has non-numeric characters, update the text box with filtered string
	if (FilteredString != InputString)
	{
		NumericTextBox->SetText(FText::FromString(FilteredString));
	}
}

void FBelindaVPToolEditorModule::OnTextCommitted(const FText& NewText, ETextCommit::Type CommitType)
{
	FString InputString = NewText.ToString();

	// Log the committed float value
	UE_LOG(LogTemp, Log, TEXT("Numeric Value Committed: %s"), *InputString);
}

FText FBelindaVPToolEditorModule::GetNodalValue() const
{
	if (!IsValid(spawnedCamMan))
		return FText::AsNumber(0.0f);

	float MyFloatValue = 0.0f;

	FProperty* Property = spawnedCamMan->GetClass()->FindPropertyByName(FName("ZNodalOffset"));
	if (Property)
	{
		FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property);
		if (FloatProperty)
		{
			MyFloatValue = FloatProperty->GetPropertyValue_InContainer(spawnedCamMan);
		}
		UE_LOG(LogTemp, Log, TEXT("MyFloatParameter value set: %f"), MyFloatValue);

	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("MyFloatParameter value not set: %f"), MyFloatValue);
	}

	return FText::AsNumber(MyFloatValue);
}

FText FBelindaVPToolEditorModule::GetButtonName() const
{
	return pilotCamBtnTxt;
}

FReply FBelindaVPToolEditorModule::OnRCPClicked()
{
	FString PresetPath = TEXT("/BelindaVPTool/RemoteControlPresets/RCP_Belinda.RCP_Belinda");

	URemoteControlPreset* Preset = LoadObject<URemoteControlPreset>(nullptr, *PresetPath);
	if (Preset)
	{
		// Open the preset in the editor
		GEditor->EditObject(Preset);
		UE_LOG(LogTemp, Log, TEXT("Opened Remote Control Preset: %s"), *PresetPath);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not find Remote Control Preset: %s"), *PresetPath);
	}

	return FReply::Handled();
}

FReply FBelindaVPToolEditorModule::OnSetLevelClicked()
{
	UVPEdtiorToolsLib::SetCurrentMapToDefault();


	return FReply::Handled();
}

void FBelindaVPToolEditorModule::FindTCBlueprintsOfClass(UClass* ClassType)
{
	//TArray<TSharedPtr<FString>> bpOptions;
	TCBlueprintOptions.Empty();

	// Add "None" option as the first entry in the dropdown
	TCBlueprintOptions.Add(MakeShared<FString>(TEXT("None")));

	// Use AssetRegistry to find all blueprints of the given class inside a specific folder
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetData;

	// Define the folder path you want to search in (your plugin content folder)
	const FName PluginContentFolder = "/BelindaVPTool"; // Replace with the actual path to your plugin's content folder

	// Create an asset registry filter
	FARFilter Filter;
	Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName()); // Filter by Blueprint class
	Filter.PackagePaths.Add(PluginContentFolder);                         // Only search inside the plugin folder
	Filter.bRecursivePaths = true;                                        // Search recursively within the folder

	// Get assets matching the filter
	AssetRegistryModule.Get().GetAssets(Filter, AssetData);

	TCAssetsDatas.Empty();

	for (const FAssetData& Asset : AssetData)
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>(Asset.GetAsset()))
		{
			if (Blueprint->GeneratedClass && Blueprint->GeneratedClass->IsChildOf(ClassType))
			{

				TCBlueprintOptions.Add(MakeShared<FString>(Blueprint->GetName()));
				FConfigAssetDatas tempDatas;
				tempDatas.name = Asset.AssetName.ToString();
				tempDatas.path = Asset.GetSoftObjectPath().ToString();
				TCAssetsDatas.Add(tempDatas);
			}
		}
	}
	if (!TCBlueprintOptions.IsEmpty())
		TCSelectedBlueprint = TCBlueprintOptions[0];
}

void FBelindaVPToolEditorModule::FindTSBlueprintsOfClass(UClass* ClassType)
{
	TSBlueprintOptions.Empty();

	// Add "None" option as the first entry in the dropdown
	TSBlueprintOptions.Add(MakeShared<FString>(TEXT("None")));

	// Use AssetRegistry to find all blueprints of the given class inside a specific folder
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetData;

	// Define the folder path you want to search in (your plugin content folder)
	const FName PluginContentFolder = "/BelindaVPTool"; // Replace with the actual path to your plugin's content folder

	// Create an asset registry filter
	FARFilter Filter;
	Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName()); // Filter by Blueprint class
	Filter.PackagePaths.Add(PluginContentFolder);                         // Only search inside the plugin folder
	Filter.bRecursivePaths = true;                                        // Search recursively within the folder

	// Get assets matching the filter
	AssetRegistryModule.Get().GetAssets(Filter, AssetData);

	TSAssetsDatas.Empty();

	for (const FAssetData& Asset : AssetData)
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>(Asset.GetAsset()))
		{
			if (Blueprint->GeneratedClass && Blueprint->GeneratedClass->IsChildOf(ClassType))
			{

				TSBlueprintOptions.Add(MakeShared<FString>(Blueprint->GetName()));
				FConfigAssetDatas tempDatas;
				tempDatas.name = Asset.AssetName.ToString();
				tempDatas.path = Asset.GetSoftObjectPath().ToString();
				TSAssetsDatas.Add(tempDatas);
			}
		}
	}
	if (!TSBlueprintOptions.IsEmpty())
		TSSelectedBlueprint = TSBlueprintOptions[0];
}

void FBelindaVPToolEditorModule::FindMOBlueprintsOfClass(UClass* ClassType)
{

	MOBlueprintOptions.Empty();

	// Add "None" option as the first entry in the dropdown
	MOBlueprintOptions.Add(MakeShared<FString>(TEXT("None")));

	// Use AssetRegistry to find all assets of the given class inside a specific folder
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetData;

	// Define the folder path you want to search in (your plugin content folder)
	const FName PluginContentFolder = "/BelindaVPTool"; // Replace with actual path

	// Create an asset registry filter
	FARFilter Filter;
	//Filter.ClassPaths.Add(ClassType->GetClassPathName()); // Filter by UMediaOutput class and its subclasses
	Filter.PackagePaths.Add(PluginContentFolder);         // Only search inside the plugin folder
	Filter.bRecursivePaths = true;                        // Search recursively within the folder

	// Get assets matching the filter
	AssetRegistryModule.Get().GetAssets(Filter, AssetData);

	//UE_LOG(LogTemp, Warning, TEXT("Total Assets Found: %d"), AssetData.Num());
	MOAssetsDatas.Empty();
	moFiles.Empty();
	for (const FAssetData& Asset : AssetData)
	{
		// Log asset info to see what assets were found
		//UE_LOG(LogTemp, Warning, TEXT("Asset Found: %s of AssetClassPath: %s"), *Asset.AssetName.ToString(), *Asset.AssetClassPath.ToString());

		UObject* LoadedAsset = Asset.GetAsset(); // Try loading the asset

		if (LoadedAsset)
		{
			//UE_LOG(LogTemp, Warning, TEXT("Loaded Asset: %s of Class: %s"), *LoadedAsset->GetName(), *LoadedAsset->GetClass()->GetName());

			// Check if the loaded asset is a child of UMediaOutput
			if (LoadedAsset->IsA(ClassType))
			{
				UE_LOG(LogTemp, Warning, TEXT("Asset is a child of UMediaOutput"));
				MOBlueprintOptions.Add(MakeShared<FString>(LoadedAsset->GetName()));

				moFiles.Add(Cast<UMediaOutput>(LoadedAsset));

				FConfigAssetDatas tempDatas;
				tempDatas.name = Asset.AssetName.ToString();
				tempDatas.path = Asset.GetSoftObjectPath().ToString();
				MOAssetsDatas.Add(tempDatas);
			}
			else
			{
				//UE_LOG(LogTemp, Warning, TEXT("Loaded Asset is NOT a child of UMediaOutput"));
			}
		}
		else
		{
			//UE_LOG(LogTemp, Warning, TEXT("Failed to load asset: %s"), *Asset.AssetName.ToString());
		}
	}

	//UE_LOG(LogTemp, Warning, TEXT("FOUND MO : %i "), MOBlueprintOptions.Num());

	//for (TSharedPtr<FString> option : MOBlueprintOptions)
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("FOUND OPTION : %s "), **option);
	//}

	if (!MOBlueprintOptions.IsEmpty())
		MOSelectedBlueprint = MOBlueprintOptions[0];


}

void FBelindaVPToolEditorModule::FindPresetOfClass(UClass* ClassType)
{
	//TArray<TSharedPtr<FString>> bpOptions;
	LLOptions.Empty();

	// Add "None" option as the first entry in the dropdown
	LLOptions.Add(MakeShared<FString>(TEXT("None")));

	// Use AssetRegistry to find all assets of the given class inside a specific folder
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetData;

	// Define the folder path you want to search in (your plugin content folder)
	const FName PluginContentFolder = "/BelindaVPTool"; // Replace with the actual path to your plugin's content folder

	// Create an asset registry filter
	FARFilter Filter;
	Filter.ClassPaths.Add(ClassType->GetClassPathName()); // Filter by the specific class type (e.g., ULiveLinkPreset)
	Filter.PackagePaths.Add(PluginContentFolder);         // Only search inside the plugin folder
	Filter.bRecursivePaths = true;                        // Search recursively within the folder

	// Get assets matching the filter
	AssetRegistryModule.Get().GetAssets(Filter, AssetData);
	liveLinkAssetsDatas.Empty();

	for (const FAssetData& Asset : AssetData)
	{
		LLOptions.Add(MakeShared<FString>(Asset.AssetName.ToString()));
		FConfigAssetDatas tempDatas;
		tempDatas.name = Asset.AssetName.ToString();
		tempDatas.path = Asset.GetSoftObjectPath().ToString();
		liveLinkAssetsDatas.Add(tempDatas);
	}

	if (!LLOptions.IsEmpty())
		SelectedLL = LLOptions[0];
}

void FBelindaVPToolEditorModule::FindPresetMOOfClass(UClass* ClassType)
{
	//TArray<TSharedPtr<FString>> bpOptions;
	MOBlueprintOptions.Empty();

	// Add "None" option as the first entry in the dropdown
	MOBlueprintOptions.Add(MakeShared<FString>(TEXT("None")));

	// Use AssetRegistry to find all assets of the given class inside a specific folder
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetData;

	// Define the folder path you want to search in (your plugin content folder)
	const FName PluginContentFolder = "/BelindaVPTool"; // Replace with the actual path to your plugin's content folder

	// Create an asset registry filter
	FARFilter Filter;
	Filter.ClassPaths.Add(ClassType->GetClassPathName()); // Filter by the specific class type (e.g., ULiveLinkPreset)
	Filter.PackagePaths.Add(PluginContentFolder);         // Only search inside the plugin folder
	Filter.bRecursivePaths = true;                        // Search recursively within the folder

	// Get assets matching the filter
	AssetRegistryModule.Get().GetAssets(Filter, AssetData);
	MOAssetsDatas.Empty();

	for (const FAssetData& Asset : AssetData)
	{
		MOBlueprintOptions.Add(MakeShared<FString>(Asset.AssetName.ToString()));
		FConfigAssetDatas tempDatas;
		tempDatas.name = Asset.AssetName.ToString();
		tempDatas.path = Asset.GetSoftObjectPath().ToString();
		MOAssetsDatas.Add(tempDatas);
	}

	UE_LOG(LogTemp, Warning, TEXT("FOUND MO : %i "), MOBlueprintOptions.Num());

	for (TSharedPtr<FString> option : MOBlueprintOptions)
	{
		UE_LOG(LogTemp, Warning, TEXT("FOUND OPTION : %s "), **option);

	}

	if (!MOBlueprintOptions.IsEmpty())
		MOSelectedBlueprint = MOBlueprintOptions[0];
}

void FBelindaVPToolEditorModule::OnTCBlueprintSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	// Handle blueprint selection
	if (NewSelection.IsValid() && *NewSelection == "None")
	{
		// Handle None option (deselect blueprint)
		TCSelectedBlueprint.Reset();
		UE_LOG(LogTemp, Log, TEXT("None option selected. No blueprint selected."));
	}
	else
	{
		TCSelectedBlueprint = NewSelection;
		UE_LOG(LogTemp, Log, TEXT("Selected Blueprint: %s"), **NewSelection);
	}
}

FText FBelindaVPToolEditorModule::GetSelectedTCBlueprintItem() const
{
	if (TCSelectedBlueprint.IsValid())
	{
		return FText::FromString(*TCSelectedBlueprint);
	}

	return FText::FromString(TEXT("None"));  // Default to "None" if nothing is selected
}

void FBelindaVPToolEditorModule::OnTSBlueprintSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	// Handle blueprint selection
	if (NewSelection.IsValid() && *NewSelection == "None")
	{
		// Handle None option (deselect blueprint)
		TSSelectedBlueprint.Reset();
		UE_LOG(LogTemp, Log, TEXT("None option selected. No blueprint selected."));
	}
	else
	{
		TSSelectedBlueprint = NewSelection;
		UE_LOG(LogTemp, Log, TEXT("Selected Blueprint: %s"), **NewSelection);
	}
}

FText FBelindaVPToolEditorModule::GetSelectedTSBlueprintItem() const
{
	if (TSSelectedBlueprint.IsValid())
	{
		return FText::FromString(*TSSelectedBlueprint);
	}

	return FText::FromString(TEXT("None"));  // Default to "None" if nothing is selected
}

void FBelindaVPToolEditorModule::OnMOBlueprintSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	// Handle blueprint selection
	if (NewSelection.IsValid() && *NewSelection == "None")
	{
		// Handle None option (deselect blueprint)
		MOSelectedBlueprint.Reset();
		UE_LOG(LogTemp, Log, TEXT("None option selected. No blueprint selected."));
	}
	else
	{
		MOSelectedBlueprint = NewSelection;
		UE_LOG(LogTemp, Log, TEXT("Selected Blueprint: %s"), **NewSelection);
	}
}

FText FBelindaVPToolEditorModule::GetSelectedMOBlueprintItem() const
{
	if (MOSelectedBlueprint.IsValid())
	{
		return FText::FromString(*MOSelectedBlueprint);
	}

	return FText::FromString(TEXT("None"));  // Default to "None" if nothing is selected
}

void FBelindaVPToolEditorModule::OnLLSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	// Handle blueprint selection
	if (NewSelection.IsValid() && *NewSelection == "None")
	{
		// Handle None option (deselect blueprint)
		SelectedLL.Reset();
		UE_LOG(LogTemp, Log, TEXT("None option selected. No blueprint selected."));
	}
	else
	{
		for (FConfigAssetDatas data : liveLinkAssetsDatas)
		{
			if (*NewSelection == data.name)
			{
				UE_LOG(LogTemp, Warning, TEXT("Changed to new path : %s"), *data.path);
			}
		}
		SelectedLL = NewSelection;
		UE_LOG(LogTemp, Log, TEXT("Selected Blueprint: %s"), **NewSelection);
	}
}

FText FBelindaVPToolEditorModule::GetSelectedLLItem() const
{
	if (SelectedLL.IsValid())
	{
		return FText::FromString(*SelectedLL);
	}

	return FText::FromString(TEXT("None"));  // Default to "None" if nothing is selected
}

bool FBelindaVPToolEditorModule::IsGenTCCheckboxEnabled() const
{
	return bGenerateDefaultTC;  // bIsFeatureAvailable could be a boolean flag
}

bool FBelindaVPToolEditorModule::CheckSpawnCameraState() const
{
	// Ensure we are using the correct path (with _C for the Blueprint Class)
	FString BlueprintPath = TEXT("/BelindaVPTool/Blueprints/BP_CameraManager.BP_CameraManager_C");

	// Load the Blueprint class
	UClass* BlueprintClass = LoadBlueprintClassByPath(BlueprintPath);
	if (!BlueprintClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load Blueprint class at path: %s"), *BlueprintPath);
		return false;
	}

	bIsCameraPresent = FindActorsOfClass(GetCurrentWorld(), BlueprintClass);

	return !bIsCameraPresent;
}

bool FBelindaVPToolEditorModule::CheckSpawncompCameraState() const
{
	// Ensure we are using the correct path (with _C for the Blueprint Class)
	FString BlueprintPath = TEXT("/BelindaVPTool/VProdTools/BP_CompCam.BP_CompCam_C");

	// Load the Blueprint class
	UClass* BlueprintClass = LoadBlueprintClassByPath(BlueprintPath);
	if (!BlueprintClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load Blueprint class at path: %s"), *BlueprintPath);
		return false;
	}

	bIsCompCameraPresent = FindActorsOfClass(GetCurrentWorld(), BlueprintClass);

	return !bIsCompCameraPresent;
}

bool FBelindaVPToolEditorModule::CheckSpawnNDCameraState() const
{
	// Ensure we are using the correct path (with _C for the Blueprint Class)
	FString BlueprintPath = TEXT("/BelindaVPTool/NDisplayTools/BP_NDCam.BP_NDCam_C");

	// Load the Blueprint class
	UClass* BlueprintClass = LoadBlueprintClassByPath(BlueprintPath);
	if (!BlueprintClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load Blueprint class at path: %s"), *BlueprintPath);
		return false;
	}

	bIsNDCameraPresent = FindActorsOfClass(GetCurrentWorld(), BlueprintClass);

	return !bIsNDCameraPresent;
}


UWorld* FBelindaVPToolEditorModule::GetCurrentWorld() const
{
	// Get the world context from the game viewport
	const FWorldContext* WorldContext = GEngine->GetWorldContextFromGameViewport(GEngine->GameViewport);
	if (!WorldContext || !WorldContext->World())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get valid world context."));
		return nullptr;
	}

	return WorldContext->World();
}

bool FBelindaVPToolEditorModule::CheckCameraPresence() const
{
	return bIsCameraPresent;
}

bool FBelindaVPToolEditorModule::CheckCompCameraPresence() const
{
	return bIsCompCameraPresent;
}

bool FBelindaVPToolEditorModule::CheckNDCameraPresence() const
{
	return bIsNDCameraPresent;
}

bool FBelindaVPToolEditorModule::FindActorsOfClass(UWorld* World, UClass* ActorClass) const
{
	if (World && ActorClass)
	{
		for (TActorIterator<AActor> It(World, ActorClass); It; ++It)
		{
			AActor* FoundActor = *It;
			if (FoundActor)
			{
				//UE_LOG(LogTemp, Warning, TEXT("Found Actor: %s"), *FoundActor->GetName());
				return true;
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("World or ActorClass is invalid."));
	}
	//UE_LOG(LogTemp, Warning, TEXT("Actor not Found !!"));
	return false;
}

AActor* FBelindaVPToolEditorModule::GetFirstActorOfClass(UWorld* World, UClass* ActorClass) const
{
	if (World && ActorClass)
	{
		for (TActorIterator<AActor> It(World, ActorClass); It; ++It)
		{
			AActor* FoundActor = *It;
			if (FoundActor)
			{
				
				//UE_LOG(LogTemp, Warning, TEXT("Found Actor: %s"), *FoundActor->GetName());
				return FoundActor;
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("World or ActorClass is invalid."));
	}
	//UE_LOG(LogTemp, Warning, TEXT("Actor not Found !!"));
	return nullptr;
}

bool FBelindaVPToolEditorModule::DestroyActorsOfClass(UWorld* World, UClass* ActorClass) const
{
	if (World && ActorClass)
	{
		for (TActorIterator<AActor> It(World, ActorClass); It; ++It)
		{
			AActor* FoundActor = *It;
			if (FoundActor)
			{
				FoundActor->Destroy();
				//UE_LOG(LogTemp, Warning, TEXT("Found Actor: %s"), *FoundActor->GetName());
				//return true;
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("World or ActorClass is invalid."));
	}
	//UE_LOG(LogTemp, Warning, TEXT("Actor not Found !!"));
	return false;
}

UClass* FBelindaVPToolEditorModule::LoadBlueprintClassByPath(const FString& BlueprintPath) const
{
	// This function expects the full path to the blueprint, including "_C" for the class
	return StaticLoadClass(UObject::StaticClass(), nullptr, *BlueprintPath);
}

void FBelindaVPToolEditorModule::OnTCCheckboxStateChanged(ECheckBoxState NewState)
{
	switch (NewState)
	{
	case ECheckBoxState::Unchecked:
		bGenerateDefaultTC = false;
		break;
	case ECheckBoxState::Checked:
		bGenerateDefaultTC = true;
		break;
	case ECheckBoxState::Undetermined:
		bGenerateDefaultTC = false;
		break;
	default:
		bGenerateDefaultTC = false;
		break;
	}

	// Checkbox state changed logic
	UE_LOG(LogTemp, Warning, TEXT("Checkbox State Changed!"));
}

void FBelindaVPToolEditorModule::OnDropdownSelectionChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
{
	SelectedDropdownItem = NewValue;
}

TSharedRef<SWidget> FBelindaVPToolEditorModule::GenerateDropdownItem(TSharedPtr<FString> InItem)
{
	return SNew(STextBlock)
		.Text(FText::FromString(*InItem));
}

TSharedRef<SWidget> FBelindaVPToolEditorModule::GenerateDropdownItem2(TSharedPtr<FString> InItem)
{
	UE_LOG(LogTemp, Warning, TEXT("ADDED : %s"), **InItem);
	return SNew(STextBlock)
		.Text(FText::FromString(*InItem));
}

FText FBelindaVPToolEditorModule::GetCurrentDropdownItem() const
{
	if (!DropdownOptions.Num())  // Check if there are any options
	{
		//UE_LOG(LogTemp, Error, TEXT("DropdownOptions array is empty!"));
		return FText::FromString("No options available");
	}

	if (SelectedDropdownItem.IsValid() && SelectedDropdownItem.Get() != nullptr)
	{
		//UE_LOG(LogTemp, Log, TEXT("Selected item is valid: %s"), **SelectedDropdownItem);
		return FText::FromString(*SelectedDropdownItem);
	}

	// Handle invalid selection
	//UE_LOG(LogTemp, Warning, TEXT("SelectedDropdownItem is invalid! Defaulting to placeholder text."));
	return FText::FromString("Select an option");
}

const FSlateBrush* FBelindaVPToolEditorModule::GetImageBrush(BrushType toUseType) const
{
	UTexture2D* Texture = nullptr;

	switch (toUseType)
	{
	case BrushType::LOGO:
		Texture = LoadObject<UTexture2D>(nullptr, TEXT("/BelindaVPTool/Textures/T_logo.T_logo"));
		break;
	case BrushType::LOGO_TXT:
		Texture = LoadObject<UTexture2D>(nullptr, TEXT("/BelindaVPTool/Textures/T_logo_txt_W.T_logo_txt_W")); ;
		break;
	default:
		Texture = LoadObject<UTexture2D>(nullptr, TEXT("/BelindaVPTool/Textures/T_logo.T_logo"));
		break;
	}


	if (Texture)
	{
		Texture->AddToRoot();  // Ensure it doesn't get garbage collected
		Texture->UpdateResource();  // Force it to load if necessary

		FSlateBrush* Brush = new FSlateBrush();
		Brush->SetResourceObject(Texture);  // Set the texture
		Brush->ImageSize = FVector2D(256, 256);  // Define the size of the image

		return Brush;  // Return the brush with the texture
	}

	// If texture not found, return nullptr
	return nullptr;
}

FReply FBelindaVPToolEditorModule::OnButtonClick(BtnType btnType)
{
	// Button click logic goes here
	FText outText = FText::FromString("");
	switch (btnType)
	{
	case BtnType::APPLY:
		outText = FText::FromString("APPLY SETTINGS CLICKED !");
		ApplyUserProjectSettings();
		break;
	case BtnType::DLT_SETS:
		outText = FText::FromString("REMOVE SETTINGS CLICKED !");
		ResetProjectSettings();
		break;
	case BtnType::ADD_CAM:
		outText = FText::FromString("ADD CAM CLICKED !");
		break;
	case BtnType::DLT_CAM:
		outText = FText::FromString("DLT CAM CLICKED !");
		CleanScene();
		break;
	case BtnType::SLCT_CAM:
		outText = FText::FromString("SLCT CAM CLICKED !");
		if (spawnedCam->IsValidLowLevel())
			FocusAndSelect(spawnedCam);
		break;
	case BtnType::SLCT_CAMMAN:
		outText = FText::FromString("SLCT CAMMAN CLICKED !");
		if (spawnedCamMan->IsValidLowLevel())
			FocusAndSelect(spawnedCamMan);
		break;
	case BtnType::DLT_COMPCAM:
		outText = FText::FromString("DLT CAM CLICKED !");
		CleanCompScene();
		break;
	case BtnType::SLCT_COMPCAM:
		outText = FText::FromString("SLCT CAM CLICKED !");
		FocusAndSelectCompCam();
		break;
	case BtnType::SLCT_COMPCAMMAN:
		outText = FText::FromString("SLCT CAMMAN CLICKED !");
		FocusAndSelectCompCamMan();
		break;
	case BtnType::DLT_NDCAM:
		outText = FText::FromString("DLT CAM CLICKED !");
		CleanNDScene();
		break;
	case BtnType::SLCT_NDCAM:
		outText = FText::FromString("SLCT CAM CLICKED !");
		FocusAndSelectNDCam();
		break;
	case BtnType::SLCT_NDCAMMAN:
		outText = FText::FromString("SLCT CAMMAN CLICKED !");
		FocusAndSelectNDCamMan();
		break;
	case BtnType::PILOT_CAM:
		outText = FText::FromString("PILOT CAM CLICKED !");
		PilotCamera();
		pilotCamBtnTxt = bPilotCam ? FText::FromString("Eject camera") : FText::FromString("Pilot camera");
		break;
	case BtnType::SET_MC:
		ApplyMediaCapture();
		break;
	case BtnType::STP_CAM:
		SetupCam();
		break;
	case BtnType::PLC_OBJ:
		PlaceAtObject();
		break;
	case BtnType::FCS_OBJ:
		FocusOnObject();
		break;
	default:
		outText = FText::FromString("DEFAULT");
		break;
	}

	UE_LOG(LogTemp, Warning, TEXT("%s"), *outText.ToString());

	return FReply::Handled();
}

void FBelindaVPToolEditorModule::FocusAndSelectCompCam()
{
	FString BlueprintPath = TEXT("/BelindaVPTool/VProdTools/BP_CompCam.BP_CompCam_C");
	UClass* BlueprintClass = LoadBlueprintClassByPath(BlueprintPath);
	AActor* tempCompCam = GetFirstActorOfClass(GetCurrentWorld(), BlueprintClass);

	if (tempCompCam->IsValidLowLevel())
	{
		FocusAndSelect(tempCompCam);
	}
}

void FBelindaVPToolEditorModule::FocusAndSelectCompCamMan()
{
	FString BlueprintPath = TEXT("/BelindaVPTool/VProdTools/BP_Turntable.BP_Turntable_C");
	UClass* BlueprintClass = LoadBlueprintClassByPath(BlueprintPath);
	AActor* tempCompCam = GetFirstActorOfClass(GetCurrentWorld(), BlueprintClass);

	if (tempCompCam->IsValidLowLevel())
	{
		FocusAndSelect(tempCompCam);
	}
}

void FBelindaVPToolEditorModule::FocusAndSelectNDCam()
{
	FString BlueprintPath = TEXT("/BelindaVPTool/NDisplayTools/BP_NDCam.BP_NDCam_C");
	UClass* BlueprintClass = LoadBlueprintClassByPath(BlueprintPath);
	AActor* tempCompCam = GetFirstActorOfClass(GetCurrentWorld(), BlueprintClass);

	if (tempCompCam->IsValidLowLevel())
	{
		FocusAndSelect(tempCompCam);
	}
}

void FBelindaVPToolEditorModule::FocusAndSelectNDCamMan()
{
	FString BlueprintPath = TEXT("/BelindaVPTool/VProdTools/NDC_CustomBasic.NDC_CustomBasic_C");
	UClass* BlueprintClass = LoadBlueprintClassByPath(BlueprintPath);
	AActor* tempCompCam = GetFirstActorOfClass(GetCurrentWorld(), BlueprintClass);

	if (tempCompCam->IsValidLowLevel())
	{
		FocusAndSelect(tempCompCam);
	}
}

void FBelindaVPToolEditorModule::RegisterMenuExtensions()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu(
		"LevelEditor.LevelEditorToolBar.User");
	FToolMenuSection& ToolbarSection = ToolbarMenu->FindOrAddSection("File");

	// Use the custom icon defined in FMyEditorStyle
	FSlateIcon BelindaIcon = FSlateIcon(FMyEditorStyle::GetStyleSetName(), "MyStyle.Icon");

	ToolbarSection.AddEntry(FToolMenuEntry::InitToolBarButton(
		TEXT("Belinda VP Tools"),
		FExecuteAction::CreateLambda([&]()
			{
				//FString WidgetPath = TEXT("/BelindaVPTool/UWE_SetupTool.UWE_SetupTool");
				//FBelindaVPToolEditorModule::RunEditorUtilityWidget(WidgetPath);
				OnMenuButtonClicked();

			}),
		INVTEXT("Belinda VP Tools"),
		INVTEXT("Belinda VP Tools"),
		BelindaIcon // Use the custom icon
	));
}

void FBelindaVPToolEditorModule::OnFirstCheckboxChanged(ECheckBoxState NewState)
{
	if (NewState == ECheckBoxState::Checked)
	{
		FirstCheckboxState = ECheckBoxState::Checked;
		SecondCheckboxState = ECheckBoxState::Unchecked;
		UE_LOG(LogTemp, Error, TEXT("Object not selected !"));
	}
	else
	{
		FirstCheckboxState = ECheckBoxState::Unchecked;
		UE_LOG(LogTemp, Error, TEXT("Object  selected !"));

	}
}

void FBelindaVPToolEditorModule::OnSecondCheckboxChanged(ECheckBoxState NewState)
{
	if (NewState == ECheckBoxState::Checked)
	{
		SecondCheckboxState = ECheckBoxState::Checked;
		FirstCheckboxState = ECheckBoxState::Unchecked;
	}
	else
	{
		SecondCheckboxState = ECheckBoxState::Unchecked;
	}
}

void FBelindaVPToolEditorModule::RunEditorUtilityWidget(FString WidgetPath)
{
	// Get the Editor Utility Subsystem
	UEditorUtilitySubsystem* EditorUtilitySubsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();

	if (EditorUtilitySubsystem)
	{
		// Load the Editor Utility Widget Blueprint by its path
		UEditorUtilityWidgetBlueprint* WidgetBP = LoadObject<UEditorUtilityWidgetBlueprint>(nullptr, *WidgetPath);

		if (WidgetBP)
		{
			// Spawn and register the widget as a tab in the editor
			EditorUtilitySubsystem->SpawnAndRegisterTab(WidgetBP);

			UE_LOG(LogTemp, Log, TEXT("Successfully loaded and displayed the widget: %s"), *WidgetPath);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to load the widget blueprint: %s"), *WidgetPath);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get Editor Utility Subsystem."));
	}
}

void FBelindaVPToolEditorModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(
		FText::FromString("Belinda Virtual Production tools"),
		FText::FromString("Opens Belinda Virtual Production tools window."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FBelindaVPToolEditorModule::OnMenuButtonClicked))
	);
}

void FBelindaVPToolEditorModule::OnMenuButtonClicked()
{
	OnFillArrays();
	FGlobalTabmanager::Get()->TryInvokeTab(FName("VPTools"));
}

void FBelindaVPToolEditorModule::OnFillArrays()
{
	// Populate blueprint options, including "None" as the default option
	FindTCBlueprintsOfClass(UGenlockedTimecodeProvider::StaticClass());


	// Populate blueprint options, including "None" as the default option
	FindTSBlueprintsOfClass(UGenlockedCustomTimeStep::StaticClass());

	// Populate blueprint options, including "None" as the default option
	FindPresetOfClass(ULiveLinkPreset::StaticClass());


	FindMOBlueprintsOfClass(UMediaOutput::StaticClass());
	//FindPresetMOOfClass(UMediaOutput::StaticClass());
}

FReply FBelindaVPToolEditorModule::SpawnRig(BtnType btnType)
{
	if (!GetCurrentWorld() || !GetCurrentWorld()->IsValidLowLevel())
		return FReply::Unhandled();

	// Ensure we are using the correct path (with _C for the Blueprint Class)
	FString BlueprintPath = TEXT("/BelindaVPTool/Blueprints/BP_CameraManager.BP_CameraManager_C");

	// Load the Blueprint class
	UClass* BlueprintClass = LoadBlueprintClassByPath(BlueprintPath);
	if (!BlueprintClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load Blueprint class at path: %s"), *BlueprintPath);
		return FReply::Unhandled();
	}

	// Spawn parameters (optional but recommended)
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// Define spawn location and rotation (example uses zero vector and zero rotator)
	FVector SpawnLocation = FVector(0.f, 0.f, 100.f);
	FRotator SpawnRotation = FRotator::ZeroRotator;

	// Spawn the actor from the loaded Blueprint class
	AActor* SpawnedActor = GetCurrentWorld()->SpawnActor<AActor>(BlueprintClass, SpawnLocation, SpawnRotation, SpawnParams);

	if (SpawnedActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Successfully spawned actor!"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to spawn actor from Blueprint class."));
	}
	return FReply::Handled();
}

AActor* FBelindaVPToolEditorModule::SpawnActor(UClass* toSpawnClass, FVector location)
{
	if (!toSpawnClass)
	{
		//UE_LOG(LogTemp, Error, TEXT("Failed to load Blueprint class at path: %s"), *toSpawnClass->GetClass()->GetDefaultObjectName().ToString());
		return nullptr;
	}

	// Spawn parameters (optional but recommended)
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// Define spawn location and rotation (example uses zero vector and zero rotator)
	FVector SpawnLocation = location;
	FRotator SpawnRotation = FRotator::ZeroRotator;

	// Spawn the actor from the loaded Blueprint class
	AActor* SpawnedActor = GetCurrentWorld()->SpawnActor<AActor>(toSpawnClass, SpawnLocation, SpawnRotation, SpawnParams);

	if (SpawnedActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Successfully spawned actor!"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to spawn actor from Blueprint class."));
		return nullptr;
	}
	return SpawnedActor;
}

FReply FBelindaVPToolEditorModule::SpawnAndSetupRig(BtnType btnType)
{
	if (!GetCurrentWorld() || !GetCurrentWorld()->IsValidLowLevel())
		return FReply::Unhandled();

	CleanScene();
	if (btnType == BtnType::ADD_CAM)
	{
		// Ensure we are using the correct path (with _C for the Blueprint Class)
		FString BlueprintPath = TEXT("/BelindaVPTool/Blueprints/BP_CameraManager.BP_CameraManager_C");
		// Load the Blueprint class
		UClass* BlueprintClass = LoadBlueprintClassByPath(BlueprintPath);

		TArray<FVector> viewLocations = GetCurrentWorld()->ViewLocationsRenderedLastFrame;
		FVector camLocation = FVector(0.0f, 0.0f, 100.0f);
		if (!viewLocations.IsEmpty())
			camLocation = viewLocations[0];

		spawnedCamMan = SpawnActor(BlueprintClass, camLocation);

		BlueprintPath = TEXT("/BelindaVPTool/Blueprints/BP_MainCameraRobot.BP_MainCameraRobot_C");
		BlueprintClass = LoadBlueprintClassByPath(BlueprintPath);

		spawnedCam = SpawnActor(BlueprintClass);

		BlueprintPath = TEXT("/BelindaVPTool/Blueprints/BP_Probe.BP_Probe_C");
		BlueprintClass = LoadBlueprintClassByPath(BlueprintPath);

		spawnedProbe = SpawnActor(BlueprintClass);



		if (spawnedCamMan->IsValidLowLevel() || spawnedCam->IsValidLowLevel())
		{
			FTimerHandle UnusedHandle;
			GetCurrentWorld()->GetTimerManager().SetTimer(UnusedHandle, [&]()
				{
					CallFunctionByName(spawnedCam, TEXT("FixLiveLink"));
					CallFunctionByName(spawnedCamMan, TEXT("SetupCamera"));
					FocusAndSelect(spawnedCamMan);
				}, 0.1f, false);
		}
	}
	else if (btnType == BtnType::ADD_COMPCAM)
	{
		SpawnCompCam();
	}
	else if (btnType == BtnType::ADD_NDCAM)
	{
		SpawnNDCam();
	}

	return FReply::Handled();

}

void FBelindaVPToolEditorModule::FocusAndSelect(AActor* objectToFocus)
{
	if (!objectToFocus->IsValidLowLevel())
		return;
	if (GEditor)
	{
		//ULevelEditorSubsystem* LevelEditorSubsystem = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>();

		//if (LevelEditorSubsystem)
		//{
		//	LevelEditorSubsystem->PilotLevelActor(objectToFocus);
		//}

		GEditor->SelectNone(true, true);

		UE_LOG(LogTemp, Error, TEXT("Object selected !"));
		GEditor->SelectActor(objectToFocus, true, true, true, true);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Object not selected !"));
	}


	if (GEngine)
		GEngine->Exec(GetCurrentWorld(), TEXT("CAMERA ALIGN"));
}

void FBelindaVPToolEditorModule::CleanScene()
{
	if (!GetCurrentWorld() || !GetCurrentWorld()->IsValidLowLevel())
		return;


	UE_LOG(LogTemp, Error, TEXT("CLEAN SCENE !!!!!"));

	FString BlueprintPath = TEXT("/BelindaVPTool/Blueprints/BP_CameraManager.BP_CameraManager_C");
	UClass* BlueprintClass = LoadBlueprintClassByPath(BlueprintPath);
	if (BlueprintClass->IsValidLowLevel())
	{
		DestroyActorsOfClass(GetCurrentWorld(), BlueprintClass);
	}

	BlueprintPath = TEXT("/BelindaVPTool/Blueprints/BP_MainCameraRobot.BP_MainCameraRobot_C");
	BlueprintClass = LoadBlueprintClassByPath(BlueprintPath);
	if (BlueprintClass->IsValidLowLevel())
	{
		DestroyActorsOfClass(GetCurrentWorld(), BlueprintClass);
	}

	BlueprintPath = TEXT("/BelindaVPTool/Blueprints/BP_Probe.BP_Probe_C");
	BlueprintClass = LoadBlueprintClassByPath(BlueprintPath);
	if (BlueprintClass->IsValidLowLevel())
	{
		DestroyActorsOfClass(GetCurrentWorld(), BlueprintClass);
	}
}


void FBelindaVPToolEditorModule::CleanCompScene()
{
	if (!GetCurrentWorld() || !GetCurrentWorld()->IsValidLowLevel())
		return;


	UE_LOG(LogTemp, Error, TEXT("CLEAN SCENE !!!!!"));
	RemoveCompCam();
}

void FBelindaVPToolEditorModule::CleanNDScene()
{
	if (!GetCurrentWorld() || !GetCurrentWorld()->IsValidLowLevel())
		return;


	UE_LOG(LogTemp, Error, TEXT("CLEAN SCENE !!!!!"));
	RemoveNDCam();
}

bool FBelindaVPToolEditorModule::CallFunctionByName(UObject* Object, FName FunctionName)
{
	if (Object)
	{
		if (UFunction* Function = Object->FindFunction(FunctionName))
		{
			Object->ProcessEvent(Function, nullptr);
			return true;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Trying to run function named %s but is not found"), *FunctionName.ToString());
			return false;
		}

	}
	return false;
}

bool FBelindaVPToolEditorModule::CallFunctionByNameWWorld(UObject* Object, FName FunctionName)
{
	if (Object)
	{
		if (UFunction* Function = Object->FindFunction(FunctionName))
		{
			// Check if the function requires parameters, like a world context object.
			// If it does, you'll need to provide the necessary arguments to ProcessEvent.
			// For example, let's assume the world context object is expected as the first parameter.

			struct FFunctionParams
			{
				UObject* WorldContext; // Add more fields as needed for other parameters
				UMediaOutput* MediaOutputFile;
				bool bUseMethod1;
			};

			FFunctionParams Params;
			Params.WorldContext = GetCurrentWorld();
			for (int i = 0; i < MOAssetsDatas.Num(); i++)
			{
				if (MOAssetsDatas[i].name == GetSelectedMOBlueprintItem().ToString())
				{
					Params.MediaOutputFile = moFiles[i];
					break;
				}
			}
			if (FirstCheckboxState == ECheckBoxState::Checked)
			{
				Params.bUseMethod1 = true;
			}
			else
			{
				Params.bUseMethod1 = false;
			}

			Object->ProcessEvent(Function, &Params); // Pass the parameters to ProcessEvent
			return true;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Trying to run function named %s but is not found"), *FunctionName.ToString());
			return false;
		}
	}
	return false;
}

void FBelindaVPToolEditorModule::PilotCamera()
{

	if (!IsValid(spawnedCam))
		return;
	if (!bPilotCam)
	{
		ULevelEditorSubsystem* LevelEditorSubsystem = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>();

		if (LevelEditorSubsystem)
		{
			GEditor->SelectNone(true, true);


			GEditor->SelectActor(spawnedCam, true, true, true, true);

			LevelEditorSubsystem->PilotLevelActor(spawnedCam);
			bPilotCam = true;
		}
	}
	else
	{
		EjectCamera();
	}
}

void FBelindaVPToolEditorModule::EjectCamera()
{

	if (!IsValid(spawnedCam))
		return;

	ULevelEditorSubsystem* LevelEditorSubsystem = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>();

	if (LevelEditorSubsystem)
	{
		GEditor->SelectNone(true, true);


		GEditor->SelectActor(spawnedCam, true, true, true, true);
		LevelEditorSubsystem->EjectPilotLevelActor();
		bPilotCam = false;
	}
}

void FBelindaVPToolEditorModule::ApplyUserProjectSettings()
{
	FProjectSettingsDatas userDatas;

	for (FConfigAssetDatas tcData : TCAssetsDatas)
	{
		if (tcData.name == GetSelectedTCBlueprintItem().ToString())
		{
			userDatas.tcPath = tcData.path;
			UE_LOG(LogTemp, Warning, TEXT("USER DATA TC PATH is: %s"), *userDatas.tcPath);
			break;
		}
	}

	for (FConfigAssetDatas tsData : TSAssetsDatas)
	{
		if (tsData.name == GetSelectedTSBlueprintItem().ToString())
		{
			userDatas.tsPath = tsData.path;
			break;
		}
	}

	for (FConfigAssetDatas llData : liveLinkAssetsDatas)
	{
		if (llData.name == GetSelectedLLItem().ToString())
		{
			userDatas.LLPath = llData.path;
			break;
		}
	}


	userDatas.genTC = bGenerateDefaultTC;
	FString returnVal = bGenerateDefaultTC ? "true" : "false";

	UE_LOG(LogTemp, Warning, TEXT("Bool value is: %s"), *returnVal);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Black, FString::Printf(TEXT("Bool: %s"), userDatas.genTC ? TEXT("true") : TEXT("false")));
	}


	if (userDatas.genTC)
	{
		userDatas.framerateTC = GetCurrentDropdownItem().ToString();
	}

	//UE_LOG(LogTemp, Warning, TEXT("FPS Selected : %s"), *GetCurrentDropdownItem().ToString());
	//UE_LOG(LogTemp, Warning, TEXT("FPS Selected : %s"), *GetCurrentDropdownItem().ToString());

	UVPEdtiorToolsLib::ApplyProjectSettings(userDatas);

	//FName LiveLinkWindowName("LiveLinkHubMainTab");
	//FGlobalTabmanager::Get()->TryInvokeTab(LiveLinkWindowName);
	//FName MediaCaptureWindowName("MediaCapture");
	//FGlobalTabmanager::Get()->TryInvokeTab(MediaCaptureWindowName);
	//userDatas.framerateTC =

}

void FBelindaVPToolEditorModule::ResetProjectSettings()
{
	FProjectSettingsDatas userDatas;


	userDatas.tcPath = "";


	userDatas.tsPath = "";

	userDatas.LLPath = "";


	userDatas.genTC = false;

	UVPEdtiorToolsLib::ApplyProjectSettings(userDatas);

}

void FBelindaVPToolEditorModule::SetupCam()
{
	if (spawnedCamMan->IsValidLowLevel() || spawnedCam->IsValidLowLevel())
	{
		FTimerHandle UnusedHandle;
		GetCurrentWorld()->GetTimerManager().SetTimer(UnusedHandle, [&]()
			{
				CallFunctionByName(spawnedCamMan, TEXT("SetupCamera"));
			}, 0.1f, false);
	}
}

void FBelindaVPToolEditorModule::PlaceAtObject()
{
	if (GEditor)
	{
		if (GEditor->GetSelectedActorCount() == 1)
		{
			if (spawnedCamMan->IsValidLowLevel() || spawnedCam->IsValidLowLevel())
			{
				if (GEditor)
				{
					// Get the list of selected actors
					USelection* SelectedActors = GEditor->GetSelectedActors();

					if (SelectedActors && SelectedActors->Num() > 0)
					{
						// Retrieve the first selected actor
						AActor* FirstSelectedActor = Cast<AActor>(SelectedActors->GetSelectedObject(0));

						if (FirstSelectedActor)
						{
							UE_LOG(LogTemp, Warning, TEXT("First selected actor: %s"), *FirstSelectedActor->GetName());
							GEditor->SelectNone(true, true);

							UE_LOG(LogTemp, Error, TEXT("Object selected !"));
							GEditor->SelectActor(FirstSelectedActor, true, true, true, true);
							spawnedCamMan->SetActorLocation(FirstSelectedActor->GetActorLocation());
						}
					}


					FTimerHandle UnusedHandle;
					GetCurrentWorld()->GetTimerManager().SetTimer(UnusedHandle, [&]()
						{
							CallFunctionByName(spawnedCamMan, TEXT("SetupCamera"));
						}, 0.1f, false);


					if (IsValid(spawnedCam))
						FocusAndSelect(spawnedCam);
				}
			}
		}
	}
}

void FBelindaVPToolEditorModule::FocusOnObject()
{
	if (GEngine)
		GEngine->Exec(GetCurrentWorld(), TEXT("CAMERA ALIGN"));
}

void FBelindaVPToolEditorModule::ApplyMediaCapture()
{
	//UMediaFrameworkCapturePanel::AddViewportCapture()
	// Ensure we are using the correct path (with _C for the Blueprint Class)
	FString BlueprintPath = TEXT("/BelindaVPTool/Blueprints/EUB_EditorFunctions.EUB_EditorFunctions_C");
	// Load the Blueprint class
	UClass* BlueprintClass = LoadBlueprintClassByPath(BlueprintPath);

	CallFunctionByNameWWorld(BlueprintClass->GetDefaultObject(), TEXT("SetMediaCapture"));
}


void FBelindaVPToolEditorModule::SpawnCompCam()
{
	//UMediaFrameworkCapturePanel::AddViewportCapture()
	// Ensure we are using the correct path (with _C for the Blueprint Class)
	FString BlueprintPath = TEXT("/BelindaVPTool/VProdTools/EUW_SpawnTool.EUW_SpawnTool_C");
	// Load the Blueprint class
	UClass* BlueprintClass = LoadBlueprintClassByPath(BlueprintPath);

	CallFunctionByNameWWorld(BlueprintClass->GetDefaultObject(), TEXT("SpawnRig"));
}

void FBelindaVPToolEditorModule::SpawnNDCam()
{
	//UMediaFrameworkCapturePanel::AddViewportCapture()
	// Ensure we are using the correct path (with _C for the Blueprint Class)
	FString BlueprintPath = TEXT("/BelindaVPTool/VProdTools/EUW_SpawnTool.EUW_SpawnTool_C");
	// Load the Blueprint class
	UClass* BlueprintClass = LoadBlueprintClassByPath(BlueprintPath);

	CallFunctionByNameWWorld(BlueprintClass->GetDefaultObject(), TEXT("SpawnNDRig"));
}

void FBelindaVPToolEditorModule::RemoveCompCam()
{
	//UMediaFrameworkCapturePanel::AddViewportCapture()
	// Ensure we are using the correct path (with _C for the Blueprint Class)
	FString BlueprintPath = TEXT("/BelindaVPTool/VProdTools/EUW_SpawnTool.EUW_SpawnTool_C");
	// Load the Blueprint class
	UClass* BlueprintClass = LoadBlueprintClassByPath(BlueprintPath);

	CallFunctionByNameWWorld(BlueprintClass->GetDefaultObject(), TEXT("DeleteAll"));
}

void FBelindaVPToolEditorModule::RemoveNDCam()
{
	//UMediaFrameworkCapturePanel::AddViewportCapture()
	// Ensure we are using the correct path (with _C for the Blueprint Class)
	FString BlueprintPath = TEXT("/BelindaVPTool/VProdTools/EUW_SpawnTool.EUW_SpawnTool_C");
	// Load the Blueprint class
	UClass* BlueprintClass = LoadBlueprintClassByPath(BlueprintPath);

	CallFunctionByNameWWorld(BlueprintClass->GetDefaultObject(), TEXT("DeleteAllND"));
}


void FBelindaVPToolEditorModule::OnSliderValueChanged(float value)
{
	if (!spawnedCamMan)
		return;

	FRotator rot = spawnedCam->GetActorRotation();
	rot.Yaw = value * 360.0f;
	spawnedCamMan->SetActorRotation(rot);
}

float FBelindaVPToolEditorModule::GetDefaultSliderValue() const
{
	if (!spawnedCamMan)
		return 0.0f;

	return spawnedCam->GetActorRotation().Yaw / 360.0f;
}

#undef LOCTEXT_NAMESPACE

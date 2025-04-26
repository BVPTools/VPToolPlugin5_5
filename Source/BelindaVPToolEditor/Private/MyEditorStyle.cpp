#include "MyEditorStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"

TSharedPtr<FSlateStyleSet> FMyEditorStyle::StyleInstance = nullptr;

#define IMAGE_BRUSH(RelativePath, ...) FSlateImageBrush( FMyEditorStyle::InResources(RelativePath, TEXT(".png")), __VA_ARGS__ )

void FMyEditorStyle::Initialize()
{
    if (!StyleInstance.IsValid())
    {
        StyleInstance = MakeShareable(new FSlateStyleSet("MyEditorStyle"));

        // Set the content root where the resources like icons are located
        FString ContentDir = IPluginManager::Get().FindPlugin(TEXT("BelindaVPTool"))->GetBaseDir() / TEXT("Resources");
        StyleInstance->SetContentRoot(ContentDir);

        // Register your custom icon (32x32 size in this example)
        StyleInstance->Set("MyStyle.Icon", new IMAGE_BRUSH(TEXT("IconName"), FVector2D(32, 32)));

        // Register the style
        FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
    }
}

void FMyEditorStyle::Shutdown()
{
    if (StyleInstance.IsValid())
    {
        FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
        ensure(StyleInstance.IsUnique());
        StyleInstance.Reset();
    }
}

FName FMyEditorStyle::GetStyleSetName()
{
    static FName StyleSetName(TEXT("MyEditorStyle"));
    return StyleSetName;
}

TSharedPtr<ISlateStyle> FMyEditorStyle::Get()
{
    return StyleInstance;
}

FString FMyEditorStyle::InResources(const FString& RelativePath, const TCHAR* Extension)
{
    return StyleInstance->RootToContentDir(RelativePath, Extension);
}

#undef IMAGE_BRUSH
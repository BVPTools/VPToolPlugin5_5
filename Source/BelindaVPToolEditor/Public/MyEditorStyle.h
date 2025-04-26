#pragma once
#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

class FMyEditorStyle final
{
public:
    static void Initialize();
    static void Shutdown();
    static FName GetStyleSetName();
    static TSharedPtr<class ISlateStyle> Get();


private:
    static TSharedPtr<FSlateStyleSet> StyleInstance;
    static FString InResources(const FString& RelativePath, const TCHAR* Extension);
};
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CameraMangagerInterface.h"
#include "EventTriggerComp.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BELINDAVPTOOL_API UEventTriggerComp : public UActorComponent , public ICameraMangagerInterface
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UEventTriggerComp();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void SetupCamera_Implementation() override;
};

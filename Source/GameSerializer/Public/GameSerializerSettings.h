// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Templates/SubclassOf.h"

#include "GameSerializerSettings.generated.h"

/**
 * 
 */
UCLASS( config = Game, defaultconfig, meta = ( DisplayName = "Game Serializer Settings" ) )
class GAMESERIALIZER_API UGameSerializerSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	
	UGameSerializerSettings();

	static UGameSerializerSettings* Get(){ return GetMutableDefault<UGameSerializerSettings>(); }
	
	UPROPERTY( config, EditAnywhere, Category = Classes )
		TSubclassOf< class USavedGameState > SaveClass;

	UPROPERTY( config, EditAnywhere, Category = Serialization )
		bool bAutoLoadGameOnBeginPlay;
};

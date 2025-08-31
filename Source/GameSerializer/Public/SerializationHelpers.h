// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "UObject/NoExportTypes.h"
#include "Classes.h"

#include "SerializationHelpers.generated.h"

/**
 * 
 */
UCLASS()
class GAMESERIALIZER_API USerializationHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

	//Save Object
	UFUNCTION(BlueprintCallable, Category = "Game Serializer")
		static FSerializedActor SaveActor(AActor* actor);

	UFUNCTION( BlueprintCallable, Category = "Game Serializer" )
		static FSerializedGameObject SaveObject( UObject* object );

	UFUNCTION(BlueprintCallable, Category = "Game Serializer")
		static void LoadActor(AActor* actor, FSerializedActor save);

	UFUNCTION( BlueprintCallable, Category = "Game Serializer" )
		static void LoadObject( UObject* object, FSerializedGameObject save );

	UFUNCTION(BlueprintCallable, Category = "Game Serializer", meta = ( WorldContext = "WorldContextObject" ) )
		static AActor* SpawnAndLoadActor(UObject* WorldContextObject, FSerializedActor save);

	UFUNCTION( BlueprintCallable, Category = "Game Serializer" )
		static void SerializeGameWorld() {}

	UFUNCTION( BlueprintPure, Category = "Game Serializer", meta = ( WorldContext = "WorldContextObject" ) )
		static UGameSaveManager* GetGameSaveManager(UObject* WorldContextObject);

	UFUNCTION( BlueprintPure, Category = "Game Serializer" )
		static FName ResolveID( UObject* object );

	UFUNCTION( BlueprintPure, Category = "Game Serializer" )
		static bool IsClassBlacklisted( TSubclassOf< UObject > objectClass );

	UFUNCTION( BlueprintPure, Category = "Game Serializer" )
		static FString GetSlotSaveName( int32 slot ) { return ( "save_" + FString::FromInt( slot ) ); }

	//Load Object

	//Save all objects in scene to packet

	//Load all objects in scene from packet
	
};

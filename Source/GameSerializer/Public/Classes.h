// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "GameFramework/Actor.h"

#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/SaveGame.h"

#include "Classes.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FGameSaveEvent, USaveGame*, save );

namespace SaveTags
{
	static const FName Save = FName( "Save" );
	static const FName Ignore = FName( "Ignore" );
	static const FName IgnoreTransform = FName( "IgnoreTransform" );
}

/**
 * 
 */
class GAMESERIALIZER_API Classes
{
public:
	Classes();
	~Classes();
};

USTRUCT(BlueprintType)
struct FSerializedGameObject
{
	GENERATED_BODY()

public:

	FSerializedGameObject()
	{
		ObjectClass = UObject::StaticClass();
		UniqueId = NAME_None;
	}

	UPROPERTY( VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = Serializer )
		TSubclassOf< UObject > ObjectClass;

	UPROPERTY( VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = Serializer )
		FName UniqueId;

	UPROPERTY( VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = Serializer )
		TArray< uint8 > Data;
};

USTRUCT(BlueprintType)
struct FSerializedActor : public FSerializedGameObject
{
	GENERATED_BODY()

public:

	FSerializedActor()
	{
		ActorClass = AActor::StaticClass();
		ActorTransform = FTransform();
		Data = TArray< uint8 >();
		bWasSpawned = true;
	}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = Serializer)
		TSubclassOf< AActor > ActorClass;

	UPROPERTY( VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = Serializer )
		FTransform ActorTransform;

	UPROPERTY( VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = Serializer )
		uint32 bWasSpawned : 1;

	UPROPERTY( VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = Serializer )
		FName AttachmentPoint;

	static FSerializedActor Null()
	{
		FSerializedActor out = FSerializedActor();
		out.ActorClass = nullptr;
		return out;
	}

	bool IsValidActor() { return ActorClass != nullptr; }
};

USTRUCT(BlueprintType)
struct FSerializedWorld
{
	GENERATED_BODY()

public:

	FSerializedWorld()
	{
		bLoaded = false;
	}

	UPROPERTY( VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = Serializer )
		uint32 bLoaded : 1;

	UPROPERTY( VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = Serializer )
		TMap< FName, FSerializedActor > Actors;

};

USTRUCT(BlueprintType)
struct FSerializedGameState
{
	GENERATED_BODY()

public:

	UPROPERTY( VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = Serializer )
		TMap< FName, FSerializedWorld > Worlds;

	UPROPERTY( VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = Serializer )
		TMap< FName, FSerializedGameObject > PersistentObjects;

	UPROPERTY( SaveGame, VisibleAnywhere, BlueprintReadOnly )
		FSerializedActor SavedPlayerState;
};

UCLASS(BlueprintType)
class GAMESERIALIZER_API USavedGameState : public USaveGame
{
	GENERATED_BODY()

public:

	UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadWrite, Category = Save)
		int32 ScreenshotSizeX;

	UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadWrite, Category = Save)
		int32 ScreenshotSizeY;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = Save)
		TArray< FColor > ScreenshotPixels;

	UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadWrite, Category = Save)
		FDateTime TimeOfSave;
		
	UPROPERTY( SaveGame, VisibleAnywhere, BlueprintReadOnly )
		FString SavedTime;

	UPROPERTY( SaveGame, VisibleAnywhere, BlueprintReadOnly )
		FTransform PlayerTransform;

	UPROPERTY( SaveGame, VisibleAnywhere, BlueprintReadWrite )
		uint32 bUseActualTransform : 1;

	UPROPERTY( SaveGame, VisibleAnywhere, BlueprintReadOnly )
		FString SavedMap;

	UPROPERTY( SaveGame, VisibleAnywhere, BlueprintReadOnly )
		FSerializedGameState SavedState;

	UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadWrite, Category = Save)
		FString SavedGameOptions;
	
	/*UPROPERTY( SaveGame, VisibleAnywhere, BlueprintReadOnly )
		TMap< FName, FSerializedWorld > SavedWorlds;*/
};

UCLASS(BlueprintType)
class GAMESERIALIZER_API UGameSaveManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	UGameSaveManager();

	/**
	 * Can be set to filter out save games easily, will be used by all save manager functions
	 * For example, a prefix of "story_" will only look for "story_save_##" files 
	 */
	UPROPERTY( VisibleAnywhere, BlueprintReadOnly )
		FString SavePrefix;
	
	UPROPERTY( VisibleAnywhere, BlueprintReadOnly )
		uint32 bIsLoading : 1;

	UPROPERTY( VisibleAnywhere, BlueprintReadOnly )
		int32 CurrentSlot;

	UPROPERTY( VisibleAnywhere, BlueprintReadOnly )
		TArray< USavedGameState* > SavedGames;

	UPROPERTY( VisibleAnywhere, BlueprintReadOnly )
		FSerializedGameState CurrentGameState;

	/** The active data for the current session, good for loading levels but keeping data */
	UPROPERTY( VisibleAnywhere, BlueprintReadOnly )
		USavedGameState* CurrentSessionSave;

	/** Will keep objects in the array to prevent GC from destroying them. ONLY USE TRANSIENT OUTER OBJECTS! */
	UPROPERTY( VisibleAnywhere, BlueprintReadOnly )
		TArray< UObject* > PersistentObjects;

	UPROPERTY( BlueprintAssignable, Category = Game )
		FGameSaveEvent OnNewGame;

	UPROPERTY( BlueprintAssignable, Category = Saving )
		FGameSaveEvent OnSave;

	UPROPERTY( BlueprintAssignable, Category = Saving )
		FGameSaveEvent OnLoad;

		FDelegateHandle ActorLoadBinding;

	virtual void Initialize( FSubsystemCollectionBase& Collection ) override;

	virtual void Deinitialize() override;
	
	UFUNCTION( BlueprintPure )
	USavedGameState* GetCurrentSave()
	{
		if ( SavedGames.Num() <= 0 )
		{
			SavedGames = GetSaves();
		}

		if ( SavedGames.Num() <= 0 || !SavedGames.IsValidIndex( CurrentSlot ) )
		{
			return nullptr;
		}

		return SavedGames[CurrentSlot];
	}

	UFUNCTION( BlueprintPure )
		USavedGameState* GetSaveAtSlot( int32 slot );

	/* Returns the state of a single world in the current session. */
	UFUNCTION( BlueprintPure )
		FSerializedWorld GetWorldState( FName Id, bool& success );

	FSerializedWorld GetWorldState( FName Id ) { bool success = false; return GetWorldState( Id, success ); }

	UFUNCTION( BlueprintCallable )
		virtual USavedGameState* StartNewGame( int32 slot = -1 );

	UFUNCTION( BlueprintCallable )
		virtual void LoadGameFromSlot( int32 index, bool bOpenLevel = true );
	void SaveSessionToSaveObject(USavedGameState* saveFile);

	UFUNCTION( BlueprintCallable )
		void SaveGameToSlot( int32 index );

	UFUNCTION( BlueprintPure )
		TArray< USavedGameState* > GetSaves();

	UFUNCTION(BlueprintCallable)
		void CreateSessionSave();
	
	UFUNCTION(BlueprintCallable)
		void SaveSessionState();

	UFUNCTION(BlueprintPure)
		FString GetIndexedSaveName(int32 index = 0) const;

	UFUNCTION(BlueprintCallable)
		void AssignSavePrefix(FString Prefix);
	
protected:

	virtual USavedGameState* CreateSaveGame(FName NameOverride = NAME_None);

public:

	virtual void LoadWorldState( FSerializedGameState state );

	void LoadPersistentObjects( FSerializedGameState state );

	virtual FSerializedGameState CacheWorld();

	virtual void GatherWorldStates();

	virtual void CacheWorldState( FName worldId, FSerializedWorld world )
	{
		CurrentGameState.Worlds.Add( worldId, world );
	}

	
	virtual void CachePersistentObject( UObject* object );
	
	virtual void ReleasePersistentObject( UObject* object );

	
	virtual void LoadedActor( AActor* actor );

	virtual void UnloadedActor( AActor* actor );
};

class ISerializationCore
{
public:

	virtual UGameSaveManager* GetSaveManager() = 0;

};
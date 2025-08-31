// Fill out your copyright notice in the Description page of Project Settings.

#include "Classes.h"
#include "SerializationHelpers.h"
#include "SerializationManager.h"
#include "GameSerializerSettings.h"

#include "Engine/Engine.h"
//#include "EngineGlobals.h"
#include "EngineUtils.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerState.h"

Classes::Classes()
{
}

Classes::~Classes()
{
}

UGameSaveManager::UGameSaveManager()
{
	CurrentGameState = FSerializedGameState();
	CurrentSlot = 0;
	bIsLoading = false;
}

void UGameSaveManager::CreateSessionSave()
{
	CurrentSessionSave = CreateSaveGame(TEXT("SessionSave"));
}

void UGameSaveManager::Initialize( FSubsystemCollectionBase & Collection )
{
	Super::Initialize( Collection );
	UE_LOG( LogSaveGame, Warning, TEXT( "Game Save Manager initialized!" ) );

	if(UGameSerializerSettings* Settings = UGameSerializerSettings::Get())
	{
		if(Settings->bAutoLoadGameOnBeginPlay)
		{
			LoadGameFromSlot(0, false);
		}
		else
		{
			CreateSessionSave();
		}
	}

	//LoadGameFromSlot( 0, false );

	//Should load level objects at startup to help with resuming 
	//if ( CurrentSlot >= 0 )
	//{
	//	TArray< USavedGameState* > saves = GetSaves();

	//	if ( saves.IsValidIndex( CurrentSlot ) )
	//	{
	//		LoadPersistentObjects( saves[CurrentSlot]->SavedState );
	//	}
	//}

	/*ActorLoadBinding = GEngine->OnLevelActorAdded().AddLambda( [this]( AActor* level ) {
		LoadedActor( level );
	} );
	*/
	//LoadGameFromSlot( 0 );
}

void UGameSaveManager::Deinitialize()
{
	//GEngine->OnLevelActorAdded().Remove( ActorLoadBinding );
}

USavedGameState * UGameSaveManager::GetSaveAtSlot( int32 slot )
{
	TArray< USavedGameState* > saves = GetSaves();

	if ( saves.IsValidIndex( slot ) == false || saves[slot] == nullptr )
	{
		saves.SetNum( slot + 1, false );
		saves[slot] = CreateSaveGame();
		UE_LOG( LogSaveGame, Warning, TEXT( "Had to create save for slot %i" ), slot );
	}

	return saves[slot];
}

FSerializedWorld UGameSaveManager::GetWorldState( FName Id, bool& success )
{
	if ( CurrentGameState.Worlds.Contains(Id) )
	{
		success = true;
		return CurrentGameState.Worlds[Id];
	}

	success = false;
	return FSerializedWorld();
}

USavedGameState * UGameSaveManager::StartNewGame( int32 slot )
{
	if ( slot < 0 )
	{
		slot = GetSaves().Num();
	}

	CurrentSlot = slot;
	USavedGameState* save = CreateSaveGame();

	SavedGames.Add( save );

	PersistentObjects.Empty();
	CurrentGameState = FSerializedGameState();

	UGameplayStatics::SaveGameToSlot( save, GetIndexedSaveName(slot), 0 );
	
	//Use this to hook into the logic for handling new game things, like setting up mode, extra data, and loading initial map.
	OnNewGame.Broadcast( save );

	return save;
}

void UGameSaveManager::LoadGameFromSlot( int32 index, bool bLoadLevel )
{
	USavedGameState* saveFile = Cast< USavedGameState >( UGameplayStatics::LoadGameFromSlot( GetIndexedSaveName( index ), 0 ) );

	if ( !saveFile )
	{
		return;
	}

	CurrentSlot = index;

	if ( bLoadLevel )
	{
		UGameplayStatics::OpenLevel( this, *saveFile->SavedMap, true );
	}

	LoadWorldState( saveFile->SavedState );
	OnLoad.Broadcast( saveFile );

	UE_LOG( LogSaveGame, Warning, TEXT( "Loaded game!" ) );
}

void UGameSaveManager::SaveSessionToSaveObject(USavedGameState* saveFile)
{
	saveFile->SavedState = CacheWorld();
	saveFile->SavedMap = UGameplayStatics::GetCurrentLevelName( this, true );

	FDateTime time = FDateTime::Now();
	saveFile->SavedTime = time.ToString();

	saveFile->SavedGameOptions = GetWorld()->GetAuthGameMode()->OptionsString;
	
	if ( APawn* player = UGameplayStatics::GetPlayerPawn( this, 0 ) )
	{
		saveFile->PlayerTransform = player->GetActorTransform();
	}

	OnSave.Broadcast( saveFile );
}

void UGameSaveManager::SaveGameToSlot( int32 index )
{
	CurrentSlot = index;

	// Load data from disk
	USavedGameState* saveFile = GetSaveAtSlot( index ); //CreateSaveGame();//NewObject< USavedGameState >( GetTransientPackage() );

	// Write data to object
	SaveSessionToSaveObject(saveFile);

	// Write data to disk
	if ( UGameplayStatics::SaveGameToSlot( saveFile, GetIndexedSaveName(index), 0 ) )
	{
		auto size = (float)sizeof( saveFile );
		UE_LOG( LogSaveGame, Warning, TEXT( "Saved game to %i! File size: %f" ), index, size );
	}
	else
	{
		UE_LOG( LogSaveGame, Error, TEXT( "Failed to save game!" ) );
	}
}

TArray<USavedGameState*> UGameSaveManager::GetSaves()
{
	//FString pref = "save_";
	int32 index = 0;

	TArray< USavedGameState* > saves = TArray< USavedGameState* >();

	//If we already have saves, just return the array
	if ( saves.Num() > 0 )
	{
		return SavedGames;
	}

	while ( index <= 25 )
	{
		FString slot = GetIndexedSaveName(index);//pref + FString::FromInt( index );
		USavedGameState* save = Cast< USavedGameState >( UGameplayStatics::LoadGameFromSlot( slot, 0 ) );

		if ( save )
		{
			saves.Add( save );
			index++;
		}
		else
		{
			UE_LOG( LogSaveGame, Warning, TEXT( "Couldn't find save {%s}" ), *slot );
			break;
		}
	}

	SavedGames = saves;

	return saves;
}

void UGameSaveManager::SaveSessionState()
{
	check(IsValid(CurrentSessionSave));

	SaveSessionToSaveObject(CurrentSessionSave);

	UE_LOG(LogSaveGame, Verbose, TEXT("Saved current session!"));
}

FString UGameSaveManager::GetIndexedSaveName(int32 index) const
{
	return SavePrefix + "save_" + FString::FromInt( index );
}

void UGameSaveManager::AssignSavePrefix(FString Prefix)
{
	UE_LOG(LogSaveGame, Log, TEXT("Changed save manager prefix to %s"), *Prefix);
	SavePrefix = Prefix;
}

USavedGameState * UGameSaveManager::CreateSaveGame(FName NameOverride)
{
	TSubclassOf< USavedGameState > saveClass = USavedGameState::StaticClass();
	UGameSerializerSettings* settings = GetMutableDefault< UGameSerializerSettings >();

	if ( settings && settings->SaveClass )
	{
		saveClass = settings->SaveClass;
	}

	
	return NewObject< USavedGameState >( this, saveClass, NameOverride );
}

void UGameSaveManager::LoadWorldState( FSerializedGameState state )
{
	CurrentGameState = state;

	LoadPersistentObjects( state );

	for ( TActorIterator< ASerializationManager > Iter( GetWorld() ); Iter; ++Iter )
	{
		ASerializationManager* manager = *Iter;
	}

	UE_LOG( LogSaveGame, Warning, TEXT( "Loaded world state!" ) );
}

void UGameSaveManager::LoadPersistentObjects( FSerializedGameState state )
{
	UE_LOG(LogTemp, Log, TEXT("Purging persistent objects and clearing garbage..."))
	PersistentObjects.Empty();
	GEngine->ForceGarbageCollection(true);
	UE_LOG(LogTemp, Log, TEXT("Garbage purged!"))
	
	for ( auto&& keypair : CurrentGameState.PersistentObjects )
	{
		if ( keypair.Key == NAME_None )
		{
			UE_LOG( LogSaveGame, Warning, TEXT( "Invalid name on persistent object save!" ) );
			continue;
		}

		UObject* object = FindObject<UObject>( GetTransientPackage(), *keypair.Key.ToString(), false );

		if ( !object && keypair.Value.ObjectClass )
		{
			object = NewObject< UObject >( GetTransientPackage(), keypair.Value.ObjectClass, keypair.Value.UniqueId );
			UE_LOG( LogSaveGame, Warning, TEXT( "Created %s" ), *object->GetPathName() );
		}

		PersistentObjects.Add( object );
	}

	for ( int32 x = 0; x < PersistentObjects.Num(); x++ )
	{
		USerializationHelpers::LoadObject( PersistentObjects[x], CurrentGameState.PersistentObjects.FindOrAdd( *PersistentObjects[x]->GetPathName() ) );
	}

	UE_LOG( LogSaveGame, Warning, TEXT( "Loaded persistent objects!" ) );
}

FSerializedGameState UGameSaveManager::CacheWorld()
{
	GatherWorldStates();

	for ( UObject* obj : PersistentObjects )
	{
		FName saveId = USerializationHelpers::ResolveID( obj );
		FSerializedGameObject saveObj = USerializationHelpers::SaveObject( obj );

		CurrentGameState.PersistentObjects.Add( saveId, saveObj );
	}

	APlayerController* controller = UGameplayStatics::GetPlayerController( this, 0 );

	if ( APlayerState* state = controller->GetPlayerState< APlayerState >() )
	{
		CurrentGameState.SavedPlayerState = USerializationHelpers::SaveActor( state );
	}

	return CurrentGameState;
}

void UGameSaveManager::GatherWorldStates()
{
	TMap< UObject*, FName > worldPaths;
	TArray< TSubclassOf< AActor > > typeBlacklist;

	//Maybe cache serialization managers as they spawn?
	for ( TActorIterator< ASerializationManager > Iter( GetWorld() ); Iter; ++Iter )
	{
		ASerializationManager* manager = *Iter;
		UObject* outer = manager->GetLevel()->GetOuter();

		worldPaths.Add( outer, manager->WorldID );
		manager->CacheWorldState();
	}

	//for ( TActorIterator< AActor > Iter( GetWorld() ); Iter; ++Iter )
	//{
	//	AActor* actor = *Iter;
	//	UObject* outer = Iter->GetLevel()->GetOuter();
	//	
	//	if ( worldPaths.Contains( outer ) )
	//	{
	//		TSubclassOf< AActor > aClass = actor->GetClass();
	//		bool valid = true;

	//		for ( int32 x = 0; x < typeBlacklist.Num(); x++ )
	//		{
	//			if ( aClass->IsChildOf( typeBlacklist[x] ) )
	//			{
	//				valid = false;
	//				break;
	//			}

	//			continue;
	//		}
	//
	//		if ( valid )
	//		{

	//		}
	//	}
	//}
}

void UGameSaveManager::CachePersistentObject( UObject* object )
{
	FName id = USerializationHelpers::ResolveID( object );
	FSerializedGameObject data = USerializationHelpers::SaveObject( object );
	CurrentGameState.PersistentObjects.Add( id, data );
	
	PersistentObjects.Add( object );

	UE_LOG( LogSaveGame, Warning, TEXT( "Cached %s" ), *id.ToString() );
}

void UGameSaveManager::ReleasePersistentObject( UObject * object )
{
	if ( PersistentObjects.Contains( object ) )
	{
		PersistentObjects.Remove( object );
		UE_LOG( LogSaveGame, Warning, TEXT( "Removed %s from being persistent!" ), *object->GetName() );
	}
}

void UGameSaveManager::LoadedActor( AActor * actor )
{
	UE_LOG( LogSaveGame, Warning, TEXT( "Loaded level actor %s" ), *actor->GetName() );

	APlayerState* state = Cast< APlayerState >( actor );

	if ( state )
	{
		//USerializationHelpers::LoadActor( state, CurrentGameState.SavedPlayerState );
	}
}

void UGameSaveManager::UnloadedActor( AActor * actor )
{
}

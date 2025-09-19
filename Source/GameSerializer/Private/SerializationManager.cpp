// Fill out your copyright notice in the Description page of Project Settings.

#include "SerializationManager.h"

#include "Classes.h"
#include "SerializationHelpers.h"
#include "GameSerializerSettings.h"

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "IGameSerializable.h"
#include "Engine/GameInstance.h"


// Sets default values for this component's properties
ASerializationManager::ASerializationManager()
{

}


void ASerializationManager::LoadWorldState( FSerializedWorld state )
{
	WorldData = state;

	UWorld* world = GetWorld();
	UObject* outer = GetLevel()->GetOuter();

	TMap< AActor*, FSerializedActor > Spawned = TMap< AActor*, FSerializedActor >();

	for ( auto&& keypair : state.Actors )
	{
		if ( keypair.Value.bWasSpawned )
		{
			Spawned.Add( 
				GetWorld()->SpawnActorDeferred<AActor>( keypair.Value.ActorClass, keypair.Value.ActorTransform, this ),
				keypair.Value
				);
		}
	}

	for ( TActorIterator< AActor > Iter( GetWorld() ); Iter; ++Iter )
	{
		AActor* actor = *Iter;

		if ( actor->GetLevel()->GetOuter() == outer )
		{
			FName actorID = USerializationHelpers::ResolveID( actor );
			if ( state.Actors.Contains( actorID ) )
			{
				USerializationHelpers::LoadActor( actor, state.Actors[actorID] );

				if ( actor->ActorHasTag( SaveTags::IgnoreTransform ) == false )
				{
					actor->SetActorTransform( state.Actors[actorID].ActorTransform, false, nullptr, ETeleportType::ResetPhysics );
				}
			}
		}
	}

	for ( auto&& actorData : Spawned )
	{
		actorData.Key->FinishSpawning( actorData.Value.ActorTransform );
	}
}

// Called when the game starts
void ASerializationManager::BeginPlay()
{
	Super::BeginPlay();

	WorldRef = Cast< UWorld >( GetLevel()->GetOuter() );
	UE_LOG( LogSaveGame, Warning, TEXT( "%s" ), *GetWorld()->GetName() );
	UE_LOG( LogSaveGame, Warning, TEXT( "Initialized Serialization Manager %s" ), *this->GetName() );
	// 

	UGameSaveManager* manager = USerializationHelpers::GetGameSaveManager( this );
	SaveManagerRef = manager;

	LoadWorldState( manager->GetWorldState( WorldID ) );
}

void ASerializationManager::BeginDestroy()
{
	UE_LOG( LogSaveGame, Warning, TEXT( "Manager Begin Destroy!" ) );
	Super::BeginDestroy();

}

void ASerializationManager::EndPlay( const EEndPlayReason::Type EndPlayReason )
{
	UE_LOG( LogSaveGame, Warning, TEXT( "Manager End Play!" ) );
	
	if ( EndPlayReason != EEndPlayReason::Quit )
	{
		CacheWorld();
	}

	Super::EndPlay( EndPlayReason );
}

void ASerializationManager::PreDelete( AActor* actor, EEndPlayReason::Type EndPlayReason )
{
}

void ASerializationManager::CacheWorld()
{
	////Get master scene
	////record all actors

	UWorld* world = GetWorld();
	UObject* outer = GetLevel()->GetOuter();

	WorldData = FSerializedWorld();

	for ( TActorIterator< AActor > Iter( GetWorld() ); Iter; ++Iter )
	{
		AActor* actor = *Iter;

		//Serialization on scene actors is opt in now
		if(!actor->Implements<UGameSerializable>())
		{
			continue;
		}

		TSubclassOf<AActor> aClass = actor->GetClass();

		if ( USerializationHelpers::IsClassBlacklisted( aClass ) )
		{
			UE_LOG( LogSaveGame, Warning, TEXT( "Ignored %s" ), *actor->GetPathName() );
			continue;
		}

		if ( actor->ActorHasTag( SaveTags::Ignore ) || actor->IsRootComponentStatic() )
		{
			UE_LOG( LogSaveGame, Warning, TEXT( "Ignored %s" ), *actor->GetPathName() );
			continue;
		}

		if ( AActor* parent = actor->GetParentActor() )
		{
			if ( actor->IsChildActor() && parent->ActorHasTag( SaveTags::Ignore ) )
			{
				UE_LOG( LogSaveGame, Warning, TEXT( "Ignored %s" ), *actor->GetPathName() );
				continue;
			}
		}

		if ( !actor->ActorHasTag( SaveTags::Save ) )
		{
			UE_LOG( LogSaveGame, Warning, TEXT( "Ignored %s" ), *actor->GetPathName() );
			continue;
		}

		if ( actor->GetLevel()->GetOuter() == outer )
		{
			FSerializedActor save = USerializationHelpers::SaveActor( actor );

			WorldData.Actors.Add( USerializationHelpers::ResolveID( actor ), save );

			UE_LOG( LogSaveGame, Warning, TEXT( "Found actor %s :: Full path == %s" ), *actor->GetName(), *actor->GetPathName() );
		}
	}

	UGameInstance* gameInstance = GetWorld()->GetGameInstance();
	UGameSaveManager* manager = gameInstance->GetSubsystem<UGameSaveManager>();

	//Cache the existence of the level
	WorldData.bLoaded = GetLevel()->bIsVisible;

	if ( manager )
	{
		manager->CacheWorldState( WorldID, WorldData );
		//manager->CacheWorldState( FName( *WorldRef.GetUniqueID().ToString() ), WorldData );
		//UE_LOG( LogTemp, Warning, TEXT( "Serialized %s actors!" ), *FString::FromInt( WorldData.Actors.Num() ) );
	}
	else
	{
		UE_LOG( LogSaveGame, Error, TEXT( "Failed to cache world state!" ) );
	}
}

// Fill out your copyright notice in the Description page of Project Settings.

#include "SerializationHelpers.h"
#include "GameFramework/Actor.h"

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"

#include "Engine/World.h"
#include "Templates/SubclassOf.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "GameSerializer/Public/GameSerializerArchive.h"
#include "GameSerializer/Public/IGameSerializable.h"

FSerializedActor USerializationHelpers::SaveActor(AActor* actor)
{
	ensure( actor );

	if ( !actor ) { return FSerializedActor::Null(); }

	auto save = FSerializedActor();
	FMemoryWriter MemoryWriter(save.Data);
	FGameSerializerArchive Ar(MemoryWriter);

	actor->Serialize( Ar );

	save.bWasSpawned = !actor->bNetStartup;
	save.ActorClass = actor->GetClass();
	save.ActorTransform = actor->GetTransform();
	save.UniqueId = *actor->GetName();
	//try
	//{
	//	actor->Serialize(Ar);
	//}
	//catch (const std::exception&)
	//{
	//	UE_LOG(LogTemp, Error, TEXT("Failed to save %s"), *actor->GetName());
	//}

	return save;
}

FSerializedGameObject USerializationHelpers::SaveObject( UObject * object )
{
	FSerializedGameObject save = FSerializedGameObject();
	FMemoryWriter MemoryWriter( save.Data );
	FGameSerializerArchive Ar( MemoryWriter );

	object->Serialize( Ar );

	save.UniqueId = *object->GetName();
	save.ObjectClass = object->GetClass();

	UE_LOG( LogSaveGame, Warning, TEXT( "Saved object %s" ), *object->GetPathName() );
	return save;
}

void USerializationHelpers::LoadActor(AActor* actor, FSerializedActor save)
{
	ensure( actor );

	if (actor == nullptr)
	{
		UE_LOG(LogSaveGame, Error, TEXT("Do not try to save/load null actors!"));
		return;
	}

	FMemoryReader MemoryReader( save.Data, true );
	FGameSerializerArchive Ar( MemoryReader );

	actor->Serialize( Ar );

	if ( actor->GetClass()->ImplementsInterface( UGameSerializable::StaticClass() ) )
	{
		IGameSerializable::Execute_PostDataLoaded( actor );
	}

	UE_LOG( LogSaveGame, Log, TEXT( "Loaded data into %s" ), *actor->GetName() );

	//try
	//{
	//	FMemoryReader MemoryReader(save.Data, true);
	//	FSerializerArchive Ar(MemoryReader);

	//	actor->Serialize(Ar);

	//	if (actor->GetClass()->ImplementsInterface(USerializable::StaticClass()))
	//	{
	//		ISerializable::Execute_PostDataLoaded(actor);
	//	}

	//	UE_LOG(LogTemp, Log, TEXT("Loaded data into %s"), *actor->GetName());
	//}
	//catch (const std::exception&)
	//{
	//	UE_LOG(LogTemp, Error, TEXT("Failed to load %s"), *actor->GetName());
	//}
}

void USerializationHelpers::LoadObject( UObject * object, FSerializedGameObject save )
{
	ensure( object );

	if ( object == nullptr )
	{
		UE_LOG( LogSaveGame, Error, TEXT( "Do not try to save/load null objects! It's rude!" ) );
		return;
	}

	FMemoryReader MemoryReader( save.Data );
	FGameSerializerArchive Ar( MemoryReader );

	object->Serialize( Ar );

	if ( object->GetClass()->ImplementsInterface( UGameSerializable::StaticClass() ) )
	{
		IGameSerializable::Execute_PostDataLoaded( object );
	}

	UE_LOG( LogSaveGame, Warning, TEXT( "Loaded data into %s" ), *object->GetPathName() );
}

AActor* USerializationHelpers::SpawnAndLoadActor(UObject* WorldContextObject, FSerializedActor save)
{
	AActor* actor = nullptr;

	if (save.ActorClass != nullptr && WorldContextObject != nullptr)
	{
		auto world = WorldContextObject->GetWorld();
		FTransform transform = save.ActorTransform;
		auto params = FActorSpawnParameters();

		//params.Name = //serialized name
		
		actor = world->SpawnActor<AActor>(save.ActorClass, transform, params);

		USerializationHelpers::LoadActor(actor, save);
	}
	else
	{
		UE_LOG(LogSaveGame, Error, TEXT("Failed to spawn actor from save!"));
	}

	return actor;
}

UGameSaveManager * USerializationHelpers::GetGameSaveManager(UObject* WorldContextObject)
{
	check( WorldContextObject );

	UGameInstance* gInstance = WorldContextObject->GetWorld()->GetGameInstance();
	UGameSaveManager* manager = gInstance->GetSubsystem< UGameSaveManager >();

	return manager;
}

FName USerializationHelpers::ResolveID( UObject* object )
{
	if ( !object ) { return NAME_None; }

	return FName( *object->GetPathName() );
}

bool USerializationHelpers::IsClassBlacklisted( TSubclassOf<UObject> objectClass )
{
	TArray< TSubclassOf< UObject > > Blacklist = TArray< TSubclassOf< UObject > >();
	Blacklist.Add( APlayerState::StaticClass() );
	Blacklist.Add( AController::StaticClass() );
	Blacklist.Add( AGameModeBase::StaticClass() );
	Blacklist.Add( AGameStateBase::StaticClass() );

	for ( int32 x = 0; x < Blacklist.Num(); x++ )
	{
		if ( objectClass->IsChildOf( Blacklist[x] ) )
		{
			return true;
		}
	}

	return false;
}

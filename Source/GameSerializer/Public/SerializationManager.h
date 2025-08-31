// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "Classes.h"

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Info.h"
#include "SerializationManager.generated.h"


UCLASS( ClassGroup=(Custom)/*, meta=(BlueprintSpawnableComponent)*/ )
class GAMESERIALIZER_API ASerializationManager : public AInfo
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	ASerializationManager();
	
	/** Purely for sanity checks, and a way to debug the manager. */
	UPROPERTY( VisibleAnywhere )
		class UGameSaveManager* SaveManagerRef;

	/** The id of the scene */
	UPROPERTY( EditAnywhere, BlueprintReadOnly )
		FName WorldID;

	UPROPERTY( EditAnywhere, BlueprintReadOnly )
		TSoftObjectPtr< class UWorld > WorldRef;

	UPROPERTY( VisibleAnywhere, BlueprintReadOnly )
		FSerializedWorld WorldData;

	UFUNCTION( BlueprintCallable, Category = Serialization )
	FSerializedWorld CacheWorldState()
	{
		CacheWorld();
		return WorldData;
	}

	virtual void LoadWorldState( FSerializedWorld state );


protected:

	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void BeginDestroy() override;

	virtual void EndPlay( const EEndPlayReason::Type EndPlayReason ) override;

	virtual void PreDelete( AActor* actor, EEndPlayReason::Type EndPlayReason );

	virtual void CacheWorld();

public:	

	
};

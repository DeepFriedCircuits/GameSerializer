// Fill out your copyright notice in the Description page of Project Settings.

#include "IGameSerializable.h"


// Add default functionality here for any ISerializable functions that are not pure virtual.
void IGameSerializable::PostDataLoaded_Implementation()
{
    UE_LOG(LogSaveGame, Warning, TEXT( "Post serialize!" ));
    DataLoaded();
}

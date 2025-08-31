// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

/**
 * 
 */
struct GAMESERIALIZER_API FGameSerializerArchive : public FObjectAndNameAsStringProxyArchive
{
	FGameSerializerArchive(FArchive& innerArchive)
		:FObjectAndNameAsStringProxyArchive(innerArchive, false)
	{
		ArIsSaveGame = true;
	}
};

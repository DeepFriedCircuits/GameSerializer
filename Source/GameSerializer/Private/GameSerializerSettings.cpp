#include "GameSerializerSettings.h"

#include "Classes.h"

UGameSerializerSettings::UGameSerializerSettings()
{
	SaveClass = USavedGameState::StaticClass();
}
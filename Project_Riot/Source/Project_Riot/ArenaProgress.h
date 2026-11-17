#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ArenaProgress.generated.h"

// What is kept on disk between launches: money, purchases, the outfit, and which
// routes have been finished. Where she is in a route is not saved; a launch starts at home.
UCLASS()
class PROJECT_RIOT_API UArenaProgress : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	int32 Gold = 0;

	UPROPERTY()
	TMap<FName, int32> Purchases;

	UPROPERTY()
	FName Outfit;

	UPROPERTY()
	TArray<FString> FinishedRoutes;
};

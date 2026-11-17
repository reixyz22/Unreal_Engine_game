#pragma once

#include "CoreMinimal.h"
#include "ArenaStage.generated.h"

class UArenaTheme;

// One stop on a hero's world-map route. Index 0 is their home town; the rest are combat stages.
USTRUCT(BlueprintType)
struct FStageDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UArenaTheme> Theme;

	// Where it sits on the world map, 0..1 from the top-left.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D MapPosition = FVector2D(0.5, 0.5);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Waves = 10;

	// Still a stand-in: shown as work in progress on the map.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bPlaceholder = false;
};

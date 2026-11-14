#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ArenaTheme.generated.h"

class AArenaEnemy;
class UPaperFlipbook;
class UPaperSprite;
class USoundBase;

// How one enemy type looks under a theme: an optional replacement animation, and a
// palette swap. The swap is a gradient map: each pixel's brightness picks a color
// between Shadow and Light, blended over the original art by Strength.
USTRUCT(BlueprintType)
struct FEnemySkin
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AArenaEnemy> Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UPaperFlipbook> Anim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Shadow = FLinearColor::Black;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Light = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Strength = 0.f;

	// Tint for anything this enemy fires (Pyro beads).
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor ProjectileTint = FLinearColor::White;
};

// A run's look and sound. Each theme is a Blueprint subclass that overrides only what it
// changes; these C++ defaults are the plain dungeon. One is picked at random per run.
// Player characters and the HUD are never themed, so they always read clearly.
UCLASS(Abstract, Blueprintable)
class PROJECT_RIOT_API UArenaTheme : public UObject
{
	GENERATED_BODY()

public:
	const FEnemySkin* FindSkin(const UClass* EnemyClass) const;

	UPROPERTY(EditDefaultsOnly, Category = "Theme")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, Category = "Theme")
	TObjectPtr<USoundBase> Music;

	// The arena floor art for this theme; empty keeps the level's floor.
	UPROPERTY(EditDefaultsOnly, Category = "Theme")
	TObjectPtr<UPaperSprite> FloorSprite;

	UPROPERTY(EditDefaultsOnly, Category = "Theme")
	FLinearColor FloorTint = FLinearColor(0.85f, 0.85f, 0.85f);

	UPROPERTY(EditDefaultsOnly, Category = "Theme")
	FLinearColor FogColor = FLinearColor(0.45f, 0.32f, 0.7f);

	UPROPERTY(EditDefaultsOnly, Category = "Theme")
	float FogDensity = 0.35f;

	// Only the pause screen takes the theme's color; gameplay UI stays fixed.
	UPROPERTY(EditDefaultsOnly, Category = "Theme")
	FLinearColor MenuTint = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "Theme")
	TArray<FEnemySkin> EnemySkins;
};

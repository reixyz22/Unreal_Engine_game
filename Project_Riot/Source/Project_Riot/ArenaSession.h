#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ArenaSession.generated.h"

class AArenaPlayerPawn;

// Which part of the game the level is showing. One level hosts them all; travelling
// reloads it with a different scene.
enum class EArenaScene : uint8
{
	Hut,       // Nikki's home, and the intro
	Town,      // the village hub
	WorldMap,  // stage select
	Stage,     // a combat stage
};
class UAudioComponent;
class USoundBase;

// What survives a level reload: settings, the chosen hero, and story progress
// (where she is, what she has bought, which stages are open).
// Also the one place sounds are played, so the volume sliders apply everywhere.
UCLASS()
class PROJECT_RIOT_API UArenaSession : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UArenaSession* Get(const UObject* WorldContext);

	void PlayMusic(USoundBase* Music);
	// Plays a one-shot at the player's SFX volume. Safe with a null sound.
	static void PlaySfx(const UObject* WorldContext, USoundBase* Sound);

	float GetMusicVolume() const { return MusicVolume; }
	float GetSfxVolume() const { return SfxVolume; }
	void SetMusicVolume(float Volume);
	void SetSfxVolume(float Volume) { SfxVolume = FMath::Clamp(Volume, 0.f, 1.f); }

	// Menus play the music quietly underneath.
	void SetMusicDucked(bool bDucked);

	// Reloads the level into another scene. StageIndex picks the stage on the route.
	void Travel(const UObject* WorldContext, EArenaScene NewScene, int32 NewStageIndex = INDEX_NONE);
	bool Owns(FName ItemId) const { return Purchases.Contains(ItemId); }

	TSubclassOf<AArenaPlayerPawn> ChosenHero;
	bool bIntroDone = false;       // hero chosen on the start screen
	bool bControlsShown = false;   // how-to-play shown before the first stage

	// Story progress.
	EArenaScene Scene = EArenaScene::Hut;
	EArenaScene PreviousScene = EArenaScene::Hut;
	int32 StageIndex = 1;
	int32 HighestUnlocked = 1;
	int32 Gold = 0;
	TMap<FName, int32> Purchases;
	bool bIntroSeen = false;
	bool bBedSmashed = false;
	bool bDressed = false;
	bool bContractSigned = false;

	// F1 toggles debug keys (see AArenaHUD::TickDebug).
	bool bDebug = false;

private:
	void ApplyMusicVolume() const;

	UPROPERTY()
	TObjectPtr<UAudioComponent> MusicPlayer;

	float MusicVolume = 0.5f;
	float SfxVolume = 0.8f;
	bool bMusicDucked = false;
};

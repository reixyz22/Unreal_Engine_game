#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ArenaSession.generated.h"

class AArenaPlayerPawn;
class UAudioComponent;
class USoundBase;

// What survives a restart: volume settings, the chosen hero, and whether the intro was seen.
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

	TSubclassOf<AArenaPlayerPawn> ChosenHero;
	bool bIntroDone = false;

private:
	void ApplyMusicVolume() const;

	UPROPERTY()
	TObjectPtr<UAudioComponent> MusicPlayer;

	float MusicVolume = 0.7f;
	float SfxVolume = 0.8f;
	bool bMusicDucked = false;
};

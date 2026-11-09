#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ArenaGameMode.generated.h"

class UPaperFlipbook;
class UPaperSprite;
class USoundBase;

UCLASS()
class PROJECT_RIOT_API AArenaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	void OnPlayerDied();

protected:
	virtual void BeginPlay() override;

private:
	void CheckWave();
	void StartNextWave();
	FVector PickSpawnPoint() const;
	int32 CountEnemies() const;
	void ShowRoundText() const;
	void RestartArena();

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TObjectPtr<USoundBase> Music;

	UPROPERTY(EditDefaultsOnly, Category = "Waves")
	TObjectPtr<UPaperFlipbook> PyroAnim;

	UPROPERTY(EditDefaultsOnly, Category = "Waves")
	FLinearColor PyroTint = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "Waves")
	TObjectPtr<UPaperSprite> BeadSprite;

	UPROPERTY(EditDefaultsOnly, Category = "Waves")
	int32 FirstWaveSize = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Waves")
	int32 ExtraEnemiesPerRound = 2;

	UPROPERTY(EditDefaultsOnly, Category = "Waves")
	int32 MaxWaveSize = 30;

	UPROPERTY(EditDefaultsOnly, Category = "Waves")
	float SecondsBetweenWaves = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves")
	float ArenaHalfSize = 1024.f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves")
	float SpawnInset = 120.f;

	UPROPERTY(EditDefaultsOnly, Category = "Waves")
	float MinSpawnDistanceFromPlayer = 450.f;

	int32 Round = 0;
	bool bWaveQueued = false;
	bool bPlayerDead = false;

	FTimerHandle CheckTimer;
	FTimerHandle WaveTimer;
	FTimerHandle RestartTimer;
};

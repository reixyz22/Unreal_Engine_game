#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ArenaGameMode.generated.h"

class AArenaEnemy;
class USoundBase;

USTRUCT()
struct FWaveGroup
{
	GENERATED_BODY()

	UPROPERTY()
	TSubclassOf<AArenaEnemy> Type;

	int32 Count = 0;
};

UCLASS()
class PROJECT_RIOT_API AArenaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AArenaGameMode();

	void OnPlayerDied();

	// What the wave banner shows: the wave being fought, or the next one while it's incoming.
	int32 GetBannerWave() const { return bWaveQueued ? Round + 1 : Round; }
	const TArray<FWaveGroup>& GetBannerGroups() const { return Wave; }
	bool IsWaveIncoming() const { return bWaveQueued; }
	bool IsPlayerDead() const { return bPlayerDead; }
	int32 GetRoundsSurvived() const { return FMath::Max(0, Round - 1); }

protected:
	virtual void BeginPlay() override;

private:
	void CheckWave();
	void PlanWave();
	void StartNextWave();
	FVector PickSpawnPoint() const;
	int32 CountEnemies() const;
	void RestartArena();

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TObjectPtr<USoundBase> Music;

	// Each enemy in a wave is a random pick from this list.
	UPROPERTY(EditDefaultsOnly, Category = "Waves")
	TArray<TSubclassOf<AArenaEnemy>> EnemyTypes;

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

	UPROPERTY()
	TArray<FWaveGroup> Wave;

	int32 Round = 0;
	bool bWaveQueued = false;
	bool bPlayerDead = false;

	FTimerHandle CheckTimer;
	FTimerHandle WaveTimer;
	FTimerHandle RestartTimer;
};

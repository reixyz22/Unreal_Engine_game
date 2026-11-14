#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ArenaGameMode.generated.h"

class AArenaEnemy;
class AArenaPlayerPawn;
class UArenaTheme;
class UPaperSprite;
class USoundBase;

// One enemy type's place in the waves: from which round it appears, and its share of each wave.
USTRUCT(BlueprintType)
struct FEnemySpawnRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AArenaEnemy> Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 FirstRound = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Weight = 1.f;
};

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

	// The run's theme, picked at random before anything else begins play.
	const UArenaTheme& GetTheme() const { return *Theme; }
	UPaperSprite* GetEnemyIcon(TSubclassOf<AArenaEnemy> Type) const;

	const TArray<TSubclassOf<AArenaPlayerPawn>>& GetHeroes() const { return Heroes; }
	void ChooseHero(TSubclassOf<AArenaPlayerPawn> Hero);

	// What the wave banner shows: the wave being fought, or the next one while it's incoming.
	int32 GetBannerWave() const { return bWaveQueued ? Round + 1 : Round; }
	const TArray<FWaveGroup>& GetBannerGroups() const { return Wave; }
	bool IsWaveIncoming() const { return bWaveQueued; }
	bool IsPlayerDead() const { return bPlayerDead; }
	int32 GetRoundsSurvived() const { return FMath::Max(0, Round - 1); }

protected:
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

private:
	void CheckWave();
	void PlanWave();
	void StartNextWave();
	FVector PickSpawnPoint() const;
	int32 CountEnemies() const;
	void RestartArena();

	// Playable heroes, shown on the start screen.
	UPROPERTY(EditDefaultsOnly, Category = "Heroes")
	TArray<TSubclassOf<AArenaPlayerPawn>> Heroes;

	UPROPERTY(EditDefaultsOnly, Category = "Theme")
	TArray<TSubclassOf<UArenaTheme>> Themes;

	UPROPERTY()
	TObjectPtr<const UArenaTheme> Theme;

	// The first rule is the filler type: it gets whatever the other types' shares leave over.
	UPROPERTY(EditDefaultsOnly, Category = "Waves")
	TArray<FEnemySpawnRule> SpawnRules;

	UPROPERTY(EditDefaultsOnly, Category = "Waves")
	int32 FirstWaveSize = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Waves")
	int32 ExtraEnemiesPerRound = 2;

	UPROPERTY(EditDefaultsOnly, Category = "Waves")
	int32 MaxWaveSize = 30;

	UPROPERTY(EditDefaultsOnly, Category = "Waves")
	float SecondsBetweenWaves = 2.f;

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

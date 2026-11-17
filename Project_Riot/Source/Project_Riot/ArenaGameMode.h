#pragma once

#include "CoreMinimal.h"
#include "ArenaStage.h"
#include "GameFramework/GameModeBase.h"
#include "ArenaGameMode.generated.h"

class AArenaEnemy;
class AArenaPlayerPawn;
class AStoryScene;
enum class EArenaScene : uint8;
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

// A "COOL!"-style callout over a kill, for the HUD to draw.
struct FScorePopup
{
	FVector Where = FVector::ZeroVector;
	int32 Tier = 0;  // 0 = plain kill, 1..5 = COOL .. LEGENDARY
	int32 Points = 0;
	double Time = 0.0;
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
	void RestartArena();

	// Score: kills by the same spell (one chain throw, one Q burst) chain into combos,
	// and clearing a wave quickly adds a bonus. Resets every run.
	void OnEnemyKilled(const AArenaEnemy& Enemy, const UObject* Source);
	int32 GetScore() const { return Score; }
	const TArray<FScorePopup>& GetPopups() const { return Popups; }
	const struct FEnemySkin* GetEnemySkin(TSubclassOf<AArenaEnemy> Type) const;

	// Which scene this load is showing, and its story scene actor if it has one.
	EArenaScene GetSceneKind() const { return SceneKind; }
	bool IsCombatStage() const;
	AStoryScene* GetScene() const { return Scene; }
	// The chosen hero's route across the world map.
	const TArray<FStageDef>& GetRoute() const;
	// Debug cheat: wipe the stage and count it as cleared.
	void BeatStage();
	int32 GetStageWaves() const;
	bool IsStageCleared() const { return bStageCleared; }

	// Story art and sounds are looked up by name, so placeholder art can be swapped
	// by replacing the asset without touching code.
	UPaperSprite* GetStoryArt(FName Name) const { return StoryArt.FindRef(Name); }
	USoundBase* GetStorySound(FName Name) const { return StorySounds.FindRef(Name); }
	void SetFloor(UPaperSprite* Sprite, const FLinearColor& Tint);
	void KillAllEnemies();

	// The stage's theme (the plain defaults outside combat).
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
	void AwardWaveBonus();
	void BankGold();

	// Playable heroes, shown on the start screen.
	UPROPERTY(EditDefaultsOnly, Category = "Heroes")
	TArray<TSubclassOf<AArenaPlayerPawn>> Heroes;

	UPROPERTY(EditDefaultsOnly, Category = "Story")
	TMap<FName, TObjectPtr<UPaperSprite>> StoryArt;

	UPROPERTY(EditDefaultsOnly, Category = "Story")
	TMap<FName, TObjectPtr<USoundBase>> StorySounds;

	UPROPERTY(EditDefaultsOnly, Category = "Story")
	TObjectPtr<USoundBase> TownMusic;

	UPROPERTY(EditDefaultsOnly, Category = "Story")
	TObjectPtr<USoundBase> MapMusic;

	UPROPERTY()
	TObjectPtr<AStoryScene> Scene;

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
	bool bStageCleared = false;
	bool bGoldBanked = false;
	EArenaScene SceneKind;

	int32 Score = 0;
	double WaveStartedAt = 0.0;
	TMap<TWeakObjectPtr<const UObject>, int32> KillsBySource;
	TArray<FScorePopup> Popups;

	FTimerHandle CheckTimer;
	FTimerHandle WaveTimer;
};

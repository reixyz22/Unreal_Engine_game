#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ArenaLeaderboard.generated.h"

USTRUCT()
struct FLeaderboardEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FString Initials;

	UPROPERTY()
	int32 Score = 0;

	UPROPERTY()
	int32 Wave = 0;
};

// Arcade-style top scores with three-letter initials, saved to disk between launches.
UCLASS()
class PROJECT_RIOT_API UArenaLeaderboard : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 MaxEntries = 10;

	static UArenaLeaderboard* Load();
	void Save();

	bool Qualifies(int32 Score) const;
	void Add(const FString& Initials, int32 Score, int32 Wave);

	UPROPERTY()
	TArray<FLeaderboardEntry> Entries;
};

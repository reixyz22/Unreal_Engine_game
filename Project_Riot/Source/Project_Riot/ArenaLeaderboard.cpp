#include "ArenaLeaderboard.h"

#include "Kismet/GameplayStatics.h"

namespace
{
	const FString SlotName(TEXT("Leaderboard"));
}

UArenaLeaderboard* UArenaLeaderboard::Load()
{
	if (UArenaLeaderboard* Saved = Cast<UArenaLeaderboard>(UGameplayStatics::LoadGameFromSlot(SlotName, 0)))
	{
		return Saved;
	}
	return Cast<UArenaLeaderboard>(UGameplayStatics::CreateSaveGameObject(StaticClass()));
}

void UArenaLeaderboard::Save()
{
	UGameplayStatics::SaveGameToSlot(this, SlotName, 0);
}

bool UArenaLeaderboard::Qualifies(int32 Score) const
{
	return Score > 0 && (Entries.Num() < MaxEntries || Score > Entries.Last().Score);
}

void UArenaLeaderboard::Add(const FString& Initials, int32 Score, int32 Wave)
{
	Entries.Add({ Initials, Score, Wave });
	Entries.Sort([](const FLeaderboardEntry& A, const FLeaderboardEntry& B) { return A.Score > B.Score; });
	if (Entries.Num() > MaxEntries)
	{
		Entries.SetNum(MaxEntries);
	}
}

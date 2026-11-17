#pragma once

#include "CoreMinimal.h"
#include "ArenaStats.h"

class UArenaSession;

// Something bought with gold at the village stall: a stat upgrade or a special.
struct FShopItem
{
	FName Id;
	FString Name;
	FString Description;
	int32 Cost = 0;
	// How many times it can be bought (stat upgrades stack; specials are bought once).
	int32 MaxCount = 1;
	FArenaStats Bonus = FArenaStats::Zero();
	// Script section played as dialog when it is bought, if any.
	FString BoughtSection;
};

namespace ArenaShop
{
	// Buying this unlocks Nikki's Q.
	inline const FName Guitar(TEXT("Guitar"));

	const TArray<FShopItem>& Items();
	bool CanBuy(const UArenaSession& Session, const FShopItem& Item);
	bool TryBuy(UArenaSession& Session, const FShopItem& Item);
	// Everything the player has bought, summed.
	FArenaStats TotalBonus(const UArenaSession& Session);
}

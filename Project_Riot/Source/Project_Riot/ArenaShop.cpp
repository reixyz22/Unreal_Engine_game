#include "ArenaShop.h"

#include "ArenaSession.h"

namespace
{
	FShopItem StatItem(const TCHAR* Id, const TCHAR* Name, const TCHAR* Description, int32 Cost, float FArenaStats::* Stat, float Amount)
	{
		FShopItem Item{ Id, Name, Description, Cost, 5 };
		Item.Bonus.*Stat = Amount;
		return Item;
	}
}

// Tuning table: names, prices and amounts all live here.
const TArray<FShopItem>& ArenaShop::Items()
{
	static const TArray<FShopItem> Table = {
		{ Guitar, TEXT("Guitar"), TEXT("Unlocks Q: a shock blast that hits the whole screen."), 5000, 1, FArenaStats::Zero(), TEXT("bought_guitar") },
		StatItem(TEXT("AD"), TEXT("Heavier Flail"), TEXT("+4 Attack Damage"), 1500, &FArenaStats::AttackDamage, 4.f),
		StatItem(TEXT("AP"), TEXT("Louder Amp"), TEXT("+4 Ability Power"), 1500, &FArenaStats::AbilityPower, 4.f),
		StatItem(TEXT("HP"), TEXT("Hearty Breakfast"), TEXT("+20 Max Health"), 1200, &FArenaStats::MaxHealth, 20.f),
		StatItem(TEXT("Armor"), TEXT("Leather Jacket"), TEXT("+10 Armor"), 1200, &FArenaStats::Armor, 10.f),
		StatItem(TEXT("MR"), TEXT("Lucky Charm"), TEXT("+10 Magic Resist"), 1200, &FArenaStats::MagicResist, 10.f),
		StatItem(TEXT("MS"), TEXT("Good Boots"), TEXT("+20 Move Speed"), 1000, &FArenaStats::MoveSpeed, 20.f),
	};
	return Table;
}

bool ArenaShop::CanBuy(const UArenaSession& Session, const FShopItem& Item)
{
	return Session.Gold >= Item.Cost && Session.Purchases.FindRef(Item.Id) < Item.MaxCount;
}

bool ArenaShop::TryBuy(UArenaSession& Session, const FShopItem& Item)
{
	if (!CanBuy(Session, Item))
	{
		return false;
	}
	Session.Gold -= Item.Cost;
	++Session.Purchases.FindOrAdd(Item.Id);
	return true;
}

FArenaStats ArenaShop::TotalBonus(const UArenaSession& Session)
{
	FArenaStats Total = FArenaStats::Zero();
	for (const FShopItem& Item : Items())
	{
		Total.Add(Item.Bonus, Session.Purchases.FindRef(Item.Id));
	}
	return Total;
}

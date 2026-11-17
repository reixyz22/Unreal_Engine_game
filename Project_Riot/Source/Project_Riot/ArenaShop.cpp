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

	FShopItem Special(FName Id, const TCHAR* Name, const TCHAR* Description, int32 Cost, const TCHAR* Hero, const TCHAR* Section)
	{
		FShopItem Item{ Id, Name, Description, Cost, 1, FArenaStats::Zero(), Section };
		Item.Hero = Hero;
		return Item;
	}

	FShopItem Outfit(const TCHAR* Id, const TCHAR* Name, int32 Cost, const FLinearColor& Tint)
	{
		FShopItem Item{ Id, Name, TEXT("Outfit (tester: tints the hero)"), Cost, 1 };
		Item.bOutfit = true;
		Item.Tint = Tint;
		return Item;
	}
}

// Tuning table: names, prices and amounts all live here. Prices are in cents.
// For scale: a kill pays about a dollar, and the contract pays thousands.
const TArray<FShopItem>& ArenaShop::Items()
{
	static const TArray<FShopItem> Table = {
		Special(Guitar, TEXT("Guitar"), TEXT("Unlocks Q: a shock blast that hits the whole screen."), 5000, TEXT("Nikki"), TEXT("bought_guitar")),
		Special(Volley, TEXT("Volley"), TEXT("Unlocks 2: a fan of arrows."), 5000, TEXT("Carl"), TEXT("bought_volley")),
		StatItem(TEXT("AD"), TEXT("Sharper Edge"), TEXT("+4 Attack Damage"), 1500, &FArenaStats::AttackDamage, 4.f),
		StatItem(TEXT("AP"), TEXT("Louder Amp"), TEXT("+4 Ability Power"), 1500, &FArenaStats::AbilityPower, 4.f),
		StatItem(TEXT("HP"), TEXT("Hearty Breakfast"), TEXT("+20 Max Health"), 1200, &FArenaStats::MaxHealth, 20.f),
		StatItem(TEXT("Armor"), TEXT("Leather Jacket"), TEXT("+10 Armor"), 1200, &FArenaStats::Armor, 10.f),
		StatItem(TEXT("MR"), TEXT("Lucky Charm"), TEXT("+10 Magic Resist"), 1200, &FArenaStats::MagicResist, 10.f),
		StatItem(TEXT("MS"), TEXT("Good Boots"), TEXT("+20 Move Speed"), 1000, &FArenaStats::MoveSpeed, 20.f),
		Outfit(TEXT("OutfitGold"), TEXT("Gold Outfit"), 800, FLinearColor(1.f, 0.85f, 0.4f)),
		Outfit(TEXT("OutfitShadow"), TEXT("Shadow Outfit"), 800, FLinearColor(0.45f, 0.4f, 0.6f)),
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

FLinearColor ArenaShop::OutfitTint(const UArenaSession& Session)
{
	for (const FShopItem& Item : Items())
	{
		if (Item.bOutfit && Item.Id == Session.Outfit)
		{
			return Item.Tint;
		}
	}
	return FLinearColor::White;
}

FString ArenaShop::Money(int32 Cents)
{
	return FString::Printf(TEXT("$%d.%02d"), Cents / 100, Cents % 100);
}

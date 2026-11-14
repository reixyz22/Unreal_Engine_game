#include "ArenaTheme.h"

#include "ArenaEnemy.h"

const FEnemySkin* UArenaTheme::FindSkin(const UClass* EnemyClass) const
{
	// Most specific match wins, so a skin for a parent class covers its children too.
	const FEnemySkin* Best = nullptr;
	for (const FEnemySkin& Skin : EnemySkins)
	{
		if (Skin.Type && EnemyClass->IsChildOf(Skin.Type) && (!Best || Skin.Type->IsChildOf(Best->Type)))
		{
			Best = &Skin;
		}
	}
	return Best;
}

#include "CarlPawn.h"

#include "ArenaArrow.h"
#include "ArenaSession.h"
#include "ArenaShop.h"
#include "GameFramework/PlayerController.h"

ACarlPawn::ACarlPawn()
{
	HeroName = NSLOCTEXT("Arena", "Carl", "Carl");
	ScriptName = TEXT("Carl");
	PointerCursor = TEXT("Slate/Cursors/Carl_Pointer");
	AttackCursor = TEXT("Slate/Cursors/Carl_Sword");

	// 30 attack damage one-shots a Pyro. He is frailer and a touch quicker than Nikki.
	BaseStats.MaxHealth = 80.f;
	BaseStats.AttackDamage = 30.f;
	BaseStats.AbilityPower = 0.f;
	BaseStats.MoveSpeed = 420.f;
}

void ACarlPawn::HandleRightClick()
{
	FVector Cursor;
	if (GetWorld()->GetTimeSeconds() < NextShotAt || !GetCursorWorldLocation(Cursor))
	{
		return;
	}
	Shoot(Cursor - GetActorLocation());
	NextShotAt = GetWorld()->GetTimeSeconds() + SecondsBetweenShots;
}

void ACarlPawn::HandleSpecialKeys(const APlayerController& PlayerController)
{
	if (PlayerController.WasInputKeyJustPressed(EKeys::One) && Mana >= SpecialManaCost)
	{
		Mana -= SpecialManaCost;
		RicochetShotsLeft = RicochetShots;
	}

	const UArenaSession* Session = UArenaSession::Get(this);
	FVector Cursor;
	if (PlayerController.WasInputKeyJustPressed(EKeys::Two) && Session && Session->Owns(ArenaShop::Volley)
		&& Mana >= SpecialManaCost && GetCursorWorldLocation(Cursor))
	{
		Mana -= SpecialManaCost;
		const FVector Aim = Cursor - GetActorLocation();
		for (int32 Index = 0; Index < VolleyArrows; ++Index)
		{
			// Spread the arrows evenly across the fan, centred on the cursor.
			const float Alpha = VolleyArrows > 1 ? float(Index) / (VolleyArrows - 1) - 0.5f : 0.f;
			Shoot(Aim.RotateAngleAxis(Alpha * VolleySpreadDegrees, FVector::YAxisVector));
		}
	}
}

void ACarlPawn::Shoot(const FVector& Direction)
{
	AArenaArrow* Arrow = GetWorld()->SpawnActor<AArenaArrow>(GetActorLocation(), FRotator::ZeroRotator);
	if (!Arrow)
	{
		return;
	}
	const bool bRicochet = RicochetShotsLeft > 0;
	RicochetShotsLeft = FMath::Max(0, RicochetShotsLeft - 1);
	Arrow->Fire(ArrowSprite, Direction, ArrowSpeed, Stats.AttackDamage * AutoDamageRatio, bRicochet ? BouncesPerArrow : 0);
	UArenaSession::PlaySfx(this, ThrowSound);
}

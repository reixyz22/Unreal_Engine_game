#include "EndingScene.h"

#include "ArenaPlayerPawn.h"
#include "ArenaSession.h"

void AEndingScene::StartScene()
{
	LoadHeroScript();
	SetFloor(TEXT("Floor_Town"));
	Bounds = FBox2D(FVector2D(-600.0), FVector2D(600.0));
	MoveHeroTo(FVector2D::ZeroVector);
	bLockInput = true;

	const AArenaPlayerPawn* Hero = GetHero();
	const bool bFinal = GetSession().bFinalRoute;
	const FString Route = bFinal ? FString(TEXT("Final")) : Hero ? Hero->GetScriptName() : FString(TEXT("Nikki"));

	After(1.f, [this, bFinal, Route]()
	{
		Say(bFinal ? TEXT("ending_final") : TEXT("ending"), [this, Route]()
		{
			if (const AArenaPlayerPawn* Star = GetHero())
			{
				Popup(TEXT("THE END"), *Star);
			}
			Sfx(TEXT("Fanfare"));
			// Mark the route finished, save, and go back to the title screen.
			After(3.f, [this, Route]() { GetSession().FinishRoute(this, Route); });
		});
	});
}

#include "TownScene.h"

#include "ArenaPlayerPawn.h"
#include "ArenaSession.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "StoryProp.h"

namespace
{
	constexpr double TownHalf = 1500.0;
	const FVector2D FromHutSpot(0.0, 1040.0);
	const FVector2D FromMapSpot(0.0, -1340.0);
	constexpr double GateLine = -1450.0;
	constexpr double GateHalfWidth = 80.0;

	// The village layout. Each row is one thing you can walk up to and read or talk to;
	// Section names a block in Content/Story/Nikki.txt. Add rows here to fill the town in.
	struct FTownSpot
	{
		const TCHAR* Art;
		FVector2D Where;
		const TCHAR* Prompt;
		const TCHAR* Section;
	};
	const FTownSpot Spots[] = {
		{ TEXT("House_Nikki"),   FVector2D(0.0, 1180.0),    TEXT("Home"),            TEXT("home_door") },
		{ TEXT("House_Evicted"), FVector2D(-520.0, 620.0),  TEXT("Read the notice"), TEXT("sign_eviction") },
		{ TEXT("House_Notice"),  FVector2D(520.0, 620.0),   TEXT("Read the paper"),  TEXT("sign_rent") },
		{ TEXT("House_Nice"),    FVector2D(-600.0, -60.0),  TEXT("Look"),            TEXT("house_nice") },
		{ TEXT("Yard_Sign"),     FVector2D(-400.0, -210.0), TEXT("Read the sign"),   TEXT("sign_vote") },
		{ TEXT("House_Plain"),   FVector2D(560.0, -60.0),   TEXT("Look"),            TEXT("house_plain") },
		{ TEXT("Sign_Post"),     FVector2D(110.0, 860.0),   TEXT("Read the sign"),   TEXT("sign_village") },
		{ TEXT("Well"),          FVector2D(-220.0, -430.0), TEXT("Look in"),         TEXT("well") },
		{ TEXT("NPC_Sarah"),     FVector2D(330.0, -640.0),  TEXT("Talk"),            TEXT("sarah_talk") },
	};

	// Scenery with nothing to say.
	const FVector2D Trees[] = {
		FVector2D(-1100.0, 1100.0), FVector2D(1000.0, 1150.0), FVector2D(-1200.0, 300.0), FVector2D(1150.0, 320.0),
		FVector2D(-1000.0, -600.0), FVector2D(1050.0, -500.0), FVector2D(-900.0, -1200.0), FVector2D(900.0, -1150.0),
		FVector2D(-260.0, 300.0), FVector2D(280.0, 260.0),
	};
}

void ATownScene::StartScene()
{
	LoadHeroScript();
	// Carl's city is a placeholder mirror image of Nikki's village.
	const FVector2D Flip(IsHero(TEXT("Carl")) ? -1.0 : 1.0, 1.0);
	SetFloor(TEXT("Floor_Town"));
	Bounds = FBox2D(FVector2D(-TownHalf), FVector2D(TownHalf));

	for (const FTownSpot& Spot : Spots)
	{
		AddTalker(Spot.Art, Spot.Where * Flip, Spot.Prompt, Spot.Section);
	}
	for (const FVector2D& Tree : Trees)
	{
		AddProp(TEXT("Tree"), Tree * Flip);
	}

	UArenaSession& Session = GetSession();
	Contract = AddProp(Session.bContractSigned ? TEXT("Contract_Signed") : TEXT("Contract"), FVector2D(330.0, -740.0) * Flip);
	if (!Session.bContractSigned)
	{
		Contract->Prompt = TEXT("Sign with your flail");
		Contract->bSwing = true;
		Contract->OnUse = [this]() { SignContract(); };
		Objective = TEXT("Find the village elder by the south gate.");
		PointAt(Contract);
	}

	Gate = AddProp(TEXT("Gate"), FVector2D(0.0, -1470.0), false);

	MoveHeroTo(Session.PreviousScene == EArenaScene::Hut ? FromHutSpot : FromMapSpot);
	if (AArenaPlayerPawn* Hero = GetHero())
	{
		Hero->SetPyjamas(false);
	}
}

AStoryProp* ATownScene::AddTalker(FName ArtName, const FVector2D& Where, const FString& Prompt, const FString& Section, bool bBlocksMovement)
{
	AStoryProp* Prop = AddProp(ArtName, Where, bBlocksMovement);
	Prop->Prompt = Prompt;
	Prop->OnUse = [this, Section]() { Say(Section); };
	return Prop;
}

void ATownScene::SignContract()
{
	// Read the terms first; the flail comes down when the dialog ends.
	Contract->OnUse = nullptr;
	Say(TEXT("contract"), [this]()
	{
		Contract->SetArt(Art(TEXT("Contract_Signed")));
		Contract->Punch();
		Sfx(TEXT("Stamp"));
		Popup(TEXT("SIGNED!"), *Contract);
		GetSession().bContractSigned = true;
		After(0.8f, [this]()
		{
			Say(TEXT("contract_signed"), [this]()
			{
				Objective = TEXT("Leave through the south gate.");
				PointAt(Gate);
			});
		});
	});
}

void ATownScene::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AArenaPlayerPawn* Hero = GetHero();
	if (!Hero || bLeaving || Hero->GetActorLocation().Z > GateLine || FMath::Abs(Hero->GetActorLocation().X) > GateHalfWidth)
	{
		bWarnedAtGate &= Hero && Hero->GetActorLocation().Z < GateLine + 120.0;
		return;
	}

	if (GetSession().bContractSigned)
	{
		bLeaving = true;
		GetSession().Travel(this, EArenaScene::WorldMap);
	}
	else if (!bWarnedAtGate && !IsDialogOpen())
	{
		// No leaving without a job: say why once, until she walks away and comes back.
		bWarnedAtGate = true;
		Say(TEXT("gate_locked"));
	}
}

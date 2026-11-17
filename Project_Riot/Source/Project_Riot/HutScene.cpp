#include "HutScene.h"

#include "ArenaPlayerPawn.h"
#include "ArenaSession.h"
#include "StoryProp.h"

namespace
{
	// The room, in world units (the floor art is drawn to match; see build_story_art.py).
	const FVector2D RoomHalf(288.0, 224.0);
	const FVector2D BedSpot(-200.0, 70.0);
	const FVector2D NightstandSpot(-120.0, 180.0);
	const FVector2D ClosetSpot(200.0, 160.0);
	const FVector2D DoorSpot(0.0, -248.0);
	const FVector2D WakeSpot(-110.0, 50.0);
	// Where she sits up in bed (the swing frames put her torso here); the chain starts from her.
	const FVector2D InBedSpot(-200.0, 100.0);
	// The ball flies out past the foot of the bed.
	const FVector2D BedSwingAim(0.0, -140.0);
	constexpr float BedSwingFrameSeconds = 0.06f;
	const FVector2D EntrySpot(0.0, -140.0);
	constexpr double ExitLine = -185.0;
	constexpr double DoorHalfWidth = 50.0;
	constexpr float PhoneCharsPerSecond = 26.f;
}

void AHutScene::StartScene()
{
	LoadHeroScript();
	SetFloor(TEXT("Floor_Hut"));
	Bounds = FBox2D(-RoomHalf, RoomHalf);

	UArenaSession& Session = GetSession();
	const bool bCarl = IsHero(TEXT("Carl"));
	Bed = AddProp(Session.bBedSmashed ? TEXT("Bed_Broken") : bCarl ? TEXT("Bed") : TEXT("Bed_Nikki"), BedSpot);
	Nightstand = AddProp(TEXT("Nightstand"), NightstandSpot);
	Closet = AddProp(TEXT("Closet"), ClosetSpot);
	Door = AddProp(TEXT("Door"), DoorSpot, false);

	if (Session.bIntroSeen)
	{
		// Back home later: no cutscene, just the room as she left it.
		bDoorOpen = true;
		MoveHeroTo(EntrySpot);
		return;
	}
	if (bCarl)
	{
		PlayCarlIntro();
		return;
	}
	PlayIntro();
}

void AHutScene::PlayIntro()
{
	bLockInput = true;
	bInBed = true;
	MoveHeroTo(WakeSpot);

	// The phone buzzes on the nightstand, then slides up with Carl's text.
	After(1.2f, [this]()
	{
		Sfx(TEXT("PhoneBuzz"));
		Nightstand->Wobble(1.2f);
		Alert = AddProp(TEXT("Alert"), NightstandSpot + FVector2D(0.0, 70.0), false);
		Alert->bOverlay = true;
	});
	After(2.8f, [this]()
	{
		const TArray<FStoryLine> Text = Script.Get(TEXT("phone_text"));
		Phone.bShown = true;
		Phone.Sender = Text[0].Speaker;
		Phone.Message = Text[0].Text;
		Phone.CharsShown = 0.f;
		bPhoneTyping = true;
	});
}

void AHutScene::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bInBed)
	{
		if (AArenaPlayerPawn* Hero = GetHero())
		{
			Hero->SetActorHiddenInGame(true);
		}
	}

	if (bPhoneTyping)
	{
		Phone.CharsShown += PhoneCharsPerSecond * DeltaTime;
		if (Phone.CharsShown >= Phone.Message.Len())
		{
			bPhoneTyping = false;
			After(1.5f, [this]() { OnPhoneRead(); });
		}
	}

	// Walking out through the door leads to the village.
	const AArenaPlayerPawn* Hero = GetHero();
	if (Hero && bDoorOpen && !bLeaving && Hero->GetActorLocation().Z < ExitLine && FMath::Abs(Hero->GetActorLocation().X) < DoorHalfWidth)
	{
		bLeaving = true;
		GetSession().bIntroSeen = true;
		GetSession().Travel(this, EArenaScene::Town);
	}
}

void AHutScene::PlayCarlIntro()
{
	// He is already awake, sitting on the edge of the bed with his phone in reach.
	MoveHeroTo(WakeSpot);
	Objective = TEXT("Your phone is on the nightstand.");
	PointAt(Nightstand);
	Nightstand->Prompt = TEXT("Text your crush");
	Nightstand->OnUse = [this]() { TextCrush(); };
}

void AHutScene::TextCrush()
{
	Nightstand->OnUse = nullptr;
	PointAt(nullptr);
	Objective.Reset();
	bLockInput = true;

	const TArray<FStoryLine> Text = Script.Get(TEXT("phone_text"));
	Phone.bShown = true;
	Phone.Sender = TEXT("Nikki");
	Phone.Message = Text[0].Text;
	Phone.CharsShown = 0.f;
	bPhoneTyping = true;
}

void AHutScene::BackToBed()
{
	if (AArenaPlayerPawn* Hero = GetHero())
	{
		Hero->SetActorHiddenInGame(true);
	}
	Bed->SetArt(Art(TEXT("Bed_Carl")));
	Bed->Punch();
	Popup(TEXT("...zzz"), *Bed);
	After(3.f, [this]() { WakeWithDog(); });
}

void AHutScene::WakeWithDog()
{
	Bed->SetArt(Art(TEXT("Bed")));
	Sfx(TEXT("Poof"));
	if (AArenaPlayerPawn* Hero = GetHero())
	{
		Hero->SetActorHiddenInGame(false);
	}
	AStoryProp* Dog = AddProp(TEXT("Dog"), WakeSpot + FVector2D(70.0, -30.0), false);
	Dog->Wobble(2.5f);

	Say(TEXT("dog_wake"), [this]()
	{
		bLockInput = false;
		GetSession().bDressed = true;
		Objective = TEXT("Head outside.");
		PointAt(Door);
		bDoorOpen = true;
	});
}

void AHutScene::OnPhoneRead()
{
	if (IsHero(TEXT("Carl")))
	{
		// His text never lands: the block shows first, then it sinks in.
		Phone.bBlocked = true;
		Sfx(TEXT("Stamp"));
		After(1.6f, [this]()
		{
			Say(TEXT("phone_react"), [this]()
			{
				Phone.bShown = false;
				BackToBed();
			});
		});
		return;
	}

	Say(TEXT("phone_react"), [this]()
	{
		Phone.bBlocked = true;
		Sfx(TEXT("Stamp"));
		After(1.4f, [this]()
		{
			Phone.bShown = false;
			SoundAlarm();
		});
	});
}

void AHutScene::SoundAlarm()
{
	// Still in bed. The alarm goes off; she can't move until she swings her way out.
	Sfx(TEXT("Alarm"));
	Nightstand->Wobble(1.5f);
	Popup(TEXT("BEEP BEEP BEEP"), *Nightstand);
	After(1.2f, [this]()
	{
		Say(TEXT("alarm"), [this]()
		{
			if (Alert)
			{
				Alert->Destroy();
			}
			bLockInput = false;
			bHoldStill = true;
			MoveHeroTo(InBedSpot);
			Objective = TEXT("Get out of bed.  Right-click it.");
			PointAt(Bed);
			// She swings from under the covers; the ball lands past the bed, not on it.
			Bed->MaceAim = BedSwingAim;
			Bed->OnMaceThrown = [this]()
			{
				Bed->OnMaceThrown = nullptr;
				PointAt(nullptr);
				TArray<UPaperSprite*> Swing;
				for (int32 Index = 0; Index < 6; ++Index)
				{
					Swing.Add(Art(*FString::Printf(TEXT("Bed_Nikki_Swing_%d"), Index)));
				}
				Bed->PlayFrames(Swing, BedSwingFrameSeconds);
			};
			Bed->OnMaceHit = [this]() { SmashOutOfBed(); };
		});
	});
}

void AHutScene::SmashOutOfBed()
{
	// The ball lands and the bed gives way; she tumbles out beside it as the chain comes back.
	Bed->OnMaceHit = nullptr;
	Bed->SetArt(Art(TEXT("Bed_Broken")));
	Bed->Punch();
	Sfx(TEXT("Smash"));
	Popup(TEXT("WHAM!"), *Bed);
	MoveHeroTo(WakeSpot);
	bInBed = false;
	if (AArenaPlayerPawn* Hero = GetHero())
	{
		Hero->SetActorHiddenInGame(false);
	}
	bHoldStill = false;
	Objective.Reset();
	GetSession().bBedSmashed = true;

	Say(TEXT("bed_smashed"), [this]()
	{
		Objective = TEXT("Get dressed at the closet.");
		PointAt(Closet);
		Closet->Prompt = TEXT("Get dressed");
		Closet->OnUse = [this]() { GetDressed(); };
	});
}

void AHutScene::GetDressed()
{
	Closet->OnUse = nullptr;
	Closet->Punch();
	Sfx(TEXT("Poof"));
	// The pawn reads bDressed, so this is what swaps the pyjamas for her clothes.
	GetSession().bDressed = true;
	if (const AArenaPlayerPawn* Hero = GetHero())
	{
		Popup(TEXT("POOF!"), *Hero);
	}

	Say(TEXT("dressed"), [this]()
	{
		Objective = TEXT("Head outside.");
		PointAt(Door);
		bDoorOpen = true;
	});
}

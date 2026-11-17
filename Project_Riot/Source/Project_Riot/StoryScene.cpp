#include "StoryScene.h"

#include "ArenaGameMode.h"
#include "ArenaPlayerPawn.h"
#include "ArenaSession.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "StoryProp.h"
#include "TimerManager.h"

namespace
{
	constexpr float CharsPerSecond = 45.f;
	constexpr int32 CharsPerBlip = 3;
	constexpr double StoryPopupSeconds = 1.1;
	// How close the hero must be to a prop's edge to use it.
	constexpr double UseReach = 90.0;
	// Blockers are shrunk a little so walking past furniture doesn't feel sticky.
	constexpr double BlockerShrink = 0.85;
}

AStoryScene::AStoryScene()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AStoryScene::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bStarted)
	{
		bStarted = true;
		Arrow = AddProp(TEXT("Arrow"), FVector2D::ZeroVector, false);
		Arrow->bOverlay = true;
		Arrow->SetActorHiddenInGame(true);
		StartScene();
	}

	// A line queued from somewhere she couldn't talk (a purchase on the world map).
	UArenaSession& Session = GetSession();
	if (!Session.PendingSection.IsEmpty() && !BlocksInput())
	{
		Say(Session.PendingSection);
		Session.PendingSection.Reset();
	}

	TickDialog(DeltaTime);
	UpdateFocus();

	const double Now = GetWorld()->GetTimeSeconds();
	Popups.RemoveAll([Now](const FStoryPopup& Popup) { return Now - Popup.Time > StoryPopupSeconds; });

	if (const AStoryProp* Target = PointTarget.Get())
	{
		const double Bob = FMath::Abs(FMath::Sin(Now * 5.0)) * 24.0;
		Arrow->SetActorLocation(Target->GetActorLocation() + FVector(0.0, 0.0, Target->GetHalfSize().Y + 40.0 + Bob));
	}
}

void AStoryScene::TickDialog(float DeltaTime)
{
	if (!IsDialogOpen())
	{
		return;
	}

	const int32 Before = GetCharsShown();
	const int32 Length = GetLine().Text.Len();
	CharsShown = FMath::Min(CharsShown + CharsPerSecond * DeltaTime, float(Length));
	if (GetCharsShown() / CharsPerBlip != Before / CharsPerBlip)
	{
		Sfx(TEXT("Blip"));
	}

	const APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0);
	const bool bAdvance = Controller && (Controller->WasInputKeyJustPressed(EKeys::SpaceBar) || Controller->WasInputKeyJustPressed(EKeys::Enter)
		|| Controller->WasInputKeyJustPressed(EKeys::E) || Controller->WasInputKeyJustPressed(EKeys::LeftMouseButton));
	if (!bAdvance)
	{
		return;
	}

	// First press finishes the line; the next one moves on.
	if (GetCharsShown() < Length)
	{
		CharsShown = Length;
		return;
	}
	CharsShown = 0.f;
	if (++LineIndex >= Lines.Num())
	{
		LineIndex = INDEX_NONE;
		Lines.Reset();
		if (TFunction<void()> Then = MoveTemp(AfterDialog))
		{
			AfterDialog = nullptr;
			Then();
		}
	}
}

void AStoryScene::Say(const FString& Section, TFunction<void()> Then)
{
	Lines = Script.Get(Section);
	LineIndex = 0;
	CharsShown = 0.f;
	AfterDialog = MoveTemp(Then);
}

void AStoryScene::UpdateFocus()
{
	FocusProp = nullptr;
	const AArenaPlayerPawn* Hero = GetHero();
	if (!Hero || BlocksInput())
	{
		return;
	}

	// The nearest usable prop whose edge is within reach.
	double Best = UseReach;
	for (AStoryProp* Prop : Props)
	{
		if (!Prop || !Prop->IsUsable())
		{
			continue;
		}
		const FVector Offset = Hero->GetActorLocation() - Prop->GetActorLocation();
		const double GapX = FMath::Max(0.0, FMath::Abs(Offset.X) - Prop->GetHalfSize().X);
		const double GapZ = FMath::Max(0.0, FMath::Abs(Offset.Z) - Prop->GetHalfSize().Y);
		const double Gap = FMath::Sqrt(GapX * GapX + GapZ * GapZ);
		if (Gap < Best)
		{
			Best = Gap;
			FocusProp = Prop;
		}
	}
}

AStoryProp* AStoryScene::FindMaceTargetAt(const FVector& Point) const
{
	if (bLockInput || IsDialogOpen())
	{
		return nullptr;
	}
	for (AStoryProp* Prop : Props)
	{
		if (!Prop || !Prop->OnMaceHit)
		{
			continue;
		}
		const FVector Offset = Point - Prop->GetActorLocation();
		if (FMath::Abs(Offset.X) <= Prop->GetHalfSize().X && FMath::Abs(Offset.Z) <= Prop->GetHalfSize().Y)
		{
			return Prop;
		}
	}
	return nullptr;
}

bool AStoryScene::UseFocusProp()
{
	AStoryProp* Prop = FocusProp.Get();
	if (!Prop || BlocksInput())
	{
		return false;
	}
	const bool bSwing = Prop->bSwing;
	Prop->OnUse();
	return bSwing;
}

FVector AStoryScene::ResolveMove(const FVector& From, const FVector& To, float Radius) const
{
	auto IsBlocked = [this, Radius](double X, double Z)
	{
		if (X < Bounds.Min.X + Radius || X > Bounds.Max.X - Radius || Z < Bounds.Min.Y + Radius || Z > Bounds.Max.Y - Radius)
		{
			return true;
		}
		for (const FBox2D& Box : Blockers)
		{
			if (X > Box.Min.X - Radius && X < Box.Max.X + Radius && Z > Box.Min.Y - Radius && Z < Box.Max.Y + Radius)
			{
				return true;
			}
		}
		return false;
	};

	// Try each axis on its own, so walking into a wall at an angle slides along it.
	FVector Result = From;
	if (!IsBlocked(To.X, Result.Z))
	{
		Result.X = To.X;
	}
	if (!IsBlocked(Result.X, To.Z))
	{
		Result.Z = To.Z;
	}
	return Result;
}

AStoryProp* AStoryScene::AddProp(FName ArtName, const FVector2D& Where, bool bBlocksMovement)
{
	AStoryProp* Prop = GetWorld()->SpawnActor<AStoryProp>(FVector(Where.X, 0.0, Where.Y), FRotator::ZeroRotator);
	Prop->SetArt(Art(ArtName));
	Props.Add(Prop);
	if (bBlocksMovement)
	{
		const FVector2D Half = Prop->GetHalfSize() * BlockerShrink;
		Blockers.Add(FBox2D(Where - Half, Where + Half));
	}
	return Prop;
}

UPaperSprite* AStoryScene::Art(FName Name) const
{
	const AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();
	return GameMode ? GameMode->GetStoryArt(Name) : nullptr;
}

void AStoryScene::SetFloor(FName ArtName)
{
	if (AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>())
	{
		GameMode->SetFloor(Art(ArtName), FLinearColor::White);
	}
}

void AStoryScene::Sfx(FName Name) const
{
	const AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();
	UArenaSession::PlaySfx(this, GameMode ? GameMode->GetStorySound(Name) : nullptr);
}

void AStoryScene::After(float Seconds, TFunction<void()> Do)
{
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [Do = MoveTemp(Do)]() { Do(); }), Seconds, false);
}

void AStoryScene::Popup(const FString& Text, const AActor& Over)
{
	Popups.Add({ Text, Over.GetActorLocation(), GetWorld()->GetTimeSeconds() });
}

void AStoryScene::PointAt(AStoryProp* Target)
{
	PointTarget = Target;
	Arrow->SetActorHiddenInGame(Target == nullptr);
}

void AStoryScene::MoveHeroTo(const FVector2D& Where)
{
	if (AArenaPlayerPawn* Hero = GetHero())
	{
		Hero->SetActorLocation(FVector(Where.X, Hero->GetActorLocation().Y, Where.Y));
	}
}

void AStoryScene::LoadHeroScript()
{
	const AArenaPlayerPawn* Hero = GetHero();
	Script = FStoryScript::Load(Hero ? Hero->GetScriptName() : TEXT("Nikki"));
}

bool AStoryScene::IsHero(const TCHAR* Name) const
{
	const AArenaPlayerPawn* Hero = GetHero();
	return Hero && Hero->GetScriptName() == Name;
}

UPaperSprite* AStoryScene::GetPortrait() const
{
	if (!IsDialogOpen())
	{
		return nullptr;
	}
	// The hero speaks with her own portrait; anyone else uses art named NPC_<Speaker>.
	const AArenaPlayerPawn* Hero = GetHero();
	if (Hero && GetLine().Speaker == Hero->GetHeroName().ToString())
	{
		return Hero->GetPortrait();
	}
	return Art(FName(*(TEXT("NPC_") + GetLine().Speaker)));
}

AArenaPlayerPawn* AStoryScene::GetHero() const
{
	return Cast<AArenaPlayerPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
}

UArenaSession& AStoryScene::GetSession() const
{
	return *UArenaSession::Get(this);
}

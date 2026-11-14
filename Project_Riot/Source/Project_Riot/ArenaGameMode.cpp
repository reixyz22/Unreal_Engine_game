#include "ArenaGameMode.h"

#include "ArenaArt.h"
#include "ArenaEnemy.h"
#include "ArenaHUD.h"
#include "ArenaPlayerPawn.h"
#include "ArenaSession.h"
#include "ArenaTheme.h"
#include "PaperFlipbook.h"
#include "PaperSpriteActor.h"
#include "PaperSpriteComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

namespace
{
	constexpr float CheckInterval = 0.5f;
	constexpr int32 MaxComboTier = 5;
	constexpr double PopupSeconds = 1.2;
	// Wave bonus: points per wave number, plus points for every second under par.
	constexpr int32 WaveBonusPerRound = 100;
	constexpr double WaveParSeconds = 60.0;
	constexpr int32 PointsPerSecondUnderPar = 10;
	constexpr int32 SpawnAttempts = 8;
}

AArenaGameMode::AArenaGameMode()
{
	HUDClass = AArenaHUD::StaticClass();
}

void AArenaGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// The first run of a launch is always the first theme (the plain dungeon);
	// after that, any theme except the one just played.
	UArenaSession* Session = UArenaSession::Get(this);
	TArray<TSubclassOf<UArenaTheme>> Choices = Themes;
	if (Session && !Session->LastTheme && !Choices.IsEmpty())
	{
		Choices = { Choices[0] };
	}
	else if (Session && Choices.Num() > 1)
	{
		Choices.Remove(Session->LastTheme.Get());
	}
	const TSubclassOf<UArenaTheme> Picked = Choices.IsEmpty() ? nullptr : Choices[FMath::RandRange(0, Choices.Num() - 1)];
	Theme = Picked ? Picked->GetDefaultObject<UArenaTheme>() : GetDefault<UArenaTheme>();
	if (Session)
	{
		Session->LastTheme = Picked;
	}
}

void AArenaGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (UArenaSession* Session = UArenaSession::Get(this))
	{
		Session->PlayMusic(Theme->Music);
	}
	// The arena floor is the level's sprite actor.
	for (TActorIterator<APaperSpriteActor> It(GetWorld()); It; ++It)
	{
		UPaperSpriteComponent* Floor = It->GetRenderComponent();
		if (Theme->FloorSprite)
		{
			// Level sprites are static; the floor has to be movable to change art at runtime.
			Floor->SetMobility(EComponentMobility::Movable);
			Floor->SetSprite(Theme->FloorSprite);
		}
		Floor->SetSpriteColor(Theme->FloorTint);
	}

	GetWorldTimerManager().SetTimer(CheckTimer, this, &AArenaGameMode::CheckWave, CheckInterval, true, 1.f);
}

UClass* AArenaGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	const UArenaSession* Session = UArenaSession::Get(this);
	return Session && Session->ChosenHero ? Session->ChosenHero.Get() : Super::GetDefaultPawnClassForController_Implementation(InController);
}

void AArenaGameMode::ChooseHero(TSubclassOf<AArenaPlayerPawn> Hero)
{
	if (UArenaSession* Session = UArenaSession::Get(this))
	{
		Session->ChosenHero = Hero;
	}

	// Swap the pawn in place if the chosen hero isn't the one already spawned.
	APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0);
	APawn* Current = Controller ? Controller->GetPawn() : nullptr;
	if (!Hero || !Current || Current->IsA(Hero))
	{
		return;
	}
	if (APawn* Replacement = GetWorld()->SpawnActor<APawn>(Hero, Current->GetActorTransform()))
	{
		Controller->Possess(Replacement);
		Current->Destroy();
	}
}

UPaperSprite* AArenaGameMode::GetEnemyIcon(TSubclassOf<AArenaEnemy> Type) const
{
	const FEnemySkin* Skin = Type ? Theme->FindSkin(Type) : nullptr;
	if (Skin && Skin->Anim)
	{
		return Skin->Anim->GetSpriteAtFrame(0);
	}
	return Type ? Type->GetDefaultObject<AArenaEnemy>()->GetIcon() : nullptr;
}

void AArenaGameMode::CheckWave()
{
	if (bPlayerDead || bWaveQueued || CountEnemies() > 0)
	{
		return;
	}
	if (Round > 0)
	{
		AwardWaveBonus();
	}

	PlanWave();
	bWaveQueued = true;
	const float Delay = Round == 0 ? 0.1f : SecondsBetweenWaves;
	GetWorldTimerManager().SetTimer(WaveTimer, this, &AArenaGameMode::StartNextWave, Delay, false);
}

void AArenaGameMode::PlanWave()
{
	Wave.Reset();
	if (SpawnRules.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("BP_ArenaGameMode has no Spawn Rules; waves will be empty."));
		return;
	}

	const int32 NextRound = Round + 1;
	const int32 WaveSize = FMath::Min(FirstWaveSize + ExtraEnemiesPerRound * Round, MaxWaveSize);

	float TotalWeight = 0.f;
	for (const FEnemySpawnRule& Rule : SpawnRules)
	{
		TotalWeight += Rule.FirstRound <= NextRound ? Rule.Weight : 0.f;
	}

	// Every unlocked type after the first gets its share (at least one); the first fills the rest.
	int32 Remaining = WaveSize;
	for (int32 Index = 1; Index < SpawnRules.Num(); ++Index)
	{
		const FEnemySpawnRule& Rule = SpawnRules[Index];
		if (Rule.FirstRound <= NextRound)
		{
			const int32 Count = FMath::Max(1, FMath::FloorToInt32(WaveSize * Rule.Weight / TotalWeight));
			Wave.Add(FWaveGroup{ Rule.Type, Count });
			Remaining -= Count;
		}
	}
	if (Remaining > 0)
	{
		Wave.Insert(FWaveGroup{ SpawnRules[0].Type, Remaining }, 0);
	}
}

void AArenaGameMode::StartNextWave()
{
	bWaveQueued = false;
	++Round;
	WaveStartedAt = GetWorld()->GetTimeSeconds();

	for (const FWaveGroup& Group : Wave)
	{
		for (int32 Index = 0; Index < Group.Count; ++Index)
		{
			GetWorld()->SpawnActor<AArenaEnemy>(Group.Type, PickSpawnPoint(), FRotator::ZeroRotator);
		}
	}
}

FVector AArenaGameMode::PickSpawnPoint() const
{
	const double Edge = ArenaArt::ArenaHalfSize - SpawnInset;
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);

	FVector Point = FVector::ZeroVector;
	for (int32 Attempt = 0; Attempt < SpawnAttempts; ++Attempt)
	{
		const double Along = FMath::FRandRange(-Edge, Edge);
		switch (FMath::RandRange(0, 3))
		{
		case 0: Point = FVector(-Edge, 0.0, Along); break;
		case 1: Point = FVector(Edge, 0.0, Along); break;
		case 2: Point = FVector(Along, 0.0, -Edge); break;
		default: Point = FVector(Along, 0.0, Edge); break;
		}

		if (!Player || FVector::Dist2D(FVector(Point.X, Point.Z, 0.0),
			FVector(Player->GetActorLocation().X, Player->GetActorLocation().Z, 0.0)) >= MinSpawnDistanceFromPlayer)
		{
			break;
		}
	}
	return Point;
}

int32 AArenaGameMode::CountEnemies() const
{
	int32 Count = 0;
	for (TActorIterator<AArenaEnemy> It(GetWorld()); It; ++It)
	{
		++Count;
	}
	return Count;
}

void AArenaGameMode::OnPlayerDied()
{
	// The HUD shows the death screen; Continue there calls RestartArena.
	bPlayerDead = true;
}

void AArenaGameMode::OnEnemyKilled(const AArenaEnemy& Enemy, const UObject* Source)
{
	if (bPlayerDead)
	{
		return;
	}

	// Mario-style doubling: 100, 200, 400 ... for each extra kill by the same spell.
	const int32 KillsInCombo = Source ? ++KillsBySource.FindOrAdd(Source) : 1;
	const int32 Points = 100 << FMath::Min(KillsInCombo - 1, MaxComboTier);
	Score += Points;

	const double Now = GetWorld()->GetTimeSeconds();
	Popups.RemoveAll([Now](const FScorePopup& Popup) { return Now - Popup.Time > PopupSeconds; });
	Popups.Add({ Enemy.GetActorLocation(), FMath::Min(KillsInCombo - 1, MaxComboTier), Points, Now });
}

void AArenaGameMode::AwardWaveBonus()
{
	const double Seconds = GetWorld()->GetTimeSeconds() - WaveStartedAt;
	Score += Round * WaveBonusPerRound + FMath::Max(0, FMath::RoundToInt32(WaveParSeconds - Seconds)) * PointsPerSecondUnderPar;
	KillsBySource.Reset();
}

const FEnemySkin* AArenaGameMode::GetEnemySkin(TSubclassOf<AArenaEnemy> Type) const
{
	return Type ? Theme->FindSkin(Type) : nullptr;
}

void AArenaGameMode::RestartArena()
{
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}

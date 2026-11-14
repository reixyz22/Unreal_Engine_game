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
	constexpr float RestartDelay = 3.f;
	constexpr int32 SpawnAttempts = 8;
}

AArenaGameMode::AArenaGameMode()
{
	HUDClass = AArenaHUD::StaticClass();
}

void AArenaGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	const TSubclassOf<UArenaTheme> Picked = Themes.IsEmpty() ? nullptr : Themes[FMath::RandRange(0, Themes.Num() - 1)];
	Theme = Picked ? Picked->GetDefaultObject<UArenaTheme>() : GetDefault<UArenaTheme>();
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
		It->GetRenderComponent()->SetSpriteColor(Theme->FloorTint);
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
	bPlayerDead = true;
	GetWorldTimerManager().SetTimer(RestartTimer, this, &AArenaGameMode::RestartArena, RestartDelay, false);
}

void AArenaGameMode::RestartArena()
{
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}

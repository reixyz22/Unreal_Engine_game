#include "ArenaGameMode.h"

#include "ArenaEnemy.h"
#include "ArenaHUD.h"
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

void AArenaGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (Music)
	{
		UGameplayStatics::SpawnSound2D(this, Music);
	}

	GetWorldTimerManager().SetTimer(CheckTimer, this, &AArenaGameMode::CheckWave, CheckInterval, true, 1.f);
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
	if (EnemyTypes.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("BP_ArenaGameMode has no Enemy Types; waves will be empty."));
		return;
	}

	const int32 WaveSize = FMath::Min(FirstWaveSize + ExtraEnemiesPerRound * Round, MaxWaveSize);
	for (int32 Index = 0; Index < WaveSize; ++Index)
	{
		const TSubclassOf<AArenaEnemy> Type = EnemyTypes[FMath::RandRange(0, EnemyTypes.Num() - 1)];
		FWaveGroup* Group = Wave.FindByPredicate([Type](const FWaveGroup& G) { return G.Type == Type; });
		if (!Group)
		{
			Group = &Wave.Add_GetRef(FWaveGroup{ Type });
		}
		++Group->Count;
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
	const double Edge = ArenaHalfSize - SpawnInset;
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

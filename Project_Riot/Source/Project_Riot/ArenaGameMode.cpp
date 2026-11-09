#include "ArenaGameMode.h"

#include "ArenaEnemy.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

namespace
{
	constexpr float CheckInterval = 0.5f;
	constexpr float RestartDelay = 3.f;
	constexpr int32 SpawnAttempts = 8;
	constexpr int32 RoundMessageKey = 1;
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

	bWaveQueued = true;
	const float Delay = Round == 0 ? 0.1f : SecondsBetweenWaves;
	GetWorldTimerManager().SetTimer(WaveTimer, this, &AArenaGameMode::StartNextWave, Delay, false);
}

void AArenaGameMode::StartNextWave()
{
	bWaveQueued = false;
	++Round;

	if (!PyroAnim || !BeadSprite)
	{
		UE_LOG(LogTemp, Warning, TEXT("BP_ArenaGameMode is missing Pyro Anim or Bead Sprite; Pyros and beads will be invisible."));
	}

	const int32 WaveSize = FMath::Min(FirstWaveSize + ExtraEnemiesPerRound * (Round - 1), MaxWaveSize);
	for (int32 Index = 0; Index < WaveSize; ++Index)
	{
		AArenaEnemy* Enemy = GetWorld()->SpawnActor<AArenaEnemy>(PickSpawnPoint(), FRotator::ZeroRotator);
		if (Enemy)
		{
			Enemy->Init(PyroAnim, BeadSprite, PyroTint);
		}
	}

	ShowRoundText();
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

void AArenaGameMode::ShowRoundText() const
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(RoundMessageKey, 1000.f, FColor::White,
			FString::Printf(TEXT("Round %d"), Round));
	}
}

void AArenaGameMode::OnPlayerDied()
{
	bPlayerDead = true;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(RoundMessageKey, RestartDelay, FColor::Red,
			FString::Printf(TEXT("Nikki fell. Rounds survived: %d"), FMath::Max(0, Round - 1)));
	}

	GetWorldTimerManager().SetTimer(RestartTimer, this, &AArenaGameMode::RestartArena, RestartDelay, false);
}

void AArenaGameMode::RestartArena()
{
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}

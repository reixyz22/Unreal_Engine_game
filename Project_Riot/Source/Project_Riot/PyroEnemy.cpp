#include "PyroEnemy.h"

#include "ArenaProjectile.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"

namespace
{
	constexpr float BeadDepth = 14.f;
	constexpr float TooCloseFraction = 0.75f;
}

void APyroEnemy::BeginPlay()
{
	Super::BeginPlay();

	const float FirstShotDelay = FMath::FRandRange(0.5f, FireInterval);
	GetWorldTimerManager().SetTimer(FireTimer, this, &APyroEnemy::Fire, FireInterval, true, FirstShotDelay);
}

void APyroEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const APawn* Player = GetPlayer();
	if (!Player)
	{
		return;
	}

	FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
	ToPlayer.Y = 0.f;
	const double Distance = ToPlayer.Size();
	const FVector TowardPlayer = ToPlayer.GetSafeNormal();

	FVector Desired = FVector::ZeroVector;
	if (Distance > PreferredRange)
	{
		Desired = TowardPlayer;
	}
	else if (Distance < PreferredRange * TooCloseFraction)
	{
		Desired = -TowardPlayer;
	}
	Desired += ComputeSeparation() * SpreadStrength;

	AddActorWorldOffset(Desired.GetClampedToMaxSize(1.0) * MoveSpeed * DeltaTime);
	FaceToward(TowardPlayer.X);
}

FVector APyroEnemy::ComputeSeparation() const
{
	FVector Push = FVector::ZeroVector;
	for (TActorIterator<AArenaEnemy> It(GetWorld()); It; ++It)
	{
		if (*It == this)
		{
			continue;
		}

		FVector Away = GetActorLocation() - It->GetActorLocation();
		Away.Y = 0.f;
		const double Distance = Away.Size();
		if (Distance > KINDA_SMALL_NUMBER && Distance < SpreadDistance)
		{
			Push += Away / Distance * (1.0 - Distance / SpreadDistance);
		}
	}
	return Push;
}

void APyroEnemy::Fire()
{
	const APawn* Player = GetPlayer();
	if (!Player)
	{
		return;
	}

	FVector Direction = Player->GetActorLocation() - GetActorLocation();
	Direction.Y = 0.f;

	FVector SpawnAt = GetActorLocation();
	SpawnAt.Y = BeadDepth;

	if (AArenaProjectile* Bead = GetWorld()->SpawnActor<AArenaProjectile>(SpawnAt, FRotator::ZeroRotator))
	{
		Bead->Launch(BeadSprite, FLinearColor::White, Direction.GetSafeNormal() * BeadSpeed, BeadDamage);
	}
}

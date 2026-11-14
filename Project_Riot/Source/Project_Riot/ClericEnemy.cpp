#include "ClericEnemy.h"

#include "ArenaBeam.h"
#include "ArenaPickup.h"
#include "ArenaPlayerPawn.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace
{
	constexpr float PickupDepth = 6.f;
}

AClericEnemy::AClericEnemy()
{
	MaxHealth = 45.f;
	MoveSpeed = 90.f;
}

void AClericEnemy::BeginPlay()
{
	Super::BeginPlay();

	const float FirstCastDelay = FMath::FRandRange(1.5f, CastInterval);
	GetWorldTimerManager().SetTimer(CastTimer, this, &AClericEnemy::CastBeam, CastInterval, true, FirstCastDelay);
}

void AClericEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const APawn* Player = GetPlayer();
	if (!Player)
	{
		return;
	}

	FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
	ToPlayer.Y = 0.0;
	FaceToward(ToPlayer.X);

	// Stand still while casting; otherwise drift to casting range.
	if (GetWorld()->GetTimeSeconds() < CastingUntil)
	{
		return;
	}
	const double Distance = ToPlayer.Size();
	const double Step = FMath::Clamp(Distance - PreferredRange, -1.0, 1.0);
	AddActorWorldOffset(ToPlayer.GetSafeNormal() * Step * MoveSpeed * DeltaTime);
}

void AClericEnemy::CastBeam()
{
	const APawn* Player = GetPlayer();
	if (!Player)
	{
		return;
	}

	AArenaBeam::FSettings Settings;
	Settings.WarningSeconds = WarningSeconds;
	Settings.ActiveSeconds = BeamSeconds;
	Settings.Width = BeamWidth;
	Settings.Damage = BeamDamage;
	Settings.StunSeconds = StunSeconds;

	if (AArenaBeam* Beam = GetWorld()->SpawnActor<AArenaBeam>(GetActorLocation(), FRotator::ZeroRotator))
	{
		Beam->Fire(BeamSprite, GetActorLocation(), Player->GetActorLocation() - GetActorLocation(), Settings);
		CastingUntil = GetWorld()->GetTimeSeconds() + WarningSeconds + BeamSeconds;
	}
}

void AClericEnemy::Die()
{
	const AArenaPlayerPawn* Player = Cast<AArenaPlayerPawn>(GetPlayer());
	const bool bManaFull = Player && Player->IsManaFull();

	FVector DropAt = GetActorLocation();
	DropAt.Y = PickupDepth;
	if (AArenaPickup* Shard = GetWorld()->SpawnActor<AArenaPickup>(DropAt, FRotator::ZeroRotator))
	{
		Shard->Init(bManaFull ? EPickupKind::Health : EPickupKind::Mana, bManaFull ? HealthShardSprite : ManaShardSprite);
	}

	Super::Die();
}

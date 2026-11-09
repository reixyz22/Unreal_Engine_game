#include "ArenaEnemy.h"

#include "ArenaArt.h"
#include "ArenaProjectile.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PaperFlipbookComponent.h"
#include "TimerManager.h"

namespace
{
	constexpr float EnemyDepth = 8.f;
	constexpr float BeadDepth = 14.f;
	constexpr float TooCloseFraction = 0.75f;
}

AArenaEnemy::AArenaEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(26.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(Collision);

	Body = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Body"));
	Body->SetupAttachment(Collision);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetRelativeScale3D(FVector(ArenaArt::PixelScale));

	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
}

void AArenaEnemy::Init(UPaperFlipbook* BodyAnim, UPaperSprite* InBeadSprite, const FLinearColor& InTint)
{
	Body->SetFlipbook(BodyAnim);
	BeadSprite = InBeadSprite;
	Tint = InTint;
	Body->SetSpriteColor(Tint);
}

void AArenaEnemy::BeginPlay()
{
	Health->SetMaxHealth(MaxHealth);
	Super::BeginPlay();

	FVector Start = GetActorLocation();
	Start.Y = EnemyDepth;
	SetActorLocation(Start);

	Health->OnDepleted.AddUObject(this, &AArenaEnemy::Die);

	const float FirstShotDelay = FMath::FRandRange(0.5f, FireInterval);
	GetWorldTimerManager().SetTimer(FireTimer, this, &AArenaEnemy::Fire, FireInterval, true, FirstShotDelay);
}

float AArenaEnemy::GetRadius() const
{
	return Collision->GetScaledSphereRadius();
}

bool AArenaEnemy::ReceiveDamage(float Amount)
{
	return Health->ApplyDamage(Amount);
}

void AArenaEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
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

	const float Facing = TowardPlayer.X < 0.0 ? -1.f : 1.f;
	Body->SetRelativeScale3D(FVector(Facing * ArenaArt::PixelScale, ArenaArt::PixelScale, ArenaArt::PixelScale));

	const bool bFlash = Health->IsInvulnerable();
	Body->SetSpriteColor(bFlash ? FLinearColor(1.f, 0.35f, 0.35f) : Tint);
}

FVector AArenaEnemy::ComputeSeparation() const
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

void AArenaEnemy::Fire()
{
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		return;
	}

	FVector Direction = Player->GetActorLocation() - GetActorLocation();
	Direction.Y = 0.f;

	FVector SpawnAt = GetActorLocation();
	SpawnAt.Y = BeadDepth;

	AArenaProjectile* Bead = GetWorld()->SpawnActor<AArenaProjectile>(SpawnAt, FRotator::ZeroRotator);
	if (Bead)
	{
		Bead->Launch(BeadSprite, FLinearColor::White, Direction.GetSafeNormal() * BeadSpeed, BeadDamage);
	}
}

void AArenaEnemy::Die()
{
	Destroy();
}

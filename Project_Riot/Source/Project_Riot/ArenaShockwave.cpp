#include "ArenaShockwave.h"

#include "ArenaEnemy.h"
#include "ArenaProjectile.h"
#include "EngineUtils.h"
#include "PaperSpriteComponent.h"

namespace
{
	// Shock_Burst.png is 96 px across with its ring near the edge.
	constexpr float BurstPixels = 96.f;
	constexpr float SpinDegreesPerSecond = 90.f;
	constexpr float ShockDepth = 18.f;
}

AArenaShockwave::AArenaShockwave()
{
	PrimaryActorTick.bCanEverTick = true;

	Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Sprite"));
	Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(Sprite);
}

void AArenaShockwave::Launch(UPaperSprite* BurstSprite, float InDamage, float InMaxRadius, float InSeconds)
{
	Sprite->SetSprite(BurstSprite);
	Damage = InDamage;
	MaxRadius = InMaxRadius;
	Seconds = InSeconds;
	StartedAt = GetWorld()->GetTimeSeconds();

	FVector Location = GetActorLocation();
	Location.Y = ShockDepth;
	SetActorLocation(Location);
	SetLifeSpan(Seconds);
}

void AArenaShockwave::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const float Alpha = FMath::Clamp(float(GetWorld()->GetTimeSeconds() - StartedAt) / Seconds, 0.f, 1.f);
	const float Radius = MaxRadius * FMath::InterpEaseOut(0.05f, 1.f, Alpha, 2.f);
	Sprite->SetRelativeScale3D(FVector(Radius * 2.f / BurstPixels));
	AddActorLocalRotation(FRotator(SpinDegreesPerSecond * DeltaTime, 0.f, 0.f));

	const FVector Center = GetActorLocation();
	auto Inside = [&Center, Radius](const AActor* Actor)
	{
		return FVector::Dist2D(FVector(Actor->GetActorLocation().X, Actor->GetActorLocation().Z, 0.0),
			FVector(Center.X, Center.Z, 0.0)) <= Radius;
	};

	for (TActorIterator<AArenaEnemy> It(GetWorld()); It; ++It)
	{
		if (Inside(*It) && !HitAlready.Contains(*It))
		{
			HitAlready.Add(*It);
			It->ReceiveDamage(Damage);
		}
	}
	for (TActorIterator<AArenaProjectile> It(GetWorld()); It; ++It)
	{
		if (Inside(*It))
		{
			It->Destroy();
		}
	}
}

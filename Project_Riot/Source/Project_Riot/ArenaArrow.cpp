#include "ArenaArrow.h"

#include "ArenaArt.h"
#include "ArenaEnemy.h"
#include "EngineUtils.h"
#include "PaperSpriteComponent.h"

namespace
{
	constexpr float ArrowDepth = 16.f;
	constexpr float ArrowHitRadius = 12.f;
	constexpr float SafetyLifeSeconds = 10.f;
	// The arrow art points up, so it is turned a quarter turn to face along +X first.
	constexpr float ArtPointsUpDegrees = 90.f;
}

AArenaArrow::AArenaArrow()
{
	PrimaryActorTick.bCanEverTick = true;

	Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Sprite"));
	Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Sprite->SetRelativeScale3D(FVector(ArenaArt::PixelScale * 0.75f));
	SetRootComponent(Sprite);
}

void AArenaArrow::Fire(UPaperSprite* ArrowSprite, const FVector& Direction, float InSpeed, float InDamage, int32 InBounces)
{
	Sprite->SetSprite(ArrowSprite);
	Velocity = FVector(Direction.X, 0.0, Direction.Z).GetSafeNormal() * InSpeed;
	Damage = InDamage;
	BouncesLeft = InBounces;

	FVector Location = GetActorLocation();
	Location.Y = ArrowDepth;
	SetActorLocation(Location);
	SetLifeSpan(SafetyLifeSeconds);
	Aim();
}

void AArenaArrow::Aim()
{
	const float Angle = FMath::RadiansToDegrees(FMath::Atan2(Velocity.Z, Velocity.X));
	SetActorRotation(FRotator(Angle - ArtPointsUpDegrees, 0.f, 0.f));
}

void AArenaArrow::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector Location = GetActorLocation() + Velocity * DeltaTime;

	// Walls: bounce like a mirror if it can, otherwise the arrow is spent.
	const double Limit = ArenaArt::ArenaHalfSize - ArenaArt::WallThickness;
	const bool bHitSide = FMath::Abs(Location.X) > Limit;
	const bool bHitTopOrBottom = FMath::Abs(Location.Z) > Limit;
	if (bHitSide || bHitTopOrBottom)
	{
		if (BouncesLeft <= 0)
		{
			Destroy();
			return;
		}
		--BouncesLeft;
		Velocity.X *= bHitSide ? -1.0 : 1.0;
		Velocity.Z *= bHitTopOrBottom ? -1.0 : 1.0;
		Location.X = FMath::Clamp(Location.X, -Limit, Limit);
		Location.Z = FMath::Clamp(Location.Z, -Limit, Limit);
		JustBouncedOff = nullptr;
		Aim();
	}
	SetActorLocation(Location);

	AArenaEnemy* Enemy = FindEnemyHit();
	if (!Enemy)
	{
		return;
	}
	// The arrow itself is the damage source, so every kill from one ricochet chains into a combo.
	Enemy->ReceiveDamage(Damage, EDamageType::Physical, this);
	if (BouncesLeft <= 0)
	{
		Destroy();
		return;
	}

	// Ricochet: reflect off the line from the enemy's centre to the arrow.
	--BouncesLeft;
	FVector Normal = Location - Enemy->GetActorLocation();
	Normal.Y = 0.0;
	Normal = Normal.GetSafeNormal();
	Velocity = Velocity - 2.0 * FVector::DotProduct(Velocity, Normal) * Normal;
	JustBouncedOff = Enemy;
	Aim();
}

AArenaEnemy* AArenaArrow::FindEnemyHit() const
{
	for (TActorIterator<AArenaEnemy> It(GetWorld()); It; ++It)
	{
		FVector Offset = It->GetActorLocation() - GetActorLocation();
		Offset.Y = 0.0;
		if (*It != JustBouncedOff.Get() && Offset.Size() < It->GetRadius() + ArrowHitRadius)
		{
			return *It;
		}
	}
	return nullptr;
}

#include "ArenaChainBall.h"

#include "ArenaArt.h"
#include "ArenaEnemy.h"
#include "ArenaPlayerPawn.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "PaperSpriteComponent.h"

namespace
{
	constexpr float ArriveDistance = 20.f;
	constexpr float BallHitRadius = 22.f;
	const FColor ChainColor(120, 110, 120);
}

AArenaChainBall::AArenaChainBall()
{
	PrimaryActorTick.bCanEverTick = true;

	Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Sprite"));
	Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Sprite->SetRelativeScale3D(FVector(ArenaArt::PixelScale * ArenaArt::FlailScale));
	SetRootComponent(Sprite);
}

void AArenaChainBall::Throw(AArenaPlayerPawn* InThrower, AArenaEnemy* InTarget, UPaperSprite* InSprite, const FLinearColor& Tint, float InSpeed, float InDamage)
{
	Thrower = InThrower;
	Target = InTarget;
	Sprite->SetSprite(InSprite);
	Sprite->SetSpriteColor(Tint);
	Speed = InSpeed;
	Damage = InDamage;
}

bool AArenaChainBall::MoveToward(const FVector& Goal, float DeltaTime)
{
	FVector ToGoal = Goal - GetActorLocation();
	ToGoal.Y = 0.f;

	const double Step = Speed * DeltaTime;
	if (ToGoal.Size() <= FMath::Max(Step, ArriveDistance))
	{
		return true;
	}

	AddActorWorldOffset(ToGoal.GetSafeNormal() * Step);
	return false;
}

void AArenaChainBall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AArenaPlayerPawn* ThrowerPawn = Thrower.Get();
	if (!ThrowerPawn)
	{
		Destroy();
		return;
	}

	const FVector Hand = ThrowerPawn->GetHandLocation();
	DrawDebugLine(GetWorld(), Hand, GetActorLocation(), ChainColor, false, -1.f, 0, 3.f);

	if (State == EChainState::Outbound)
	{
		AArenaEnemy* Enemy = Target.Get();
		if (!Enemy)
		{
			State = EChainState::Returning;
			return;
		}

		const bool bArrived = MoveToward(Enemy->GetActorLocation(), DeltaTime);
		HitEnemiesInPath();
		if (bArrived)
		{
			HitEnemy(Enemy);
			State = EChainState::Returning;
		}
		return;
	}

	const bool bHome = MoveToward(Hand, DeltaTime);
	HitEnemiesInPath();
	if (bHome)
	{
		ThrowerPawn->OnChainReturned();
		Destroy();
	}
}

void AArenaChainBall::HitEnemiesInPath()
{
	for (TActorIterator<AArenaEnemy> It(GetWorld()); It; ++It)
	{
		FVector Offset = It->GetActorLocation() - GetActorLocation();
		Offset.Y = 0.f;
		if (Offset.Size() < It->GetRadius() + BallHitRadius)
		{
			HitEnemy(*It);
		}
	}
}

void AArenaChainBall::HitEnemy(AArenaEnemy* Enemy)
{
	if (HitThisThrow.Contains(Enemy))
	{
		return;
	}

	Enemy->ReceiveDamage(Damage);
	HitThisThrow.Add(Enemy);
}

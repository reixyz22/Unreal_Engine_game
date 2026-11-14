#include "ArenaChainBall.h"

#include "ArenaArt.h"
#include "ArenaEnemy.h"
#include "ArenaPlayerPawn.h"
#include "ArenaSession.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "PaperSpriteComponent.h"

namespace
{
	constexpr float ArriveDistance = 20.f;
	constexpr float BallHitRadius = 22.f;
	// Chain_Link.png is 8 x 3 pixels; it is stretched from her hand to the ball.
	constexpr float LinkPixelsLong = 8.f;
	constexpr float LinkPixelsWide = 3.f;
	constexpr float ChainThickness = 9.f;
	constexpr float ChainDepthOffset = -1.f;
}

AArenaChainBall::AArenaChainBall()
{
	PrimaryActorTick.bCanEverTick = true;

	Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Sprite"));
	Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Sprite->SetRelativeScale3D(FVector(ArenaArt::PixelScale * ArenaArt::FlailScale));
	SetRootComponent(Sprite);

	Chain = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Chain"));
	Chain->SetupAttachment(Sprite);
	Chain->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Chain->SetUsingAbsoluteLocation(true);
	Chain->SetUsingAbsoluteRotation(true);
	Chain->SetUsingAbsoluteScale(true);
}

void AArenaChainBall::Throw(AArenaPlayerPawn* InThrower, AArenaEnemy* InTarget, const FChainLook& Look, float InSpeed, float InDamage)
{
	Thrower = InThrower;
	Target = InTarget;
	HitSound = Look.HitSound;
	Sprite->SetSprite(Look.Ball);
	Chain->SetSprite(Look.Link);
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
	DrawChain(Hand);

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

void AArenaChainBall::DrawChain(const FVector& Hand)
{
	FVector Span = GetActorLocation() - Hand;
	Span.Y = 0.0;
	FVector Middle = Hand + Span * 0.5;
	Middle.Y = GetActorLocation().Y + ChainDepthOffset;

	Chain->SetWorldLocationAndRotation(Middle, FRotator(FMath::RadiansToDegrees(FMath::Atan2(Span.Z, Span.X)), 0.f, 0.f));
	Chain->SetWorldScale3D(FVector(FMath::Max(Span.Size(), 1.0) / LinkPixelsLong, 1.0, ChainThickness / LinkPixelsWide));
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
	UArenaSession::PlaySfx(this, HitSound);
}

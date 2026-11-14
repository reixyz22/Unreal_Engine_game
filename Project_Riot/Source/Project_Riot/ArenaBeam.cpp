#include "ArenaBeam.h"

#include "ArenaPlayerPawn.h"
#include "ArenaSession.h"
#include "Kismet/GameplayStatics.h"
#include "PaperSpriteComponent.h"

namespace
{
	constexpr double BeamLength = 6000.0;
	constexpr float SpritePixels = 8.f;
	constexpr float WarningThickness = 6.f;
	constexpr float BeamDepth = 12.f;
	const FLinearColor WarningTint(1.f, 0.75f, 0.3f);
	constexpr double WarningGrowSeconds = 0.3;
	constexpr double ActiveGrowSeconds = 0.08;
}

AArenaBeam::AArenaBeam()
{
	PrimaryActorTick.bCanEverTick = true;

	Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Sprite"));
	Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(Sprite);
}

void AArenaBeam::Fire(UPaperSprite* BeamSprite, const FVector& InStart, const FVector& InDirection, const FSettings& InSettings)
{
	Start = InStart;
	Start.Y = BeamDepth;
	Direction = InDirection.GetSafeNormal();
	Direction.Y = 0.0;
	Settings = InSettings;
	FireSound = Settings.FireSound;
	FiredAt = GetWorld()->GetTimeSeconds();
	ActiveAt = FiredAt + Settings.WarningSeconds;
	SetLifeSpan(Settings.WarningSeconds + Settings.ActiveSeconds);

	SetActorRotation(FRotator(FMath::RadiansToDegrees(FMath::Atan2(Direction.Z, Direction.X)), 0.f, 0.f));
	Sprite->SetSprite(BeamSprite);
	Draw(0.0, WarningThickness, WarningTint);

	UArenaSession::PlaySfx(this, Settings.ChargeSound);
}

void AArenaBeam::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const double Now = GetWorld()->GetTimeSeconds();
	if (!bActive && Now >= ActiveAt)
	{
		bActive = true;
		UArenaSession::PlaySfx(this, FireSound);
	}

	// Each phase sweeps out from the Cleric: slow enough to read the direction,
	// fast enough that standing far away doesn't buy real time.
	const double PhaseStart = bActive ? ActiveAt : FiredAt;
	const double GrowSeconds = bActive ? ActiveGrowSeconds : WarningGrowSeconds;
	Length = BeamLength * FMath::Clamp((Now - PhaseStart) / GrowSeconds, 0.0, 1.0);
	Draw(Length, bActive ? Settings.Width : WarningThickness, bActive ? FLinearColor::White : WarningTint);

	AArenaPlayerPawn* Player = Cast<AArenaPlayerPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (bActive && !bHitPlayer && Player && IsTouchingPlayer(Player->GetRadius(), Player->GetActorLocation()))
	{
		bHitPlayer = Player->ReceiveStunHit(Settings.Damage, Settings.StunSeconds);
	}
}

void AArenaBeam::Draw(double InLength, float Thickness, const FLinearColor& Tint)
{
	// The sprite is centred, so sit halfway along the drawn part.
	SetActorLocation(Start + Direction * (InLength * 0.5));
	Sprite->SetRelativeScale3D(FVector(FMath::Max(InLength, 1.0) / SpritePixels, 1.f, Thickness / SpritePixels));
	Sprite->SetSpriteColor(Tint);
}

bool AArenaBeam::IsTouchingPlayer(float PlayerRadius, const FVector& PlayerLocation) const
{
	FVector ToPlayer = PlayerLocation - Start;
	ToPlayer.Y = 0.0;
	const double Along = FVector::DotProduct(ToPlayer, Direction);
	const double Across = (ToPlayer - Direction * Along).Size();
	return Along >= 0.0 && Along <= Length && Across <= Settings.Width * 0.5 + PlayerRadius;
}

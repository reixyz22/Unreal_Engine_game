#include "ArenaBeam.h"

#include "ArenaPlayerPawn.h"
#include "Kismet/GameplayStatics.h"
#include "PaperSpriteComponent.h"

namespace
{
	constexpr double BeamLength = 6000.0;
	constexpr float SpritePixels = 8.f;
	constexpr float WarningThickness = 6.f;
	constexpr float BeamDepth = 12.f;
	const FLinearColor WarningTint(1.f, 0.75f, 0.3f);
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
	Direction = InDirection.GetSafeNormal();
	Settings = InSettings;
	ActiveAt = GetWorld()->GetTimeSeconds() + Settings.WarningSeconds;
	SetLifeSpan(Settings.WarningSeconds + Settings.ActiveSeconds);

	// The sprite is centred, so sit halfway along the beam and rotate it within the X/Z plane.
	FVector Middle = Start + Direction * (BeamLength * 0.5);
	Middle.Y = BeamDepth;
	SetActorLocation(Middle);
	SetActorRotation(FRotator(FMath::RadiansToDegrees(FMath::Atan2(Direction.Z, Direction.X)), 0.f, 0.f));

	Sprite->SetSprite(BeamSprite);
	SetThickness(WarningThickness, WarningTint);
}

void AArenaBeam::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bActive && GetWorld()->GetTimeSeconds() >= ActiveAt)
	{
		bActive = true;
		SetThickness(Settings.Width, FLinearColor::White);
	}

	AArenaPlayerPawn* Player = Cast<AArenaPlayerPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (bActive && !bHitPlayer && Player && IsTouchingPlayer(Player->GetRadius(), Player->GetActorLocation()))
	{
		bHitPlayer = Player->ReceiveStunHit(Settings.Damage, Settings.StunSeconds);
	}
}

void AArenaBeam::SetThickness(float Thickness, const FLinearColor& Tint)
{
	Sprite->SetRelativeScale3D(FVector(BeamLength / SpritePixels, 1.f, Thickness / SpritePixels));
	Sprite->SetSpriteColor(Tint);
}

bool AArenaBeam::IsTouchingPlayer(float PlayerRadius, const FVector& PlayerLocation) const
{
	FVector ToPlayer = PlayerLocation - Start;
	ToPlayer.Y = 0.0;
	const double Along = FVector::DotProduct(ToPlayer, Direction);
	const double Across = (ToPlayer - Direction * Along).Size();
	return Along >= 0.0 && Across <= Settings.Width * 0.5 + PlayerRadius;
}

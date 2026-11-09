#include "ArenaProjectile.h"

#include "ArenaArt.h"
#include "ArenaPlayerPawn.h"
#include "Kismet/GameplayStatics.h"
#include "PaperSpriteComponent.h"

namespace
{
	constexpr float BeadLifeSeconds = 6.f;
}

AArenaProjectile::AArenaProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Sprite"));
	Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Sprite->SetRelativeScale3D(FVector(ArenaArt::PixelScale));
	SetRootComponent(Sprite);
}

void AArenaProjectile::Launch(UPaperSprite* InSprite, const FLinearColor& Tint, const FVector& InVelocity, float InDamage)
{
	Sprite->SetSprite(InSprite);
	Sprite->SetSpriteColor(Tint);
	Velocity = InVelocity;
	Damage = InDamage;
	SetLifeSpan(BeadLifeSeconds);
}

void AArenaProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AddActorWorldOffset(Velocity * DeltaTime);

	AArenaPlayerPawn* Player = Cast<AArenaPlayerPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Player)
	{
		return;
	}

	FVector Offset = Player->GetActorLocation() - GetActorLocation();
	Offset.Y = 0.f;
	if (Offset.Size() < Player->GetRadius() + HitRadius)
	{
		Player->ReceiveDamage(Damage);
		Destroy();
	}
}

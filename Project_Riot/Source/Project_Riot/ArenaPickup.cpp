#include "ArenaPickup.h"

#include "ArenaArt.h"
#include "ArenaPlayerPawn.h"
#include "Kismet/GameplayStatics.h"
#include "PaperSpriteComponent.h"

namespace
{
	constexpr float LifetimeSeconds = 9.f;
	constexpr float BlinkFromSeconds = 3.f;
	constexpr float CollectRadius = 50.f;
	constexpr float HealAmount = 25.f;
	constexpr float BobHeight = 6.f;
}

AArenaPickup::AArenaPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Sprite"));
	Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Sprite->SetRelativeScale3D(FVector(ArenaArt::PixelScale));
	SetRootComponent(Sprite);
}

void AArenaPickup::Init(EPickupKind InKind, UPaperSprite* ShardSprite)
{
	Kind = InKind;
	Home = GetActorLocation();
	Sprite->SetSprite(ShardSprite);
	SetLifeSpan(LifetimeSeconds);
}

void AArenaPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const double Now = GetWorld()->GetTimeSeconds();
	SetActorLocation(Home + FVector(0.0, 0.0, FMath::Sin(Now * 4.0) * BobHeight));
	Sprite->SetVisibility(GetLifeSpan() > BlinkFromSeconds || FMath::Fmod(Now, 0.2) < 0.12);

	AArenaPlayerPawn* Player = Cast<AArenaPlayerPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Player || FVector::Dist2D(FVector(Home.X, Home.Z, 0.0), FVector(Player->GetActorLocation().X, Player->GetActorLocation().Z, 0.0)) > CollectRadius + Player->GetRadius())
	{
		return;
	}

	if (Kind == EPickupKind::Mana)
	{
		Player->AddMana(1);
	}
	else
	{
		Player->Heal(HealAmount);
	}
	Destroy();
}

#include "ArenaEnemy.h"

#include "ArenaArt.h"
#include "Components/SphereComponent.h"
#include "HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"

namespace
{
	constexpr float EnemyDepth = 8.f;
	const FLinearColor HitFlash(1.f, 0.35f, 0.35f);
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

void AArenaEnemy::BeginPlay()
{
	Health->SetMaxHealth(MaxHealth);
	Super::BeginPlay();

	FVector Start = GetActorLocation();
	Start.Y = EnemyDepth;
	SetActorLocation(Start);

	Body->SetFlipbook(BodyAnim);
	Body->SetSpriteColor(Tint);
	BodyMaterial = Body->CreateDynamicMaterialInstance(0);

	Health->OnDepleted.AddUObject(this, &AArenaEnemy::Die);
}

void AArenaEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Body->SetSpriteColor(Health->IsInvulnerable() ? HitFlash : Tint);
}

bool AArenaEnemy::ReceiveDamage(float Amount)
{
	return Health->ApplyDamage(Amount);
}

float AArenaEnemy::GetRadius() const
{
	return Collision->GetScaledSphereRadius();
}

void AArenaEnemy::SetHovered(bool bHovered)
{
	if (BodyMaterial)
	{
		BodyMaterial->SetVectorParameterValue(ArenaArt::OutlineParam, bHovered ? ArenaArt::HoverOutline : FLinearColor::Transparent);
	}
}

UPaperSprite* AArenaEnemy::GetIcon() const
{
	return BodyAnim ? BodyAnim->GetSpriteAtFrame(0) : nullptr;
}

APawn* AArenaEnemy::GetPlayer() const
{
	return UGameplayStatics::GetPlayerPawn(this, 0);
}

void AArenaEnemy::FaceToward(double DirectionX)
{
	const float Facing = DirectionX < 0.0 ? -1.f : 1.f;
	Body->SetRelativeScale3D(FVector(Facing * ArenaArt::PixelScale, ArenaArt::PixelScale, ArenaArt::PixelScale));
}

void AArenaEnemy::MoveWithinArena(const FVector& Offset)
{
	const double Limit = ArenaArt::ArenaHalfSize - ArenaArt::WallThickness - GetRadius();
	FVector Location = GetActorLocation() + Offset;
	Location.X = FMath::Clamp(Location.X, -Limit, Limit);
	Location.Z = FMath::Clamp(Location.Z, -Limit, Limit);
	SetActorLocation(Location);
}

void AArenaEnemy::Die()
{
	Destroy();
}

#include "StoryProp.h"

#include "ArenaArt.h"
#include "Kismet/GameplayStatics.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"

namespace
{
	constexpr float BehindHeroDepth = 8.f;
	constexpr float InFrontOfHeroDepth = 12.f;
	constexpr float OverlayDepth = 15.f;
	constexpr float HeroFeetOffset = 40.f;
	constexpr double PunchSeconds = 0.25;
}

AStoryProp::AStoryProp()
{
	PrimaryActorTick.bCanEverTick = true;

	Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Sprite"));
	Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Sprite->SetRelativeScale3D(FVector(ArenaArt::PixelScale));
	SetRootComponent(Sprite);
}

void AStoryProp::SetArt(UPaperSprite* InSprite)
{
	Sprite->SetSprite(InSprite);
	if (InSprite)
	{
		const FVector Extent = InSprite->GetRenderBounds().BoxExtent * ArenaArt::PixelScale;
		HalfSize = FVector2D(Extent.X, Extent.Z);
	}
}

void AStoryProp::Punch()
{
	PunchUntil = GetWorld()->GetTimeSeconds() + PunchSeconds;
}

void AStoryProp::Wobble(float Seconds)
{
	WobbleUntil = GetWorld()->GetTimeSeconds() + Seconds;
}

void AStoryProp::PlayFrames(const TArray<UPaperSprite*>& InFrames, float SecondsPerFrame)
{
	Frames.Reset();
	for (UPaperSprite* Frame : InFrames)
	{
		Frames.Add(Frame);
	}
	FramesStartedAt = GetWorld()->GetTimeSeconds();
	FrameSeconds = SecondsPerFrame;
}

void AStoryProp::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	const double Now = GetWorld()->GetTimeSeconds();

	if (Frames.Num() > 0)
	{
		const int32 Index = FMath::Min(Frames.Num() - 1, FMath::FloorToInt32((Now - FramesStartedAt) / FrameSeconds));
		if (Sprite->GetSprite() != Frames[Index])
		{
			Sprite->SetSprite(Frames[Index]);
		}
		if (Index == Frames.Num() - 1)
		{
			Frames.Reset();
		}
	}

	// Depth sort: if the hero's feet are above this prop's base on screen, she is behind it.
	FVector Location = GetActorLocation();
	const APawn* Hero = UGameplayStatics::GetPlayerPawn(this, 0);
	const bool bHeroBehind = Hero && Hero->GetActorLocation().Z - HeroFeetOffset > Location.Z - HalfSize.Y;
	Location.Y = bOverlay ? OverlayDepth : bHeroBehind ? InFrontOfHeroDepth : BehindHeroDepth;
	SetActorLocation(Location);

	const float PunchLeft = FMath::Clamp(float((PunchUntil - Now) / PunchSeconds), 0.f, 1.f);
	const float Squash = FMath::Sin(PunchLeft * UE_PI) * 0.3f;
	Sprite->SetRelativeScale3D(FVector(1.f + Squash, 1.f, 1.f - Squash) * ArenaArt::PixelScale);
	Sprite->SetRelativeRotation(FRotator(Now < WobbleUntil ? FMath::Sin(Now * 60.0) * 10.0 : 0.0, 0.0, 0.0));
}

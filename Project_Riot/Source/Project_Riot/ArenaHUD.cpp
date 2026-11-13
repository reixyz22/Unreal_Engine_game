#include "ArenaHUD.h"

#include "ArenaEnemy.h"
#include "ArenaGameMode.h"
#include "ArenaPlayerPawn.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "EngineUtils.h"
#include "HealthComponent.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"

namespace
{
	const FLinearColor HealthRed(0.95f, 0.12f, 0.18f);
	const FLinearColor ManaBlue(0.25f, 0.55f, 1.f);
	const FLinearColor BarBacking(0.03f, 0.02f, 0.05f, 0.85f);
	const FLinearColor WaveYellow(1.f, 0.85f, 0.2f);
	constexpr float OverheadBarWidth = 56.f;
	constexpr float HeroOverheadBarWidth = 80.f;
	constexpr float OverheadBarHeight = 7.f;
	constexpr float BannerTop = 18.f;
	constexpr float BannerScale = 1.6f;
	constexpr float IconSize = 40.f;
}

void AArenaHUD::DrawHUD()
{
	Super::DrawHUD();

	DrawHealthBars();

	const AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();
	if (GameMode)
	{
		DrawWaveBanner(*GameMode);
		DrawDeathBanner(*GameMode);
	}
	if (const AArenaPlayerPawn* Hero = Cast<AArenaPlayerPawn>(GetOwningPawn()))
	{
		DrawHeroPanel(*Hero);
	}
}

void AArenaHUD::DrawHealthBars()
{
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		const UHealthComponent* Health = It->FindComponentByClass<UHealthComponent>();
		const UPaperFlipbookComponent* Body = It->FindComponentByClass<UPaperFlipbookComponent>();
		if (!Health || !Body || Health->IsDead())
		{
			continue;
		}

		const FVector Top = Body->Bounds.Origin + FVector(0.0, 0.0, Body->Bounds.BoxExtent.Z + 6.0);
		const FVector Screen = Project(Top);
		const float Width = It->IsA<APawn>() ? HeroOverheadBarWidth : OverheadBarWidth;
		DrawBar(Screen.X - Width * 0.5f, Screen.Y - OverheadBarHeight, Width, OverheadBarHeight,
			Health->GetHealth() / Health->GetMaxHealth(), HealthRed);
	}
}

void AArenaHUD::DrawWaveBanner(const AArenaGameMode& GameMode)
{
	if (GameMode.GetBannerWave() == 0)
	{
		return;
	}

	// Lay out "WAVE 3 [ icon x2 + icon x3 ]" left to right, measured first so it can be centered.
	struct FPiece { FString Text; UTexture2D* Icon = nullptr; };
	TArray<FPiece> Pieces;
	Pieces.Add({ FString::Printf(TEXT("%sWAVE %d  [ "), GameMode.IsWaveIncoming() ? TEXT("INCOMING  ") : TEXT(""), GameMode.GetBannerWave()) });
	const TArray<FWaveGroup>& Groups = GameMode.GetBannerGroups();
	for (int32 Index = 0; Index < Groups.Num(); ++Index)
	{
		const AArenaEnemy* Enemy = Groups[Index].Type ? Groups[Index].Type->GetDefaultObject<AArenaEnemy>() : nullptr;
		const UPaperSprite* Icon = Enemy ? Enemy->GetIcon() : nullptr;
		Pieces.Add({ FString(), Icon ? Icon->GetBakedTexture() : nullptr });
		Pieces.Add({ FString::Printf(TEXT("x%d%s"), Groups[Index].Count, Index + 1 < Groups.Num() ? TEXT("  +  ") : TEXT("")) });
	}
	Pieces.Add({ TEXT(" ]") });

	UFont* Font = GEngine->GetLargeFont();
	float TotalWidth = 0.f;
	float TextHeight = 0.f;
	for (const FPiece& Piece : Pieces)
	{
		float W = IconSize, H = 0.f;
		if (!Piece.Icon)
		{
			GetTextSize(Piece.Text, W, H, Font, BannerScale);
			TextHeight = FMath::Max(TextHeight, H);
		}
		TotalWidth += W;
	}

	float X = (Canvas->ClipX - TotalWidth) * 0.5f;
	for (const FPiece& Piece : Pieces)
	{
		if (Piece.Icon)
		{
			const float IconY = BannerTop + (TextHeight - IconSize) * 0.5f;
			DrawTexture(Piece.Icon, X, IconY, IconSize, IconSize, 0.f, 0.f, 1.f, 1.f, FLinearColor::White, BLEND_Translucent);
			X += IconSize;
			continue;
		}
		float W, H;
		GetTextSize(Piece.Text, W, H, Font, BannerScale);
		DrawText(Piece.Text, FLinearColor(0.f, 0.f, 0.f, 0.8f), X + 2.f, BannerTop + 2.f, Font, BannerScale);
		DrawText(Piece.Text, WaveYellow, X, BannerTop, Font, BannerScale);
		X += W;
	}
}

void AArenaHUD::DrawHeroPanel(const AArenaPlayerPawn& Hero)
{
	// Bottom-center panel: health, then mana. Specials will sit in a row above it.
	const float Width = FMath::Min(420.f, Canvas->ClipX * 0.4f);
	const float X = (Canvas->ClipX - Width) * 0.5f;
	const float Y = Canvas->ClipY - 56.f;

	const UHealthComponent& Health = Hero.GetHealth();
	DrawBar(X, Y, Width, 18.f, Health.GetHealth() / Health.GetMaxHealth(), HealthRed);
	DrawCentered(FString::Printf(TEXT("%.0f / %.0f"), Health.GetHealth(), Health.GetMaxHealth()),
		Canvas->ClipX * 0.5f, Y + 2.f, FLinearColor::White, 1.f, GEngine->GetSmallFont());

	DrawBar(X, Y + 24.f, Width, 10.f, Hero.GetManaFraction(), ManaBlue);
}

void AArenaHUD::DrawDeathBanner(const AArenaGameMode& GameMode)
{
	if (!GameMode.IsPlayerDead())
	{
		return;
	}
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.5f), 0.f, 0.f, Canvas->ClipX, Canvas->ClipY);
	DrawCentered(FString::Printf(TEXT("ROUNDS SURVIVED: %d"), GameMode.GetRoundsSurvived()),
		Canvas->ClipX * 0.5f, Canvas->ClipY * 0.45f, WaveYellow, 2.f);
}

void AArenaHUD::DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& Fill)
{
	DrawRect(BarBacking, X - 2.f, Y - 2.f, Width + 4.f, Height + 4.f);
	DrawRect(Fill, X, Y, Width * FMath::Clamp(Fraction, 0.f, 1.f), Height);
}

void AArenaHUD::DrawCentered(const FString& Text, float CenterX, float Y, const FLinearColor& Color, float Scale, UFont* Font)
{
	Font = Font ? Font : GEngine->GetLargeFont();
	float W, H;
	GetTextSize(Text, W, H, Font, Scale);
	DrawText(Text, Color, CenterX - W * 0.5f, Y, Font, Scale);
}

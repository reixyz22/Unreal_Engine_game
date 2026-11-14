#include "ArenaHUD.h"

#include "ArenaEnemy.h"
#include "ArenaGameMode.h"
#include "ArenaPlayerPawn.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"

namespace
{
	const FLinearColor HealthTop(0.93f, 0.26f, 0.22f);
	const FLinearColor HealthBottom(0.62f, 0.07f, 0.09f);
	const FLinearColor HealthChip(1.f, 0.86f, 0.55f);
	const FLinearColor Notch(0.f, 0.f, 0.f, 0.55f);
	const FLinearColor ManaBlue(0.25f, 0.55f, 1.f);
	const FLinearColor BarBacking(0.03f, 0.02f, 0.05f, 0.85f);
	const FLinearColor Wood(0.2f, 0.11f, 0.05f, 0.96f);
	const FLinearColor WoodEdge(0.55f, 0.34f, 0.15f);
	const FLinearColor Parchment(0.86f, 0.74f, 0.52f);
	const FLinearColor ParchmentLit(0.98f, 0.88f, 0.64f);
	const FLinearColor Ink(0.24f, 0.13f, 0.05f);
	constexpr float HealthPerNotch = 10.f;
	const FLinearColor WaveYellow(1.f, 0.85f, 0.2f);
	constexpr float OverheadBarWidth = 56.f;
	constexpr float HeroOverheadBarWidth = 80.f;
	constexpr float OverheadBarHeight = 7.f;
	constexpr float BannerTop = 18.f;
	constexpr float BannerScale = 1.6f;
	constexpr float IconSize = 40.f;
}

AArenaHUD::AArenaHUD()
{
	PrimaryActorTick.bCanEverTick = true;
	// The HUD keeps ticking while paused so it can run the pause menu.
	SetTickableWhenPaused(true);
}

void AArenaHUD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!PlayerOwner)
	{
		return;
	}

	// Escape in a packaged game; P too, because Escape stops Play-In-Editor.
	if (PlayerOwner->WasInputKeyJustPressed(EKeys::Escape) || PlayerOwner->WasInputKeyJustPressed(EKeys::P))
	{
		SetMenuOpen(!bMenuOpen);
	}
	else if (bMenuOpen && PlayerOwner->WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		FVector2D Mouse;
		if (PlayerOwner->GetMousePosition(Mouse.X, Mouse.Y))
		{
			ClickMenu(Mouse);
		}
	}
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
		DrawSpecialSlot(*Hero);
		DrawStunStars(*Hero);
	}
	if (bMenuOpen)
	{
		DrawPauseMenu();
	}
}

void AArenaHUD::DrawHealthBars()
{
	for (auto It = Bars.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}

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
		DrawHealthBar(*Health, Screen.X - Width * 0.5f, Screen.Y - OverheadBarHeight, Width, OverheadBarHeight);
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
	// Bottom-center panel: Nikki's health.
	const float Width = FMath::Min(420.f, Canvas->ClipX * 0.4f);
	const float X = (Canvas->ClipX - Width) * 0.5f;
	const float Y = Canvas->ClipY - 56.f;

	const UHealthComponent& Health = Hero.GetHealth();
	DrawHealthBar(Health, X, Y, Width, 18.f);
	DrawCentered(FString::Printf(TEXT("%.0f / %.0f"), Health.GetHealth(), Health.GetMaxHealth()),
		Canvas->ClipX * 0.5f, Y + 2.f, FLinearColor::White, 1.f, GEngine->GetSmallFont());
}

void AArenaHUD::DrawSpecialSlot(const AArenaPlayerPawn& Hero)
{
	// Bottom-left: the Q ability icon, greyed out until there's mana, with one pip per mana point.
	constexpr float Slot = 64.f;
	constexpr float Pip = 12.f;
	const float X = 24.f;
	const float Y = Canvas->ClipY - 24.f - Pip - 8.f - Slot;
	const bool bReady = Hero.GetMana() >= Hero.GetSpecialCost();

	DrawRect(BarBacking, X - 3.f, Y - 3.f, Slot + 6.f, Slot + 6.f);
	if (const UPaperSprite* Icon = Hero.GetSpecialIcon())
	{
		const FLinearColor Shade = bReady ? FLinearColor::White : FLinearColor(0.35f, 0.35f, 0.4f);
		DrawTexture(Icon->GetBakedTexture(), X + 6.f, Y + 6.f, Slot - 12.f, Slot - 12.f, 0.f, 0.f, 1.f, 1.f, Shade, BLEND_Translucent);
	}
	DrawText(TEXT("Q"), WaveYellow, X + 4.f, Y + 2.f, GEngine->GetSmallFont(), 1.f);

	for (int32 Index = 0; Index < Hero.GetMaxMana(); ++Index)
	{
		const float PipX = X + Index * (Pip + 6.f);
		const float PipY = Y + Slot + 8.f;
		DrawRect(ManaBlue, PipX, PipY, Pip, Pip);
		if (Index >= Hero.GetMana())
		{
			DrawRect(BarBacking, PipX + 2.f, PipY + 2.f, Pip - 4.f, Pip - 4.f);
		}
	}
}

void AArenaHUD::DrawStunStars(const AArenaPlayerPawn& Hero)
{
	if (!Hero.IsStunned())
	{
		return;
	}
	const UPaperFlipbookComponent* Body = Hero.FindComponentByClass<UPaperFlipbookComponent>();
	const FVector Top = Body->Bounds.Origin + FVector(0.0, 0.0, Body->Bounds.BoxExtent.Z + 30.0);
	const FVector Screen = Project(Top);
	const float Spin = GetWorld()->GetTimeSeconds() * 6.f;
	for (int32 Star = 0; Star < 3; ++Star)
	{
		const float Angle = Spin + Star * UE_TWO_PI / 3.f;
		DrawRect(WaveYellow, Screen.X + FMath::Cos(Angle) * 16.f - 3.f, Screen.Y + FMath::Sin(Angle) * 5.f - 3.f, 6.f, 6.f);
	}
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

void AArenaHUD::DrawHealthBar(const UHealthComponent& Health, float X, float Y, float Width, float Height)
{
	// Smooth: the fill eases to the real value, and a pale chip shows what was just lost.
	FBarState& State = Bars.FindOrAdd(&Health);
	const float Actual = Health.GetHealth() / Health.GetMaxHealth();
	const float DeltaSeconds = GetWorld()->GetDeltaSeconds();
	State.Shown = State.Shown < 0.f ? Actual : FMath::FInterpTo(State.Shown, Actual, DeltaSeconds, 14.f);
	State.Trail = State.Trail < State.Shown ? State.Shown : FMath::FInterpTo(State.Trail, State.Shown, DeltaSeconds, 2.5f);

	DrawRect(BarBacking, X - 2.f, Y - 2.f, Width + 4.f, Height + 4.f);
	DrawRect(HealthChip, X, Y, Width * State.Trail, Height);
	const float Fill = Width * State.Shown;
	DrawRect(HealthBottom, X, Y, Fill, Height);
	DrawRect(HealthTop, X, Y, Fill, FMath::Max(1.f, Height * 0.4f));

	// League-style notches: a short tick every 10 health, a full-height one every 50.
	for (float Mark = HealthPerNotch; Mark < Health.GetMaxHealth(); Mark += HealthPerNotch)
	{
		const bool bMajor = FMath::IsNearlyZero(FMath::Fmod(Mark, HealthPerNotch * 5.f));
		DrawRect(Notch, X + Width * Mark / Health.GetMaxHealth(), Y, 1.f, bMajor ? Height : Height * 0.55f);
	}
}

void AArenaHUD::DrawBox(float X, float Y, float Width, float Height, const FLinearColor& Fill, const FLinearColor& Border, float BorderWidth)
{
	DrawRect(Border, X, Y, Width, Height);
	DrawRect(Fill, X + BorderWidth, Y + BorderWidth, Width - BorderWidth * 2.f, Height - BorderWidth * 2.f);
}

void AArenaHUD::SetMenuOpen(bool bOpen)
{
	bMenuOpen = bOpen;
	UGameplayStatics::SetGamePaused(this, bOpen);
}

void AArenaHUD::BuildMenuButtons(AArenaPlayerPawn& Hero)
{
	MenuButtons.Reset();
	MenuButtons.Add({ TEXT("Resume"), FBox2D(), [this]() { SetMenuOpen(false); } });
	MenuButtons.Add({ FString::Printf(TEXT("Click to move: %s"), Hero.IsClickToMove() ? TEXT("On") : TEXT("Off")),
		FBox2D(), [&Hero]() { Hero.SetClickToMove(!Hero.IsClickToMove()); } });
	MenuButtons.Add({ FString::Printf(TEXT("Fog: %s"), Hero.IsFogEnabled() ? TEXT("On") : TEXT("Off")),
		FBox2D(), [&Hero]() { Hero.SetFogEnabled(!Hero.IsFogEnabled()); } });
	MenuButtons.Add({ TEXT("Quit"), FBox2D(), [this]() { UKismetSystemLibrary::QuitGame(this, PlayerOwner, EQuitPreference::Quit, false); } });
}

void AArenaHUD::DrawPauseMenu()
{
	AArenaPlayerPawn* Hero = Cast<AArenaPlayerPawn>(GetOwningPawn());
	if (!Hero)
	{
		return;
	}
	BuildMenuButtons(*Hero);

	UFont* Font = MenuFont ? MenuFont.Get() : GEngine->GetLargeFont();
	constexpr float ButtonWidth = 360.f;
	constexpr float ButtonHeight = 56.f;
	constexpr float Gap = 14.f;
	constexpr float TitleSpace = 110.f;
	const float PanelWidth = ButtonWidth + 80.f;
	const float PanelHeight = TitleSpace + MenuButtons.Num() * (ButtonHeight + Gap) + 20.f;
	const float PanelX = (Canvas->ClipX - PanelWidth) * 0.5f;
	const float PanelY = (Canvas->ClipY - PanelHeight) * 0.5f;

	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), 0.f, 0.f, Canvas->ClipX, Canvas->ClipY);
	if (PanelTexture)
	{
		DrawTexture(PanelTexture, PanelX, PanelY, PanelWidth, PanelHeight, 0.f, 0.f, 1.f, 1.f, FLinearColor::White, BLEND_Translucent);
	}
	else
	{
		DrawBox(PanelX, PanelY, PanelWidth, PanelHeight, Wood, WoodEdge, 5.f);
		DrawRect(WoodEdge * 0.6f, PanelX + 14.f, PanelY + 82.f, PanelWidth - 28.f, 2.f);
	}
	DrawCentered(TEXT("PAUSED"), Canvas->ClipX * 0.5f, PanelY + 22.f, Parchment, 2.f, Font);

	FVector2D Mouse = FVector2D::ZeroVector;
	PlayerOwner->GetMousePosition(Mouse.X, Mouse.Y);
	const float X = (Canvas->ClipX - ButtonWidth) * 0.5f;
	float Y = PanelY + TitleSpace;
	for (FMenuButton& Button : MenuButtons)
	{
		Button.Rect = FBox2D(FVector2D(X, Y), FVector2D(X + ButtonWidth, Y + ButtonHeight));
		const bool bHovered = Button.Rect.IsInside(Mouse);
		if (ButtonTexture)
		{
			DrawTexture(ButtonTexture, X, Y, ButtonWidth, ButtonHeight, 0.f, 0.f, 1.f, 1.f,
				bHovered ? FLinearColor(1.15f, 1.1f, 1.f) : FLinearColor::White, BLEND_Translucent);
		}
		else
		{
			DrawBox(X, Y, ButtonWidth, ButtonHeight, bHovered ? ParchmentLit : Parchment, Ink, 3.f);
		}
		float TextWidth, TextHeight;
		GetTextSize(Button.Label, TextWidth, TextHeight, Font, 1.3f);
		DrawText(Button.Label, Ink, X + (ButtonWidth - TextWidth) * 0.5f, Y + (ButtonHeight - TextHeight) * 0.5f, Font, 1.3f);
		Y += ButtonHeight + Gap;
	}
}

void AArenaHUD::ClickMenu(const FVector2D& Mouse)
{
	for (const FMenuButton& Button : MenuButtons)
	{
		if (Button.Rect.IsInside(Mouse))
		{
			Button.Action();
			return;
		}
	}
}

void AArenaHUD::DrawCentered(const FString& Text, float CenterX, float Y, const FLinearColor& Color, float Scale, UFont* Font)
{
	Font = Font ? Font : GEngine->GetLargeFont();
	float W, H;
	GetTextSize(Text, W, H, Font, Scale);
	DrawText(Text, Color, CenterX - W * 0.5f, Y, Font, Scale);
}

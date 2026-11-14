#include "ArenaHUD.h"

#include "ArenaEnemy.h"
#include "ArenaGameMode.h"
#include "ArenaPlayerPawn.h"
#include "ArenaSession.h"
#include "ArenaTheme.h"
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

void AArenaHUD::BeginPlay()
{
	Super::BeginPlay();

	// The start screens show once per launch; restarts after a death go straight to play.
	const UArenaSession* Session = UArenaSession::Get(this);
	ShowScreen(Session && !Session->bIntroDone ? EScreen::ChooseHero : EScreen::Playing);
}

void AArenaHUD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!PlayerOwner)
	{
		return;
	}

	// Escape in a packaged game; P too, because Escape stops Play-In-Editor.
	const bool bPauseKey = PlayerOwner->WasInputKeyJustPressed(EKeys::Escape) || PlayerOwner->WasInputKeyJustPressed(EKeys::P);
	const bool bClicked = PlayerOwner->WasInputKeyJustPressed(EKeys::LeftMouseButton);

	switch (CurrentScreen)
	{
	case EScreen::ChooseHero:
		if (bClicked)
		{
			ClickMenu(GetMouse());
		}
		break;
	case EScreen::Controls:
		for (const FKey& Key : { EKeys::W, EKeys::A, EKeys::S, EKeys::D })
		{
			if (PlayerOwner->WasInputKeyJustPressed(Key))
			{
				ShowScreen(EScreen::Playing);
				break;
			}
		}
		break;
	case EScreen::Playing:
		if (bPauseKey)
		{
			ShowScreen(EScreen::Paused);
		}
		break;
	case EScreen::Paused:
		if (bPauseKey)
		{
			ShowScreen(EScreen::Playing);
		}
		else if (bClicked)
		{
			ClickMenu(GetMouse());
		}
		if (PlayerOwner->IsInputKeyDown(EKeys::LeftMouseButton))
		{
			DragSliders(GetMouse());
		}
		break;
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
	switch (CurrentScreen)
	{
	case EScreen::ChooseHero: DrawHeroSelect(); break;
	case EScreen::Controls: DrawControls(); break;
	case EScreen::Paused: DrawPauseMenu(); break;
	default: break;
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
		const UPaperSprite* Icon = GameMode.GetEnemyIcon(Groups[Index].Type);
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

void AArenaHUD::ShowScreen(EScreen NewScreen)
{
	CurrentScreen = NewScreen;
	const bool bMenu = NewScreen != EScreen::Playing;
	UGameplayStatics::SetGamePaused(this, bMenu);

	if (UArenaSession* Session = UArenaSession::Get(this))
	{
		Session->SetMusicDucked(bMenu);
		Session->bIntroDone |= NewScreen == EScreen::Playing;
	}
}

FVector2D AArenaHUD::GetMouse() const
{
	FVector2D Mouse = FVector2D::ZeroVector;
	if (PlayerOwner)
	{
		PlayerOwner->GetMousePosition(Mouse.X, Mouse.Y);
	}
	return Mouse;
}

UFont* AArenaHUD::GetMenuFont() const
{
	return MenuFont ? MenuFont.Get() : GEngine->GetLargeFont();
}

void AArenaHUD::DrawPanel(float X, float Y, float Width, float Height, const FString& Title)
{
	// Menus take the run's theme wood; everything in play stays fixed.
	const AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();
	FLinearColor PanelWood = GameMode ? GameMode->GetTheme().MenuWood : Wood;
	PanelWood.A = Wood.A;

	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), 0.f, 0.f, Canvas->ClipX, Canvas->ClipY);
	if (PanelTexture)
	{
		DrawTexture(PanelTexture, X, Y, Width, Height, 0.f, 0.f, 1.f, 1.f, FLinearColor::White, BLEND_Translucent);
	}
	else
	{
		DrawBox(X, Y, Width, Height, PanelWood, WoodEdge, 5.f);
		DrawRect(WoodEdge * 0.6f, X + 14.f, Y + 82.f, Width - 28.f, 2.f);
	}
	DrawCentered(Title, X + Width * 0.5f, Y + 22.f, Parchment, 2.f, GetMenuFont());
}

void AArenaHUD::DrawButton(FMenuButton& Button, const FVector2D& Mouse)
{
	const FVector2D Min = Button.Rect.Min;
	const FVector2D Size = Button.Rect.GetSize();
	const bool bHovered = Button.Rect.IsInside(Mouse);
	if (ButtonTexture)
	{
		DrawTexture(ButtonTexture, Min.X, Min.Y, Size.X, Size.Y, 0.f, 0.f, 1.f, 1.f,
			bHovered ? FLinearColor(1.15f, 1.1f, 1.f) : FLinearColor::White, BLEND_Translucent);
	}
	else
	{
		DrawBox(Min.X, Min.Y, Size.X, Size.Y, bHovered ? ParchmentLit : Parchment, Ink, 3.f);
	}

	UFont* Font = GetMenuFont();
	float TextWidth, TextHeight;
	GetTextSize(Button.Label, TextWidth, TextHeight, Font, 1.3f);
	float TextY = Min.Y + (Size.Y - TextHeight) * 0.5f;
	if (Button.Icon)
	{
		// Hero cards: a big portrait above the name.
		const float Portrait = Size.X - 40.f;
		DrawTexture(Button.Icon, Min.X + 20.f, Min.Y + 16.f, Portrait, Portrait * 1.5f, 0.f, 0.f, 1.f, 1.f, FLinearColor::White, BLEND_Translucent);
		TextY = Min.Y + Size.Y - TextHeight - 14.f;
	}
	DrawText(Button.Label, Ink, Min.X + (Size.X - TextWidth) * 0.5f, TextY, Font, 1.3f);
}

void AArenaHUD::DrawSlider(FMenuSlider& Slider, float X, float Y, float Width)
{
	constexpr float LabelWidth = 110.f;
	constexpr float TrackHeight = 10.f;
	DrawText(Slider.Label, Parchment, X, Y - 6.f, GetMenuFont(), 1.1f);

	const float TrackX = X + LabelWidth;
	const float TrackWidth = Width - LabelWidth;
	Slider.Track = FBox2D(FVector2D(TrackX, Y - 10.f), FVector2D(TrackX + TrackWidth, Y + TrackHeight + 10.f));
	const float Value = Slider.Get();
	DrawBox(TrackX, Y, TrackWidth, TrackHeight, BarBacking, Ink, 2.f);
	DrawRect(Parchment, TrackX + 2.f, Y + 2.f, (TrackWidth - 4.f) * Value, TrackHeight - 4.f);
	DrawBox(TrackX + TrackWidth * Value - 7.f, Y - 7.f, 14.f, TrackHeight + 14.f, ParchmentLit, Ink, 2.f);
}

void AArenaHUD::DrawHeroSelect()
{
	const AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();
	if (!GameMode)
	{
		return;
	}

	constexpr float CardWidth = 200.f;
	constexpr float CardHeight = 330.f;
	constexpr float Gap = 24.f;
	const TArray<TSubclassOf<AArenaPlayerPawn>>& Heroes = GameMode->GetHeroes();
	const float CardsWidth = FMath::Max(1, Heroes.Num()) * (CardWidth + Gap) - Gap;
	const float PanelWidth = FMath::Max(520.f, CardsWidth + 80.f);
	const float PanelHeight = 110.f + CardHeight + 40.f;
	const float PanelX = (Canvas->ClipX - PanelWidth) * 0.5f;
	const float PanelY = (Canvas->ClipY - PanelHeight) * 0.5f;
	DrawPanel(PanelX, PanelY, PanelWidth, PanelHeight, TEXT("CHOOSE YOUR HERO"));

	MenuButtons.Reset();
	float X = (Canvas->ClipX - CardsWidth) * 0.5f;
	for (const TSubclassOf<AArenaPlayerPawn>& Hero : Heroes)
	{
		const AArenaPlayerPawn* Defaults = Hero ? Hero->GetDefaultObject<AArenaPlayerPawn>() : nullptr;
		const UPaperSprite* Portrait = Defaults ? Defaults->GetPortrait() : nullptr;
		FMenuButton& Card = MenuButtons.Add_GetRef({ Defaults ? Defaults->GetHeroName().ToString() : TEXT("?"),
			FBox2D(FVector2D(X, PanelY + 110.f), FVector2D(X + CardWidth, PanelY + 110.f + CardHeight)),
			[this, Hero]()
			{
				if (AArenaGameMode* Mode = GetWorld()->GetAuthGameMode<AArenaGameMode>())
				{
					Mode->ChooseHero(Hero);
				}
				ShowScreen(EScreen::Controls);
			},
			Portrait ? Portrait->GetBakedTexture() : nullptr });
		DrawButton(Card, GetMouse());
		X += CardWidth + Gap;
	}
}

void AArenaHUD::DrawControls()
{
	static const TCHAR* Lines[] = {
		TEXT("WASD  -  move"),
		TEXT("Right-click an enemy  -  throw the ball and chain"),
		TEXT("Q  -  shock blast (1 mana; Clerics drop mana shards)"),
		TEXT("A thin gold line means a Cleric beam is coming: get out of it"),
		TEXT("Esc  -  menu: pause, volume, fog  (P works too)"),
	};
	constexpr float LineHeight = 40.f;
	const float PanelWidth = FMath::Min(760.f, Canvas->ClipX - 40.f);
	const float PanelHeight = 110.f + UE_ARRAY_COUNT(Lines) * LineHeight + 80.f;
	const float PanelX = (Canvas->ClipX - PanelWidth) * 0.5f;
	const float PanelY = (Canvas->ClipY - PanelHeight) * 0.5f;
	DrawPanel(PanelX, PanelY, PanelWidth, PanelHeight, TEXT("HOW TO PLAY"));

	float Y = PanelY + 110.f;
	for (const TCHAR* Line : Lines)
	{
		DrawText(Line, Parchment, PanelX + 40.f, Y, GetMenuFont(), 1.1f);
		Y += LineHeight;
	}
	const float Blink = 0.6f + 0.4f * FMath::Sin(GetWorld()->GetRealTimeSeconds() * 4.f);
	DrawCentered(TEXT("Press W, A, S or D to start"), Canvas->ClipX * 0.5f, Y + 24.f, WaveYellow * Blink, 1.3f, GetMenuFont());
}

void AArenaHUD::DrawPauseMenu()
{
	AArenaPlayerPawn* Hero = Cast<AArenaPlayerPawn>(GetOwningPawn());
	UArenaSession* Session = UArenaSession::Get(this);
	if (!Hero || !Session)
	{
		return;
	}

	MenuButtons.Reset();
	MenuButtons.Add({ TEXT("Resume"), FBox2D(), [this]() { ShowScreen(EScreen::Playing); } });
	MenuButtons.Add({ FString::Printf(TEXT("Click to move: %s"), Hero->IsClickToMove() ? TEXT("On") : TEXT("Off")),
		FBox2D(), [Hero]() { Hero->SetClickToMove(!Hero->IsClickToMove()); } });
	MenuButtons.Add({ FString::Printf(TEXT("Fog: %s"), Hero->IsFogEnabled() ? TEXT("On") : TEXT("Off")),
		FBox2D(), [Hero]() { Hero->SetFogEnabled(!Hero->IsFogEnabled()); } });
	MenuButtons.Add({ TEXT("Quit"), FBox2D(), [this]() { UKismetSystemLibrary::QuitGame(this, PlayerOwner, EQuitPreference::Quit, false); } });

	MenuSliders.Reset();
	MenuSliders.Add({ TEXT("Music"), FBox2D(), [Session]() { return Session->GetMusicVolume(); }, [Session](float V) { Session->SetMusicVolume(V); } });
	MenuSliders.Add({ TEXT("Effects"), FBox2D(), [Session]() { return Session->GetSfxVolume(); }, [Session](float V) { Session->SetSfxVolume(V); } });

	constexpr float ButtonWidth = 360.f;
	constexpr float ButtonHeight = 56.f;
	constexpr float Gap = 14.f;
	constexpr float SliderRow = 46.f;
	const float PanelWidth = ButtonWidth + 80.f;
	const float PanelHeight = 110.f + MenuButtons.Num() * (ButtonHeight + Gap) + MenuSliders.Num() * SliderRow + 30.f;
	const float PanelX = (Canvas->ClipX - PanelWidth) * 0.5f;
	const float PanelY = (Canvas->ClipY - PanelHeight) * 0.5f;
	DrawPanel(PanelX, PanelY, PanelWidth, PanelHeight, TEXT("PAUSED"));

	const FVector2D Mouse = GetMouse();
	const float X = (Canvas->ClipX - ButtonWidth) * 0.5f;
	float Y = PanelY + 110.f;
	for (FMenuButton& Button : MenuButtons)
	{
		Button.Rect = FBox2D(FVector2D(X, Y), FVector2D(X + ButtonWidth, Y + ButtonHeight));
		DrawButton(Button, Mouse);
		Y += ButtonHeight + Gap;
	}
	Y += 16.f;
	for (FMenuSlider& Slider : MenuSliders)
	{
		DrawSlider(Slider, X, Y, ButtonWidth);
		Y += SliderRow;
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

void AArenaHUD::DragSliders(const FVector2D& Mouse)
{
	for (const FMenuSlider& Slider : MenuSliders)
	{
		if (Slider.Track.IsInside(Mouse))
		{
			Slider.Set((Mouse.X - Slider.Track.Min.X) / Slider.Track.GetSize().X);
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

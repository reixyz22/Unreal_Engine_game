#include "ArenaHUD.h"

#include "ArenaArt.h"
#include "ArenaEnemy.h"
#include "ArenaGameMode.h"
#include "ArenaLeaderboard.h"
#include "ArenaPlayerPawn.h"
#include "ArenaSession.h"
#include "ArenaShop.h"
#include "ArenaTheme.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HealthComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "StoryProp.h"
#include "StoryScene.h"

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
	const FLinearColor Gold(0.96f, 0.8f, 0.45f);
	// UI art is pixel art drawn at this many screen pixels per art pixel.
	constexpr float UiScale = 3.f;
	constexpr float PanelCorner = 8.f;
	constexpr float ButtonCorner = 4.f;
	constexpr double DeathScreenDelay = 1.2;
	constexpr double StageClearDelay = 1.0;
	// World map art is 256 x 352.
	constexpr float MapAspect = 256.f / 352.f;
	constexpr double FadeInSeconds = 0.8;
	constexpr double PopupSeconds = 1.2;

	// Combo callouts, in rainbow order.
	const TCHAR* const ComboWords[] = { TEXT(""), TEXT("COOL!"), TEXT("RADICAL!"), TEXT("FANTASTIC!"), TEXT("SPECTACULAR!"), TEXT("LEGENDARY!") };
	const FLinearColor ComboColors[] = {
		FLinearColor::White, FLinearColor(1.f, 0.25f, 0.25f), FLinearColor(1.f, 0.6f, 0.15f),
		FLinearColor(1.f, 0.95f, 0.2f), FLinearColor(0.3f, 1.f, 0.4f), FLinearColor(0.45f, 0.6f, 1.f) };

	const TCHAR* const CreditLines[] = {
		TEXT("Game developer and code auditor  -  William \"reixyz\" Pitts"),
		TEXT("Spaceman and guitar art  -  William \"reixyz\" Pitts"),
		TEXT("AI pair programmer (UI, art and audio tools, engine glue)  -  Claude by Anthropic"),
		TEXT("Nikki  -  KingBell, Pixel Art Sprite Mixer (CC BY 4.0)"),
		TEXT("Pyros, Clerics, dungeon floor  -  0x72, DungeonTileset II (CC0)"),
		TEXT("Flail  -  Dungeon Crawl Stone Soup (CC0)"),
		TEXT("Cursors  -  Kenney, Cursor Pixel Pack (CC0)"),
		TEXT("Music  -  obscure music, yd, wipics (CC0)"),
		TEXT("Sound effects  -  synthesized for this game"),
		TEXT("Built with Unreal Engine 5.8"),
	};
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
	UArenaSession* Session = UArenaSession::Get(this);
	const AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();
	const EArenaScene Scene = GameMode ? GameMode->GetSceneKind() : EArenaScene::Stage;
	if (Session && !Session->bIntroDone)
	{
		ShowScreen(EScreen::ChooseHero);
	}
	else if (Scene == EArenaScene::WorldMap)
	{
		// Arriving from the village puts the marker on it; otherwise on the stage just played.
		MapSelection = Session->PreviousScene == EArenaScene::Town ? 0 : FMath::Clamp(Session->StageIndex, 0, Session->HighestUnlocked);
		ShowScreen(EScreen::WorldMap);
	}
	else if (Scene == EArenaScene::Stage && Session && !Session->bControlsShown)
	{
		// How to play shows once, before the first fight.
		Session->bControlsShown = true;
		ShowScreen(EScreen::Controls);
	}
	else
	{
		ShowScreen(EScreen::Playing);
		FadeInFrom = GetWorld()->GetRealTimeSeconds();
	}
}

void AArenaHUD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!PlayerOwner)
	{
		return;
	}

	TickDebug();

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
	{
		const AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();
		if (GameMode && GameMode->IsStageCleared())
		{
			const double Now = GetWorld()->GetRealTimeSeconds();
			ClearNoticedAt = ClearNoticedAt < 0.0 ? Now : ClearNoticedAt;
			if (Now - ClearNoticedAt > StageClearDelay)
			{
				UArenaSession::PlaySfx(this, GameMode->GetStorySound(TEXT("Fanfare")));
				ShowScreen(EScreen::StageClear);
			}
		}
		else if (GameMode && GameMode->IsPlayerDead())
		{
			// Let the death read for a moment, then the death screen.
			const double Now = GetWorld()->GetRealTimeSeconds();
			DeathNoticedAt = DeathNoticedAt < 0.0 ? Now : DeathNoticedAt;
			if (Now - DeathNoticedAt > DeathScreenDelay)
			{
				Board = UArenaLeaderboard::Load();
				ShowScreen(Board->Qualifies(GameMode->GetScore()) ? EScreen::Initials : EScreen::GameOver);
			}
		}
		else if (bPauseKey)
		{
			ShowScreen(EScreen::Paused);
		}
		break;
	}
	case EScreen::Initials:
		TickInitials();
		break;
	case EScreen::WorldMap:
		TickWorldMap();
		if (bClicked)
		{
			ClickMenu(GetMouse());
		}
		break;
	case EScreen::Shop:
		if (bPauseKey)
		{
			ShowScreen(EScreen::Playing);
		}
		else if (bClicked)
		{
			ClickMenu(GetMouse());
		}
		break;
	case EScreen::GameOver:
	case EScreen::Leaderboard:
	case EScreen::Credits:
	case EScreen::StageClear:
		if (bClicked)
		{
			ClickMenu(GetMouse());
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

	const AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();
	const AArenaPlayerPawn* Hero = Cast<AArenaPlayerPawn>(GetOwningPawn());
	if (GameMode && GameMode->IsCombatStage())
	{
		DrawHealthBars();
		DrawWaveBanner(*GameMode);
		DrawPopups(*GameMode);
		DrawScore(*GameMode);
		if (Hero)
		{
			DrawSpecialsBar(*Hero);
			DrawStunStars(*Hero);
		}
	}
	else if (const AStoryScene* Scene = GameMode ? GameMode->GetScene() : nullptr)
	{
		DrawStory(*Scene);
	}
	switch (CurrentScreen)
	{
	case EScreen::ChooseHero: DrawHeroSelect(); break;
	case EScreen::Controls: DrawControls(); break;
	case EScreen::Paused: DrawPauseMenu(); break;
	case EScreen::GameOver: DrawGameOver(); break;
	case EScreen::Initials: DrawInitials(); break;
	case EScreen::Leaderboard: DrawLeaderboard(); break;
	case EScreen::Credits: DrawCredits(); break;
	case EScreen::WorldMap: DrawWorldMap(); break;
	case EScreen::StageClear: DrawStageClear(); break;
	case EScreen::Shop: DrawShop(); break;
	default: break;
	}

	const UArenaSession* Session = UArenaSession::Get(this);
	if (Session && Session->bDebug)
	{
		DrawText(TEXT("DEBUG   F2 +mana   F3 +5000 gold   F4 clear wave   F5 unlock all   F6 world map   (F1 off)"),
			FLinearColor(0.4f, 1.f, 0.5f), 12.f, Canvas->ClipY - 24.f, GEngine->GetSmallFont(), 1.f);
	}

	// Restarts fade in from black so a new run reads as a continuation, not a cut.
	const double SinceFade = GetWorld()->GetRealTimeSeconds() - FadeInFrom;
	if (FadeInFrom >= 0.0 && SinceFade < FadeInSeconds)
	{
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 1.f - float(SinceFade / FadeInSeconds)), 0.f, 0.f, Canvas->ClipX, Canvas->ClipY);
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
	struct FPiece { FString Text; TSubclassOf<AArenaEnemy> Icon; };
	TArray<FPiece> Pieces;
	Pieces.Add({ FString::Printf(TEXT("%sWAVE %d / %d  [ "), GameMode.IsWaveIncoming() ? TEXT("INCOMING  ") : TEXT(""), GameMode.GetBannerWave(), GameMode.GetStageWaves()) });
	const TArray<FWaveGroup>& Groups = GameMode.GetBannerGroups();
	for (int32 Index = 0; Index < Groups.Num(); ++Index)
	{
		Pieces.Add({ FString(), Groups[Index].Type });
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
			DrawEnemyIcon(GameMode, Piece.Icon, X, BannerTop + (TextHeight - IconSize) * 0.5f, IconSize);
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

void AArenaHUD::DrawSpecialsBar(const AArenaPlayerPawn& Hero)
{
	if (!Hero.HasSpecial())
	{
		return;
	}

	// Bottom-middle bar of special slots (Q for now), each with its key and mana pips.
	// Health is already over Nikki's head, and the camera keeps her centred.
	constexpr float Slot = 120.f;       // slots are wide and short
	constexpr float SlotHeight = 76.f;
	constexpr float Pip = 14.f;
	constexpr float Padding = 18.f;
	const int32 Slots = 1;
	const float BarWidth = Slots * Slot + Padding * 2.f;
	const float BarHeight = SlotHeight + Pip + Padding * 2.f + 10.f;
	const float BarX = (Canvas->ClipX - BarWidth) * 0.5f;
	const float BarY = Canvas->ClipY - BarHeight - 12.f;
	if (PanelTexture)
	{
		DrawNineSlice(PanelTexture, PanelCorner, BarX, BarY, BarWidth, BarHeight, FLinearColor::White);
	}
	else
	{
		DrawRect(BarBacking, BarX, BarY, BarWidth, BarHeight);
	}

	const float X = BarX + Padding;
	const float Y = BarY + Padding;
	const bool bReady = Hero.GetMana() >= Hero.GetSpecialCost();
	DrawRect(BarBacking, X, Y, Slot, SlotHeight);
	if (const UPaperSprite* Icon = Hero.GetSpecialIcon())
	{
		const FLinearColor Shade = bReady ? FLinearColor::White : FLinearColor(0.35f, 0.35f, 0.4f);
		DrawTexture(Icon->GetBakedTexture(), X + 14.f, Y + 8.f, Slot - 28.f, SlotHeight - 16.f, 0.f, 0.f, 1.f, 1.f, Shade, BLEND_Translucent);
	}
	DrawText(TEXT("Q"), Gold, X + 6.f, Y + 4.f, GetMenuFont(), 1.f);

	const float PipsWidth = Hero.GetMaxMana() * (Pip + 6.f) - 6.f;
	for (int32 Index = 0; Index < Hero.GetMaxMana(); ++Index)
	{
		const float PipX = X + (Slot - PipsWidth) * 0.5f + Index * (Pip + 6.f);
		const float PipY = Y + SlotHeight + 10.f;
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

void AArenaHUD::DrawNineSlice(UTexture2D* Texture, float CornerPixels, float X, float Y, float Width, float Height, const FLinearColor& Tint)
{
	// Split the texture and the target box into a 3x3 grid. Corners keep their size,
	// edges stretch one way, and the middle stretches both ways.
	const float TexW = Texture->GetSizeX();
	const float TexH = Texture->GetSizeY();
	const float Corner = FMath::Min3(CornerPixels * UiScale, Width * 0.5f, Height * 0.5f);
	const float Xs[4] = { X, X + Corner, X + Width - Corner, X + Width };
	const float Ys[4] = { Y, Y + Corner, Y + Height - Corner, Y + Height };
	const float Us[4] = { 0.f, CornerPixels / TexW, 1.f - CornerPixels / TexW, 1.f };
	const float Vs[4] = { 0.f, CornerPixels / TexH, 1.f - CornerPixels / TexH, 1.f };
	for (int32 Row = 0; Row < 3; ++Row)
	{
		for (int32 Col = 0; Col < 3; ++Col)
		{
			DrawTexture(Texture, Xs[Col], Ys[Row], Xs[Col + 1] - Xs[Col], Ys[Row + 1] - Ys[Row],
				Us[Col], Vs[Row], Us[Col + 1] - Us[Col], Vs[Row + 1] - Vs[Row], Tint, BLEND_Translucent);
		}
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
		// The map and the shop are places, not pauses: music stays at full volume.
		Session->SetMusicDucked(bMenu && NewScreen != EScreen::WorldMap && NewScreen != EScreen::Shop);
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
	// The start screens hide the stage entirely; the pause menu dims it and takes the theme's tint.
	const bool bStartScreen = CurrentScreen == EScreen::ChooseHero || CurrentScreen == EScreen::Controls || CurrentScreen == EScreen::WorldMap;
	if (bStartScreen && BackdropTexture)
	{
		const float Tile = BackdropTexture->GetSizeX() * UiScale;
		DrawTexture(BackdropTexture, 0.f, 0.f, Canvas->ClipX, Canvas->ClipY, 0.f, 0.f,
			Canvas->ClipX / Tile, Canvas->ClipY / Tile, FLinearColor::White, BLEND_Opaque);
	}
	else
	{
		DrawRect(FLinearColor(0.f, 0.f, 0.f, bStartScreen ? 1.f : 0.55f), 0.f, 0.f, Canvas->ClipX, Canvas->ClipY);
	}

	const AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();
	const FLinearColor Tint = (!bStartScreen && GameMode) ? GameMode->GetTheme().MenuTint : FLinearColor::White;
	if (PanelTexture)
	{
		DrawNineSlice(PanelTexture, PanelCorner, X, Y, Width, Height, Tint);
	}
	else
	{
		DrawBox(X, Y, Width, Height, Wood, WoodEdge, 5.f);
	}
	DrawRect(Gold * 0.7f, X + 30.f, Y + 82.f, Width - 60.f, 2.f);
	DrawCentered(Title, X + Width * 0.5f + 2.f, Y + 24.f, FLinearColor(0.f, 0.f, 0.f, 0.7f), 2.f, GetMenuFont());
	DrawCentered(Title, X + Width * 0.5f, Y + 22.f, Gold, 2.f, GetMenuFont());
}

void AArenaHUD::DrawButton(FMenuButton& Button, const FVector2D& Mouse)
{
	const FVector2D Min = Button.Rect.Min;
	const FVector2D Size = Button.Rect.GetSize();
	const bool bHovered = Button.Rect.IsInside(Mouse);
	if (ButtonTexture)
	{
		DrawNineSlice(ButtonTexture, ButtonCorner, Min.X, Min.Y, Size.X, Size.Y,
			Button.bDisabled ? FLinearColor(0.5f, 0.48f, 0.46f) : bHovered ? FLinearColor(1.12f, 1.08f, 0.95f) : FLinearColor(0.92f, 0.9f, 0.88f));
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
	DrawRect(Gold * 0.8f, TrackX + 2.f, Y + 2.f, (TrackWidth - 4.f) * Value, TrackHeight - 4.f);
	const float KnobX = TrackX + TrackWidth * Value - 9.f;
	if (ButtonTexture)
	{
		DrawNineSlice(ButtonTexture, ButtonCorner, KnobX, Y - 8.f, 18.f, TrackHeight + 16.f, FLinearColor::White);
	}
	else
	{
		DrawBox(KnobX, Y - 7.f, 14.f, TrackHeight + 14.f, ParchmentLit, Ink, 2.f);
	}
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
				// Straight into the story; how-to-play waits for the first fight.
				ShowScreen(EScreen::Playing);
				FadeInFrom = GetWorld()->GetRealTimeSeconds();
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
	DrawCentered(TEXT("Press W, A, S or D to start"), Canvas->ClipX * 0.5f, Y + 24.f, Gold * Blink, 1.3f, GetMenuFont());
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
	if (Session->bContractSigned)
	{
		MenuButtons.Add({ TEXT("World map"), FBox2D(), [this, Session]() { Session->Travel(this, EArenaScene::WorldMap); } });
	}
	MenuButtons.Add({ TEXT("Credits"), FBox2D(), [this]() { CreditsReturnTo = EScreen::Paused; ShowScreen(EScreen::Credits); } });
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
		if (Button.Rect.IsInside(Mouse) && !Button.bDisabled)
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

void AArenaHUD::DrawEnemyIcon(const AArenaGameMode& GameMode, TSubclassOf<AArenaEnemy> Type, float X, float Y, float Size)
{
	const UPaperSprite* Sprite = GameMode.GetEnemyIcon(Type);
	UTexture2D* Texture = Sprite ? Sprite->GetBakedTexture() : nullptr;
	if (!Texture)
	{
		return;
	}

	// Recolored themes draw the icon through the same palette swap the enemies use.
	const FEnemySkin* Skin = GameMode.GetEnemySkin(Type);
	if (!IconMaterial || !Skin || Skin->Strength <= 0.f)
	{
		DrawTexture(Texture, X, Y, Size, Size, 0.f, 0.f, 1.f, 1.f, FLinearColor::White, BLEND_Translucent);
		return;
	}
	TObjectPtr<UMaterialInstanceDynamic>& Material = IconMaterials.FindOrAdd(Type);
	if (!Material)
	{
		Material = UMaterialInstanceDynamic::Create(IconMaterial, this);
		Material->SetTextureParameterValue(TEXT("IconTexture"), Texture);
		Material->SetVectorParameterValue(ArenaArt::PaletteShadowParam, Skin->Shadow);
		Material->SetVectorParameterValue(ArenaArt::PaletteLightParam, Skin->Light);
		Material->SetScalarParameterValue(ArenaArt::PaletteStrengthParam, Skin->Strength);
	}
	DrawMaterial(Material, X, Y, Size, Size, 0.f, 0.f, 1.f, 1.f);
}

void AArenaHUD::DrawScore(const AArenaGameMode& GameMode)
{
	const FString Text = FString::Printf(TEXT("SCORE  %s"), *FText::AsNumber(GameMode.GetScore()).ToString());
	float W, H;
	GetTextSize(Text, W, H, GEngine->GetLargeFont(), BannerScale);
	DrawText(Text, FLinearColor(0.f, 0.f, 0.f, 0.8f), Canvas->ClipX - W - 22.f, BannerTop + 2.f, GEngine->GetLargeFont(), BannerScale);
	DrawText(Text, WaveYellow, Canvas->ClipX - W - 24.f, BannerTop, GEngine->GetLargeFont(), BannerScale);
}

void AArenaHUD::DrawPopups(const AArenaGameMode& GameMode)
{
	// Each kill floats its points up; kills chained by one spell call out COOL .. LEGENDARY.
	const double Now = GetWorld()->GetTimeSeconds();
	for (const FScorePopup& Popup : GameMode.GetPopups())
	{
		const float Age = float(Now - Popup.Time);
		if (Age > PopupSeconds)
		{
			continue;
		}
		const FVector Screen = Project(Popup.Where + FVector(0.0, 0.0, 60.0 + Age * 70.0));
		FLinearColor Color = ComboColors[Popup.Tier];
		Color.A = 1.f - Age / float(PopupSeconds);
		const float Scale = 1.f + Popup.Tier * 0.2f;
		const FString Text = Popup.Tier > 0
			? FString::Printf(TEXT("%s +%d"), ComboWords[Popup.Tier], Popup.Points)
			: FString::Printf(TEXT("+%d"), Popup.Points);
		DrawCentered(Text, Screen.X + 2.f, Screen.Y + 2.f, FLinearColor(0.f, 0.f, 0.f, Color.A * 0.8f), Scale);
		DrawCentered(Text, Screen.X, Screen.Y, Color, Scale);
	}
}

void AArenaHUD::DrawMenuButtons(float X, float Y, float Width)
{
	constexpr float ButtonHeight = 56.f;
	constexpr float Gap = 14.f;
	const FVector2D Mouse = GetMouse();
	for (FMenuButton& Button : MenuButtons)
	{
		Button.Rect = FBox2D(FVector2D(X, Y), FVector2D(X + Width, Y + ButtonHeight));
		DrawButton(Button, Mouse);
		Y += ButtonHeight + Gap;
	}
}

void AArenaHUD::DrawGameOver()
{
	const AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();
	if (!GameMode)
	{
		return;
	}

	MenuButtons.Reset();
	MenuButtons.Add({ TEXT("Continue"), FBox2D(), [this]()
	{
		if (AArenaGameMode* Mode = GetWorld()->GetAuthGameMode<AArenaGameMode>())
		{
			Mode->RestartArena();
		}
	} });
	MenuButtons.Add({ TEXT("World map"), FBox2D(), [this]() { UArenaSession::Get(this)->Travel(this, EArenaScene::WorldMap); } });
	MenuButtons.Add({ TEXT("Leaderboard"), FBox2D(), [this]() { ShowScreen(EScreen::Leaderboard); } });
	MenuButtons.Add({ TEXT("Credits"), FBox2D(), [this]() { CreditsReturnTo = EScreen::GameOver; ShowScreen(EScreen::Credits); } });
	MenuButtons.Add({ TEXT("Quit"), FBox2D(), [this]() { UKismetSystemLibrary::QuitGame(this, PlayerOwner, EQuitPreference::Quit, false); } });

	constexpr float Width = 440.f;
	const float Height = 330.f + MenuButtons.Num() * 70.f;
	const float X = (Canvas->ClipX - Width) * 0.5f;
	const float Y = (Canvas->ClipY - Height) * 0.5f;
	DrawPanel(X, Y, Width, Height, TEXT("YOU DIED"));

	if (SkullTexture)
	{
		constexpr float Skull = 96.f;
		DrawTexture(SkullTexture, (Canvas->ClipX - Skull) * 0.5f, Y + 100.f, Skull, Skull, 0.f, 0.f, 1.f, 1.f, FLinearColor::White, BLEND_Translucent);
	}
	DrawCentered(FString::Printf(TEXT("Score %s   -   Wave %d"), *FText::AsNumber(GameMode->GetScore()).ToString(), GameMode->GetRoundsSurvived()),
		Canvas->ClipX * 0.5f, Y + 214.f, Parchment, 1.2f, GetMenuFont());
	DrawCentered(TEXT("Continue?"), Canvas->ClipX * 0.5f, Y + 256.f, Gold, 1.5f, GetMenuFont());
	DrawMenuButtons((Canvas->ClipX - 360.f) * 0.5f, Y + 310.f, 360.f);
}

void AArenaHUD::TickInitials()
{
	// Arcade entry: W/S change the letter, A/D move, Enter saves.
	auto Pressed = [this](const FKey& A, const FKey& B) { return PlayerOwner->WasInputKeyJustPressed(A) || PlayerOwner->WasInputKeyJustPressed(B); };
	TCHAR& Letter = Initials[InitialsCursor];
	if (Pressed(EKeys::W, EKeys::Up))
	{
		Letter = Letter == 'Z' ? 'A' : Letter + 1;
	}
	if (Pressed(EKeys::S, EKeys::Down))
	{
		Letter = Letter == 'A' ? 'Z' : Letter - 1;
	}
	if (Pressed(EKeys::D, EKeys::Right))
	{
		InitialsCursor = FMath::Min(InitialsCursor + 1, 2);
	}
	if (Pressed(EKeys::A, EKeys::Left))
	{
		InitialsCursor = FMath::Max(InitialsCursor - 1, 0);
	}
	if (Pressed(EKeys::Enter, EKeys::SpaceBar))
	{
		const AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();
		const FString Name(3, Initials);
		Board->Add(Name, GameMode->GetScore(), GameMode->GetRoundsSurvived());
		Board->Save();
		NewEntryRank = Board->Entries.IndexOfByPredicate([&](const FLeaderboardEntry& Entry)
		{
			return Entry.Initials == Name && Entry.Score == GameMode->GetScore();
		});
		ShowScreen(EScreen::Leaderboard);
	}
}

void AArenaHUD::DrawInitials()
{
	constexpr float Width = 520.f;
	constexpr float Height = 330.f;
	const float X = (Canvas->ClipX - Width) * 0.5f;
	const float Y = (Canvas->ClipY - Height) * 0.5f;
	DrawPanel(X, Y, Width, Height, TEXT("NEW HIGH SCORE!"));

	constexpr float Box = 64.f;
	const float LettersX = (Canvas->ClipX - 3.f * (Box + 16.f) + 16.f) * 0.5f;
	const float Blink = 0.6f + 0.4f * FMath::Sin(GetWorld()->GetRealTimeSeconds() * 8.f);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const float BoxX = LettersX + Index * (Box + 16.f);
		const bool bActive = Index == InitialsCursor;
		DrawBox(BoxX, Y + 110.f, Box, Box, BarBacking, bActive ? Gold * Blink : Ink, 3.f);
		DrawCentered(FString(1, &Initials[Index]), BoxX + Box * 0.5f, Y + 118.f, Gold, 2.f, GetMenuFont());
	}
	DrawCentered(TEXT("W / S  letter     A / D  move     Enter  save"), Canvas->ClipX * 0.5f, Y + 210.f, Parchment, 1.f, GetMenuFont());
}

void AArenaHUD::DrawLeaderboard()
{
	MenuButtons.Reset();
	MenuButtons.Add({ TEXT("Back"), FBox2D(), [this]() { ShowScreen(EScreen::GameOver); } });

	constexpr float Width = 520.f;
	constexpr float Row = 30.f;
	const float Height = 120.f + UArenaLeaderboard::MaxEntries * Row + 100.f;
	const float X = (Canvas->ClipX - Width) * 0.5f;
	const float Y = (Canvas->ClipY - Height) * 0.5f;
	DrawPanel(X, Y, Width, Height, TEXT("LEADERBOARD"));

	if (!Board)
	{
		Board = UArenaLeaderboard::Load();
	}
	for (int32 Rank = 0; Rank < UArenaLeaderboard::MaxEntries; ++Rank)
	{
		const float RowY = Y + 104.f + Rank * Row;
		const FLinearColor Color = Rank == NewEntryRank ? Gold : Parchment;
		DrawText(FString::Printf(TEXT("%2d."), Rank + 1), Color, X + 50.f, RowY, GetMenuFont(), 1.1f);
		if (Board->Entries.IsValidIndex(Rank))
		{
			const FLeaderboardEntry& Entry = Board->Entries[Rank];
			DrawText(Entry.Initials, Color, X + 110.f, RowY, GetMenuFont(), 1.1f);
			DrawText(FText::AsNumber(Entry.Score).ToString(), Color, X + 210.f, RowY, GetMenuFont(), 1.1f);
			DrawText(FString::Printf(TEXT("wave %d"), Entry.Wave), Color, X + 370.f, RowY, GetMenuFont(), 1.1f);
		}
		else
		{
			DrawText(TEXT("---"), Color * 0.6f, X + 110.f, RowY, GetMenuFont(), 1.1f);
		}
	}
	DrawMenuButtons((Canvas->ClipX - 240.f) * 0.5f, Y + Height - 80.f, 240.f);
}

void AArenaHUD::DrawCredits()
{
	MenuButtons.Reset();
	MenuButtons.Add({ TEXT("Back"), FBox2D(), [this]() { ShowScreen(CreditsReturnTo); } });

	constexpr float Row = 34.f;
	const float Width = FMath::Min(900.f, Canvas->ClipX - 40.f);
	const float Height = 120.f + UE_ARRAY_COUNT(CreditLines) * Row + 100.f;
	const float X = (Canvas->ClipX - Width) * 0.5f;
	const float Y = (Canvas->ClipY - Height) * 0.5f;
	DrawPanel(X, Y, Width, Height, TEXT("CREDITS"));

	float RowY = Y + 104.f;
	for (const TCHAR* Line : CreditLines)
	{
		DrawCentered(Line, Canvas->ClipX * 0.5f, RowY, Parchment, 1.f, GetMenuFont());
		RowY += Row;
	}
	DrawMenuButtons((Canvas->ClipX - 240.f) * 0.5f, Y + Height - 80.f, 240.f);
}

// ---------------------------------------------------------------- story

UTexture2D* AArenaHUD::StoryTexture(FName ArtName) const
{
	const AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();
	const UPaperSprite* Sprite = GameMode ? GameMode->GetStoryArt(ArtName) : nullptr;
	return Sprite ? Sprite->GetBakedTexture() : nullptr;
}

void AArenaHUD::DrawShadowed(const FString& Text, float CenterX, float Y, const FLinearColor& Color, float Scale)
{
	DrawCentered(Text, CenterX + 2.f, Y + 2.f, FLinearColor(0.f, 0.f, 0.f, Color.A * 0.85f), Scale, GetMenuFont());
	DrawCentered(Text, CenterX, Y, Color, Scale, GetMenuFont());
}

TArray<FString> AArenaHUD::WrapText(const FString& Text, float MaxWidth, UFont* Font, float Scale) const
{
	TArray<FString> Words;
	Text.ParseIntoArray(Words, TEXT(" "));

	TArray<FString> Rows;
	FString Row;
	for (const FString& Word : Words)
	{
		const FString Longer = Row.IsEmpty() ? Word : Row + TEXT(" ") + Word;
		float Width, Height;
		GetTextSize(Longer, Width, Height, Font, Scale);
		if (Width > MaxWidth && !Row.IsEmpty())
		{
			Rows.Add(Row);
			Row = Word;
		}
		else
		{
			Row = Longer;
		}
	}
	if (!Row.IsEmpty())
	{
		Rows.Add(Row);
	}
	return Rows;
}

float AArenaHUD::DrawWrapped(const FString& Text, float X, float Y, float MaxWidth, const FLinearColor& Color, float Scale, int32 MaxChars)
{
	// Wrap the whole text first, then reveal characters, so words never jump rows mid-typing.
	UFont* Font = GetMenuFont();
	float Width, RowHeight;
	GetTextSize(TEXT("Ag"), Width, RowHeight, Font, Scale);
	RowHeight += 6.f;

	const TArray<FString> Rows = WrapText(Text, MaxWidth, Font, Scale);
	for (int32 Index = 0; Index < Rows.Num() && MaxChars > 0; ++Index)
	{
		DrawText(Rows[Index].Left(MaxChars), Color, X, Y + Index * RowHeight, Font, Scale);
		MaxChars -= Rows[Index].Len() + 1;
	}
	return Rows.Num() * RowHeight;
}

void AArenaHUD::DrawStory(const AStoryScene& Scene)
{
	// What to do next, top-centre.
	if (!Scene.GetObjective().IsEmpty() && !Scene.IsDialogOpen() && !Scene.GetPhone().bShown)
	{
		DrawShadowed(Scene.GetObjective(), Canvas->ClipX * 0.5f, 26.f, Gold, 1.3f);
	}

	// A prompt over whatever she can use right now.
	if (const AStoryProp* Prop = Scene.GetFocusProp())
	{
		const FVector Screen = Project(Prop->GetActorLocation() + FVector(0.0, 0.0, Prop->GetHalfSize().Y + 14.0));
		DrawShadowed(FString::Printf(TEXT("[E]  %s"), *Prop->Prompt), Screen.X, Screen.Y - 30.f, FLinearColor::White, 1.1f);
	}

	// Comic callouts ("WHAM!") rise and fade.
	const double Now = GetWorld()->GetTimeSeconds();
	for (const FStoryPopup& Popup : Scene.GetPopups())
	{
		const float Age = float(Now - Popup.Time);
		const FVector Screen = Project(Popup.Where + FVector(0.0, 0.0, 90.0 + Age * 90.0));
		FLinearColor Color = WaveYellow;
		Color.A = FMath::Clamp(1.4f - Age, 0.f, 1.f);
		DrawShadowed(Popup.Text, Screen.X, Screen.Y, Color, 2.4f);
	}

	// The phone eases up from the bottom of the screen and back down.
	PhoneSlide = FMath::FInterpTo(PhoneSlide, Scene.GetPhone().bShown ? 1.f : 0.f, GetWorld()->GetDeltaSeconds(), 7.f);
	if (PhoneSlide > 0.01f)
	{
		DrawPhone(Scene.GetPhone());
	}
	if (Scene.IsDialogOpen())
	{
		DrawDialog(Scene);
	}
}

void AArenaHUD::DrawDialog(const AStoryScene& Scene)
{
	const float Width = FMath::Min(940.f, Canvas->ClipX - 60.f);
	constexpr float Height = 176.f;
	const float X = (Canvas->ClipX - Width) * 0.5f;
	const float Y = Canvas->ClipY - Height - 24.f;
	if (PanelTexture)
	{
		DrawNineSlice(PanelTexture, PanelCorner, X, Y, Width, Height, FLinearColor::White);
	}
	else
	{
		DrawBox(X, Y, Width, Height, Wood, WoodEdge, 5.f);
	}

	// Portrait on the left, scaled by a whole number so the pixels stay crisp.
	float TextX = X + 34.f;
	const UPaperSprite* Portrait = Scene.GetPortrait();
	if (UTexture2D* Texture = Portrait ? Portrait->GetBakedTexture() : nullptr)
	{
		constexpr float Box = 120.f;
		const float Scale = FMath::Max(1.f, FMath::FloorToFloat(FMath::Min(Box / Texture->GetSizeX(), Box / Texture->GetSizeY())));
		const float W = Texture->GetSizeX() * Scale;
		const float H = Texture->GetSizeY() * Scale;
		DrawRect(BarBacking, X + 26.f, Y + 26.f, Box, Box);
		DrawTexture(Texture, X + 26.f + (Box - W) * 0.5f, Y + 26.f + (Box - H), W, H, 0.f, 0.f, 1.f, 1.f, FLinearColor::White, BLEND_Translucent);
		TextX = X + 26.f + Box + 22.f;
	}

	const FStoryLine& Line = Scene.GetLine();
	DrawText(Line.Speaker, Gold, TextX, Y + 24.f, GetMenuFont(), 1.25f);
	DrawRect(Gold * 0.6f, TextX, Y + 56.f, Width - (TextX - X) - 34.f, 2.f);
	DrawWrapped(Line.Text, TextX, Y + 68.f, Width - (TextX - X) - 34.f, Parchment, 1.1f, Scene.GetCharsShown());

	if (Scene.GetCharsShown() >= Line.Text.Len())
	{
		const float Blink = 0.5f + 0.5f * FMath::Sin(GetWorld()->GetRealTimeSeconds() * 6.f);
		DrawText(TEXT("Space"), Gold * Blink, X + Width - 96.f, Y + Height - 40.f, GEngine->GetSmallFont(), 1.2f);
	}
}

void AArenaHUD::DrawPhone(const FPhoneView& Phone)
{
	// Phone_UI.png is 84 x 150: a 5 px bezel, a header bar from row 12 to 24, screen to row 134.
	UTexture2D* Frame = StoryTexture(TEXT("Phone_UI"));
	if (!Frame)
	{
		return;
	}
	const float Scale = FMath::Min(3.8f, (Canvas->ClipY - 30.f) / 150.f);
	const float Width = 84.f * Scale;
	const float Height = 150.f * Scale;
	const float Eased = FMath::InterpEaseOut(0.f, 1.f, PhoneSlide, 2.f);
	const float X = Canvas->ClipX * 0.66f - Width * 0.5f;
	const float Y = FMath::Lerp(Canvas->ClipY + 10.f, Canvas->ClipY - Height - 14.f, Eased);
	DrawTexture(Frame, X, Y, Width, Height, 0.f, 0.f, 1.f, 1.f, FLinearColor::White, BLEND_Translucent);

	const float ScreenX = X + 5.f * Scale;
	const float ScreenW = 74.f * Scale;
	const float HeaderY = Y + 12.f * Scale;
	const float HeaderH = 12.f * Scale;
	if (UTexture2D* Pfp = StoryTexture(TEXT("Carl_Pfp")))
	{
		DrawTexture(Pfp, ScreenX + 6.f, HeaderY + 3.f, HeaderH - 6.f, HeaderH - 6.f, 0.f, 0.f, 1.f, 1.f, FLinearColor::White, BLEND_Translucent);
	}
	DrawText(Phone.Sender, FLinearColor::White, ScreenX + HeaderH + 8.f, HeaderY + HeaderH * 0.5f - 10.f, GetMenuFont(), 1.f);

	// The message bubble, with a big profile picture above it so the "catch" is readable.
	float BubbleY = HeaderY + HeaderH + 14.f;
	if (UTexture2D* Pfp = StoryTexture(TEXT("Carl_Pfp")))
	{
		constexpr float Big = 96.f;
		DrawTexture(Pfp, ScreenX + (ScreenW - Big) * 0.5f, BubbleY, Big, Big, 0.f, 0.f, 1.f, 1.f, FLinearColor::White, BLEND_Translucent);
		BubbleY += Big + 12.f;
	}
	const float BubbleX = ScreenX + 10.f;
	const float BubbleW = ScreenW - 20.f;
	const int32 Rows = WrapText(Phone.Message, BubbleW - 20.f, GetMenuFont(), 0.9f).Num();
	DrawRect(FLinearColor(0.75f, 0.8f, 0.9f), BubbleX, BubbleY, BubbleW, Rows * 26.f + 20.f);
	DrawWrapped(Phone.Message, BubbleX + 10.f, BubbleY + 10.f, BubbleW - 20.f, FLinearColor(0.08f, 0.08f, 0.12f), 0.9f, FMath::FloorToInt32(Phone.CharsShown));

	if (Phone.bBlocked)
	{
		const float BandY = Y + Height * 0.5f - 30.f;
		DrawRect(FLinearColor(0.8f, 0.08f, 0.12f, 0.95f), ScreenX, BandY, ScreenW, 60.f);
		DrawCentered(TEXT("BLOCKED"), ScreenX + ScreenW * 0.5f, BandY + 12.f, FLinearColor::White, 1.6f, GetMenuFont());
	}
}

// ---------------------------------------------------------------- world map

void AArenaHUD::TravelToMapNode(int32 Index)
{
	UArenaSession* Session = UArenaSession::Get(this);
	if (!Session || Index > Session->HighestUnlocked)
	{
		return;
	}
	UArenaSession::PlaySfx(this, GetWorld()->GetAuthGameMode<AArenaGameMode>()->GetStorySound(TEXT("Select")));
	// Stop 0 is the home village; every other stop is a combat stage.
	Session->Travel(this, Index == 0 ? EArenaScene::Town : EArenaScene::Stage, Index);
}

void AArenaHUD::TickWorldMap()
{
	const UArenaSession* Session = UArenaSession::Get(this);
	const AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();
	if (!Session || !GameMode)
	{
		return;
	}
	auto Pressed = [this](const FKey& A, const FKey& B) { return PlayerOwner->WasInputKeyJustPressed(A) || PlayerOwner->WasInputKeyJustPressed(B); };
	const int32 Last = FMath::Min(Session->HighestUnlocked, GameMode->GetRoute().Num() - 1);
	if (Pressed(EKeys::S, EKeys::D) || Pressed(EKeys::Down, EKeys::Right))
	{
		MapSelection = FMath::Min(MapSelection + 1, Last);
	}
	if (Pressed(EKeys::W, EKeys::A) || Pressed(EKeys::Up, EKeys::Left))
	{
		MapSelection = FMath::Max(MapSelection - 1, 0);
	}
	if (Pressed(EKeys::Enter, EKeys::SpaceBar))
	{
		TravelToMapNode(MapSelection);
	}
}

void AArenaHUD::DrawWorldMap()
{
	const UArenaSession* Session = UArenaSession::Get(this);
	const AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();
	if (!Session || !GameMode)
	{
		return;
	}
	const TArray<FStageDef>& Route = GameMode->GetRoute();
	const float Time = GetWorld()->GetRealTimeSeconds();

	if (BackdropTexture)
	{
		const float Tile = BackdropTexture->GetSizeX() * UiScale;
		DrawTexture(BackdropTexture, 0.f, 0.f, Canvas->ClipX, Canvas->ClipY, 0.f, 0.f, Canvas->ClipX / Tile, Canvas->ClipY / Tile, FLinearColor::White, BLEND_Opaque);
	}

	// The map fills the height, leaving room for the info panel on the right.
	const float MapH = Canvas->ClipY - 40.f;
	const float MapW = MapH * MapAspect;
	const float MapX = (Canvas->ClipX - MapW) * 0.5f - 170.f;
	const float MapY = 20.f;
	if (UTexture2D* Map = StoryTexture(TEXT("World_Map")))
	{
		DrawTexture(Map, MapX, MapY, MapW, MapH, 0.f, 0.f, 1.f, 1.f, FLinearColor::White, BLEND_Translucent);
	}
	auto NodeAt = [&](int32 Index) { return FVector2D(MapX + Route[Index].MapPosition.X * MapW, MapY + Route[Index].MapPosition.Y * MapH); };

	// The road: a dotted line from stop to stop, faint where she hasn't been.
	for (int32 Index = 1; Index < Route.Num(); ++Index)
	{
		const FVector2D From = NodeAt(Index - 1), To = NodeAt(Index);
		const FLinearColor Dot = Index <= Session->HighestUnlocked ? FLinearColor(0.35f, 0.2f, 0.1f) : FLinearColor(0.35f, 0.2f, 0.1f, 0.25f);
		for (int32 Step = 1; Step < 12; ++Step)
		{
			const FVector2D P = FMath::Lerp(From, To, Step / 12.f);
			DrawRect(Dot, P.X - 3.f, P.Y - 3.f, 6.f, 6.f);
		}
	}

	MenuButtons.Reset();
	UTexture2D* Open = StoryTexture(TEXT("Map_Node"));
	UTexture2D* Locked = StoryTexture(TEXT("Map_Node_Locked"));
	constexpr float NodeSize = 44.f;
	for (int32 Index = 0; Index < Route.Num(); ++Index)
	{
		const FVector2D P = NodeAt(Index);
		const bool bOpen = Index <= Session->HighestUnlocked;
		if (UTexture2D* Node = bOpen ? Open : Locked)
		{
			DrawTexture(Node, P.X - NodeSize * 0.5f, P.Y - NodeSize * 0.5f, NodeSize, NodeSize, 0.f, 0.f, 1.f, 1.f, FLinearColor::White, BLEND_Translucent);
		}
		if (bOpen)
		{
			DrawShadowed(Route[Index].Name.ToString(), P.X, P.Y + NodeSize * 0.5f + 2.f, FLinearColor::White, 1.f);
			// Invisible buttons: clicking a stop selects it, clicking again travels.
			MenuButtons.Add({ FString(), FBox2D(P - FVector2D(NodeSize), P + FVector2D(NodeSize)), [this, Index]()
			{
				if (MapSelection == Index)
				{
					TravelToMapNode(Index);
				}
				MapSelection = Index;
			} });
		}
	}

	// Fog hides everything she hasn't reached, and hangs along the map's borders.
	if (UTexture2D* Fog = StoryTexture(TEXT("Map_Fog")))
	{
		auto Puff = [&](float X, float Y, float Size, float Phase)
		{
			const float Drift = FMath::Sin(Time * 0.5f + Phase) * 10.f;
			DrawTexture(Fog, X - Size * 0.5f + Drift, Y - Size * 0.5f, Size, Size, 0.f, 0.f, 1.f, 1.f, FLinearColor(1.f, 1.f, 1.f, 0.96f), BLEND_Translucent);
		};
		for (int32 Index = Session->HighestUnlocked + 1; Index < Route.Num(); ++Index)
		{
			const FVector2D P = NodeAt(Index);
			Puff(P.X - 70.f, P.Y, 230.f, Index);
			Puff(P.X + 70.f, P.Y - 10.f, 230.f, Index + 2.f);
			Puff(P.X, P.Y + 20.f, 260.f, Index + 4.f);
		}
		for (float Y = MapY; Y < MapY + MapH; Y += 110.f)
		{
			Puff(MapX + 8.f, Y + 40.f, 150.f, Y);
			Puff(MapX + MapW - 8.f, Y + 70.f, 150.f, Y + 1.f);
		}
	}

	// Nikki stands on the selected stop.
	const AArenaPlayerPawn* Hero = Cast<AArenaPlayerPawn>(GetOwningPawn());
	const UPaperSprite* Portrait = Hero ? Hero->GetPortrait() : nullptr;
	if (UTexture2D* Marker = Portrait ? Portrait->GetBakedTexture() : nullptr)
	{
		const FVector2D P = NodeAt(FMath::Clamp(MapSelection, 0, Route.Num() - 1));
		const float Bob = FMath::Abs(FMath::Sin(Time * 4.f)) * 10.f;
		const float W = Marker->GetSizeX() * 4.f, H = Marker->GetSizeY() * 4.f;
		DrawTexture(Marker, P.X - W * 0.5f, P.Y - H - Bob + 8.f, W, H, 0.f, 0.f, 1.f, 1.f, FLinearColor::White, BLEND_Translucent);
	}

	// Info panel for the selected stop.
	if (Route.IsValidIndex(MapSelection))
	{
		const FStageDef& Stop = Route[MapSelection];
		constexpr float PanelW = 360.f;
		constexpr float PanelH = 300.f;
		const float PanelX = MapX + MapW + 30.f;
		const float PanelY = Canvas->ClipY * 0.5f - PanelH * 0.5f;
		if (PanelTexture)
		{
			DrawNineSlice(PanelTexture, PanelCorner, PanelX, PanelY, PanelW, PanelH, FLinearColor::White);
		}
		const float Center = PanelX + PanelW * 0.5f;
		DrawCentered(Stop.Name.ToString(), Center, PanelY + 30.f, Gold, 1.6f, GetMenuFont());
		DrawCentered(MapSelection == 0 ? TEXT("Home village") : *FString::Printf(TEXT("%d waves"), Stop.Waves), Center, PanelY + 84.f, Parchment, 1.1f, GetMenuFont());
		if (Stop.bPlaceholder)
		{
			DrawCentered(TEXT("(work in progress)"), Center, PanelY + 116.f, Parchment * 0.7f, 1.f, GetMenuFont());
		}
		DrawCentered(FString::Printf(TEXT("Gold  %s"), *FText::AsNumber(Session->Gold).ToString()), Center, PanelY + 164.f, WaveYellow, 1.3f, GetMenuFont());
		DrawCentered(TEXT("W / S  choose"), Center, PanelY + 220.f, Parchment, 1.f, GetMenuFont());
		DrawCentered(TEXT("Enter  travel"), Center, PanelY + 250.f, Parchment, 1.f, GetMenuFont());
	}
}

void AArenaHUD::DrawStageClear()
{
	const UArenaSession* Session = UArenaSession::Get(this);
	const AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();
	if (!Session || !GameMode)
	{
		return;
	}
	MenuButtons.Reset();
	MenuButtons.Add({ TEXT("World map"), FBox2D(), [this]() { UArenaSession::Get(this)->Travel(this, EArenaScene::WorldMap); } });

	constexpr float Width = 520.f;
	constexpr float Height = 330.f;
	const float X = (Canvas->ClipX - Width) * 0.5f;
	const float Y = (Canvas->ClipY - Height) * 0.5f;
	DrawPanel(X, Y, Width, Height, TEXT("STAGE CLEAR!"));
	DrawCentered(FString::Printf(TEXT("Score %s  becomes gold"), *FText::AsNumber(GameMode->GetScore()).ToString()), Canvas->ClipX * 0.5f, Y + 110.f, Parchment, 1.2f, GetMenuFont());
	DrawCentered(FString::Printf(TEXT("Gold  %s"), *FText::AsNumber(Session->Gold).ToString()), Canvas->ClipX * 0.5f, Y + 152.f, WaveYellow, 1.5f, GetMenuFont());
	DrawCentered(TEXT("The road ahead is open."), Canvas->ClipX * 0.5f, Y + 204.f, Parchment, 1.f, GetMenuFont());
	DrawMenuButtons((Canvas->ClipX - 300.f) * 0.5f, Y + Height - 84.f, 300.f);
}

// ---------------------------------------------------------------- shop

void AArenaHUD::OpenShop()
{
	ShowScreen(EScreen::Shop);
}

void AArenaHUD::DrawShop()
{
	UArenaSession* Session = UArenaSession::Get(this);
	AArenaPlayerPawn* Hero = Cast<AArenaPlayerPawn>(GetOwningPawn());
	AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();
	if (!Session || !Hero || !GameMode)
	{
		return;
	}

	MenuButtons.Reset();
	for (const FShopItem& Item : ArenaShop::Items())
	{
		const int32 Owned = Session->Purchases.FindRef(Item.Id);
		const FString Label = FString::Printf(TEXT("%s   -   %s   -   %s gold   (%d / %d)"),
			*Item.Name, *Item.Description, *FText::AsNumber(Item.Cost).ToString(), Owned, Item.MaxCount);
		MenuButtons.Add({ Label, FBox2D(), [this, Session, Hero, GameMode, &Item]()
		{
			if (!ArenaShop::TryBuy(*Session, Item))
			{
				return;
			}
			UArenaSession::PlaySfx(this, GameMode->GetStorySound(TEXT("Coin")));
			Hero->RefreshStats();
			// Some purchases are story moments: close the shop and let her speak.
			if (!Item.BoughtSection.IsEmpty() && GameMode->GetScene())
			{
				ShowScreen(EScreen::Playing);
				GameMode->GetScene()->Say(Item.BoughtSection);
			}
		}, nullptr, !ArenaShop::CanBuy(*Session, Item) });
	}
	MenuButtons.Add({ TEXT("Leave  (Esc)"), FBox2D(), [this]() { ShowScreen(EScreen::Playing); } });

	const float Width = FMath::Min(1000.f, Canvas->ClipX - 40.f);
	const float Height = 190.f + MenuButtons.Num() * 70.f;
	const float X = (Canvas->ClipX - Width) * 0.5f;
	const float Y = (Canvas->ClipY - Height) * 0.5f;
	DrawPanel(X, Y, Width, Height, TEXT("SHOP"));

	const FArenaStats& Stats = Hero->GetStats();
	DrawCentered(FString::Printf(TEXT("Gold  %s"), *FText::AsNumber(Session->Gold).ToString()), Canvas->ClipX * 0.5f, Y + 96.f, WaveYellow, 1.4f, GetMenuFont());
	DrawCentered(FString::Printf(TEXT("HP %.0f    AD %.0f    AP %.0f    Armor %.0f    MR %.0f    MS %.0f"),
		Stats.MaxHealth, Stats.AttackDamage, Stats.AbilityPower, Stats.Armor, Stats.MagicResist, Stats.MoveSpeed),
		Canvas->ClipX * 0.5f, Y + 136.f, Parchment, 1.f, GetMenuFont());
	DrawMenuButtons(X + 40.f, Y + 176.f, Width - 80.f);
}

// ---------------------------------------------------------------- debug

void AArenaHUD::TickDebug()
{
	UArenaSession* Session = UArenaSession::Get(this);
	AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();
	if (!Session || !GameMode)
	{
		return;
	}
	if (PlayerOwner->WasInputKeyJustPressed(EKeys::F1))
	{
		Session->bDebug = !Session->bDebug;
	}
	if (!Session->bDebug)
	{
		return;
	}

	AArenaPlayerPawn* Hero = Cast<AArenaPlayerPawn>(GetOwningPawn());
	if (Hero && PlayerOwner->WasInputKeyJustPressed(EKeys::F2))
	{
		Hero->AddMana(1);
	}
	if (PlayerOwner->WasInputKeyJustPressed(EKeys::F3))
	{
		Session->Gold += 5000;
	}
	if (PlayerOwner->WasInputKeyJustPressed(EKeys::F4))
	{
		GameMode->KillAllEnemies();
	}
	if (PlayerOwner->WasInputKeyJustPressed(EKeys::F5))
	{
		// Everything open: all stages, the guitar, the contract.
		Session->HighestUnlocked = GameMode->GetRoute().Num() - 1;
		Session->Purchases.FindOrAdd(ArenaShop::Guitar) = 1;
		Session->bContractSigned = Session->bIntroSeen = Session->bDressed = true;
		if (Hero)
		{
			Hero->RefreshStats();
		}
	}
	if (PlayerOwner->WasInputKeyJustPressed(EKeys::F6))
	{
		Session->Travel(this, EArenaScene::WorldMap);
	}
}

void AArenaHUD::DrawCentered(const FString& Text, float CenterX, float Y, const FLinearColor& Color, float Scale, UFont* Font)
{
	Font = Font ? Font : GEngine->GetLargeFont();
	float W, H;
	GetTextSize(Text, W, H, Font, Scale);
	DrawText(Text, Color, CenterX - W * 0.5f, Y, Font, Scale);
}

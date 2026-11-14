#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ArenaHUD.generated.h"

class AArenaEnemy;
class AArenaGameMode;
class UArenaLeaderboard;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class AArenaPlayerPawn;
class UFont;
class UHealthComponent;
class UTexture2D;

// All on-screen UI, drawn straight to the canvas each frame (see DECISIONS.md 007):
// the start screens, the in-game HUD, the pause menu, and the death screens.
UCLASS()
class PROJECT_RIOT_API AArenaHUD : public AHUD
{
	GENERATED_BODY()

public:
	AArenaHUD();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void DrawHUD() override;

private:
	struct FBarState
	{
		float Shown = -1.f;  // eases toward the real health
		float Trail = -1.f;  // the pale "just lost" chip, which drains slower
	};

	enum class EScreen : uint8
	{
		ChooseHero,  // first launch: pick a hero
		Controls,    // first launch: how to play, until the first WASD press
		Playing,
		Paused,
		GameOver,     // "YOU DIED. Continue?"
		Initials,     // arcade initials entry for a top-10 score
		Leaderboard,
		Credits,
	};

	struct FMenuButton
	{
		FString Label;
		FBox2D Rect;
		TFunction<void()> Action;
		UTexture2D* Icon = nullptr;
	};

	struct FMenuSlider
	{
		FString Label;
		FBox2D Track;
		TFunction<float()> Get;
		TFunction<void(float)> Set;
	};

	void DrawHealthBar(const UHealthComponent& Health, float X, float Y, float Width, float Height);
	void ShowScreen(EScreen NewScreen);
	void DrawHeroSelect();
	void DrawControls();
	void DrawPauseMenu();
	void DrawGameOver();
	void DrawInitials();
	void DrawLeaderboard();
	void DrawCredits();
	void TickInitials();
	void DrawMenuButtons(float X, float Y, float Width);
	void DrawScore(const AArenaGameMode& GameMode);
	void DrawPopups(const AArenaGameMode& GameMode);
	void DrawEnemyIcon(const AArenaGameMode& GameMode, TSubclassOf<AArenaEnemy> Type, float X, float Y, float Size);
	void DrawPanel(float X, float Y, float Width, float Height, const FString& Title);
	void DrawButton(FMenuButton& Button, const FVector2D& Mouse);
	void DrawSlider(FMenuSlider& Slider, float X, float Y, float Width);
	void ClickMenu(const FVector2D& Mouse);
	void DragSliders(const FVector2D& Mouse);
	FVector2D GetMouse() const;
	UFont* GetMenuFont() const;

	void DrawHealthBars();
	void DrawWaveBanner(const AArenaGameMode& GameMode);
	void DrawSpecialsBar(const AArenaPlayerPawn& Hero);
	void DrawStunStars(const AArenaPlayerPawn& Hero);
	void DrawBox(float X, float Y, float Width, float Height, const FLinearColor& Fill, const FLinearColor& Border, float BorderWidth);
	// Draws a texture stretched to a box with its corners kept crisp (9-slice).
	void DrawNineSlice(UTexture2D* Texture, float CornerPixels, float X, float Y, float Width, float Height, const FLinearColor& Tint);
	void DrawCentered(const FString& Text, float CenterX, float Y, const FLinearColor& Color, float Scale, UFont* Font = nullptr);

	// Menu art, drawn as 9-slices (corners stay crisp when stretched). Plain boxes if empty.
	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	TObjectPtr<UTexture2D> PanelTexture;

	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	TObjectPtr<UTexture2D> ButtonTexture;

	// Tiled behind the start screens so the stage stays hidden until play begins.
	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	TObjectPtr<UTexture2D> BackdropTexture;

	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	TObjectPtr<UFont> MenuFont;

	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	TObjectPtr<UTexture2D> SkullTexture;

	// Draws wave-banner icons with the theme's palette swap (M_UIIcon).
	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	TObjectPtr<UMaterialInterface> IconMaterial;

	UPROPERTY()
	TMap<TSubclassOf<AArenaEnemy>, TObjectPtr<UMaterialInstanceDynamic>> IconMaterials;

	UPROPERTY()
	TObjectPtr<UArenaLeaderboard> Board;

	TMap<TWeakObjectPtr<const UHealthComponent>, FBarState> Bars;
	TArray<FMenuButton> MenuButtons;
	TArray<FMenuSlider> MenuSliders;
	EScreen CurrentScreen = EScreen::Playing;
	EScreen CreditsReturnTo = EScreen::Paused;
	double DeathNoticedAt = -1.0;
	double FadeInFrom = -1.0;
	int32 NewEntryRank = INDEX_NONE;
	TCHAR Initials[3] = { 'A', 'A', 'A' };
	int32 InitialsCursor = 0;
};

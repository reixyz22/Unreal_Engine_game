#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ArenaHUD.generated.h"

class AArenaGameMode;
class AArenaPlayerPawn;
class UFont;
class UHealthComponent;
class UTexture2D;

// All on-screen UI, drawn straight to the canvas each frame (see DECISIONS.md 007):
// the start screens, the in-game HUD, and the pause menu.
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
	void DrawDeathBanner(const AArenaGameMode& GameMode);
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

	TMap<TWeakObjectPtr<const UHealthComponent>, FBarState> Bars;
	TArray<FMenuButton> MenuButtons;
	TArray<FMenuSlider> MenuSliders;
	EScreen CurrentScreen = EScreen::Playing;
};

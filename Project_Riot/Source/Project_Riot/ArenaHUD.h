#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ArenaHUD.generated.h"

class AArenaGameMode;
class AArenaPlayerPawn;
class UFont;
class UHealthComponent;
class UTexture2D;

// All on-screen UI, drawn straight to the canvas each frame (see DECISIONS.md 007).
UCLASS()
class PROJECT_RIOT_API AArenaHUD : public AHUD
{
	GENERATED_BODY()

public:
	AArenaHUD();

	virtual void Tick(float DeltaSeconds) override;
	virtual void DrawHUD() override;

private:
	struct FBarState
	{
		float Shown = -1.f;  // eases toward the real health
		float Trail = -1.f;  // the pale "just lost" chip, which drains slower
	};

	struct FMenuButton
	{
		FString Label;
		FBox2D Rect;
		TFunction<void()> Action;
	};

	void DrawHealthBar(const UHealthComponent& Health, float X, float Y, float Width, float Height);
	void DrawPauseMenu();
	void BuildMenuButtons(AArenaPlayerPawn& Hero);
	void SetMenuOpen(bool bOpen);
	void ClickMenu(const FVector2D& Mouse);

	void DrawHealthBars();
	void DrawWaveBanner(const AArenaGameMode& GameMode);
	void DrawHeroPanel(const AArenaPlayerPawn& Hero);
	void DrawSpecialSlot(const AArenaPlayerPawn& Hero);
	void DrawStunStars(const AArenaPlayerPawn& Hero);
	void DrawDeathBanner(const AArenaGameMode& GameMode);
	void DrawBox(float X, float Y, float Width, float Height, const FLinearColor& Fill, const FLinearColor& Border, float BorderWidth);
	void DrawCentered(const FString& Text, float CenterX, float Y, const FLinearColor& Color, float Scale, UFont* Font = nullptr);

	// Optional art for the pause menu; plain wood and parchment boxes are drawn when empty.
	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	TObjectPtr<UTexture2D> PanelTexture;

	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	TObjectPtr<UTexture2D> ButtonTexture;

	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	TObjectPtr<UFont> MenuFont;

	TMap<TWeakObjectPtr<const UHealthComponent>, FBarState> Bars;
	TArray<FMenuButton> MenuButtons;
	bool bMenuOpen = false;
};

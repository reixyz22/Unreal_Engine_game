#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ArenaHUD.generated.h"

class AArenaGameMode;
class AArenaPlayerPawn;
class UFont;

// All on-screen UI, drawn straight to the canvas each frame (see DECISIONS.md 007).
UCLASS()
class PROJECT_RIOT_API AArenaHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawHealthBars();
	void DrawWaveBanner(const AArenaGameMode& GameMode);
	void DrawHeroPanel(const AArenaPlayerPawn& Hero);
	void DrawDeathBanner(const AArenaGameMode& GameMode);
	void DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& Fill);
	void DrawCentered(const FString& Text, float CenterX, float Y, const FLinearColor& Color, float Scale, UFont* Font = nullptr);
};

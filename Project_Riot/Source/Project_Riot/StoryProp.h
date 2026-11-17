#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StoryProp.generated.h"

class UPaperSprite;
class UPaperSpriteComponent;

// One thing in a story scene: furniture, a house, a sign, a villager.
// A prop with OnUse set can be used by the hero (E, or a right-click swing).
UCLASS()
class PROJECT_RIOT_API AStoryProp : public AActor
{
	GENERATED_BODY()

public:
	AStoryProp();

	virtual void Tick(float DeltaTime) override;

	void SetArt(UPaperSprite* Sprite);
	FVector2D GetHalfSize() const { return HalfSize; }
	bool IsUsable() const { return bool(OnUse); }

	// Short squash-and-stretch, for hits and reactions.
	void Punch();
	// Rattle side to side for a while (the buzzing phone).
	void Wobble(float Seconds);

	// What the HUD shows when the hero is close enough, e.g. "Read the sign".
	FString Prompt;
	// The hero swings her flail at this prop when using it.
	bool bSwing = false;
	// Props sort in front of or behind the hero by position; overlays (arrows, poofs) always draw on top.
	bool bOverlay = false;
	TFunction<void()> OnUse;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPaperSpriteComponent> Sprite;

	FVector2D HalfSize = FVector2D::ZeroVector;
	double PunchUntil = 0.0;
	double WobbleUntil = 0.0;
};

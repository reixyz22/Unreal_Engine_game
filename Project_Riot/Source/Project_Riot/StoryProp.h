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
	// Steps through Frames once and holds the last one.
	void PlayFrames(const TArray<UPaperSprite*>& Frames, float SecondsPerFrame);

	// What the HUD shows when the hero is close enough, e.g. "Read the sign".
	FString Prompt;
	// The hero swings her flail at this prop when using it.
	bool bSwing = false;
	// Props sort in front of or behind the hero by position; overlays (arrows, poofs) always draw on top.
	bool bOverlay = false;
	TFunction<void()> OnUse;
	// Set to make this a mace target: hover shows the sword cursor, right-click throws at it,
	// and this runs when the ball lands.
	TFunction<void()> OnMaceHit;
	// Runs as the mace is thrown at this prop.
	TFunction<void()> OnMaceThrown;
	// Where the ball lands, relative to the prop's center (usually right on it).
	FVector2D MaceAim = FVector2D::ZeroVector;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPaperSpriteComponent> Sprite;

	UPROPERTY()
	TArray<TObjectPtr<UPaperSprite>> Frames;

	FVector2D HalfSize = FVector2D::ZeroVector;
	double FramesStartedAt = 0.0;
	float FrameSeconds = 0.f;
	double PunchUntil = 0.0;
	double WobbleUntil = 0.0;
};

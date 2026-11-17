#pragma once

#include "CoreMinimal.h"
#include "ArenaPlayerPawn.h"
#include "CarlPawn.generated.h"

// Carl, the elf ranger. His auto is a bow: right-click fires an arrow at the cursor with
// no range limit. His specials are on the number keys:
//   1  Ricochet: his next few arrows bounce off enemies and walls. He starts with it.
//   2  Volley: a fan of arrows. Bought at the shop.
UCLASS()
class PROJECT_RIOT_API ACarlPawn : public AArenaPlayerPawn
{
	GENERATED_BODY()

public:
	ACarlPawn();

	virtual bool HasSpecial() const override { return true; }
	virtual FString GetSpecialKey() const override { return TEXT("1"); }
	virtual UPaperSprite* GetSpecialIcon() const override { return ArrowSprite; }
	int32 GetRicochetShotsLeft() const { return RicochetShotsLeft; }

protected:
	virtual void HandleRightClick() override;
	virtual void HandleSpecialKeys(const APlayerController& PlayerController) override;

private:
	void Shoot(const FVector& Direction);

	UPROPERTY(EditAnywhere, Category = "Bow")
	TObjectPtr<UPaperSprite> ArrowSprite;

	UPROPERTY(EditAnywhere, Category = "Bow")
	float ArrowSpeed = 1500.f;

	UPROPERTY(EditAnywhere, Category = "Bow")
	float SecondsBetweenShots = 0.45f;

	// Key 1: how many shots ricochet per cast, and how many times each one bounces.
	UPROPERTY(EditAnywhere, Category = "Specials")
	int32 RicochetShots = 3;

	UPROPERTY(EditAnywhere, Category = "Specials")
	int32 BouncesPerArrow = 3;

	// Key 2: arrows in the fan, and how wide it spreads.
	UPROPERTY(EditAnywhere, Category = "Specials")
	int32 VolleyArrows = 5;

	UPROPERTY(EditAnywhere, Category = "Specials")
	float VolleySpreadDegrees = 40.f;

	int32 RicochetShotsLeft = 0;
	double NextShotAt = 0.0;
};

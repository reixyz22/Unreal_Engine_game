#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArenaBeam.generated.h"

class UPaperSprite;
class UPaperSpriteComponent;

// A Cleric's beam: a thin warning line, then a wide beam that hangs in the air
// and stuns the player on touch. It crosses the whole arena.
UCLASS()
class PROJECT_RIOT_API AArenaBeam : public AActor
{
	GENERATED_BODY()

public:
	AArenaBeam();

	struct FSettings
	{
		float WarningSeconds = 0.8f;
		float ActiveSeconds = 1.f;
		float Width = 44.f;
		float Damage = 15.f;
		float StunSeconds = 0.9f;
	};

	void Fire(UPaperSprite* BeamSprite, const FVector& InStart, const FVector& InDirection, const FSettings& InSettings);
	virtual void Tick(float DeltaTime) override;

private:
	void SetThickness(float Thickness, const FLinearColor& Tint);
	bool IsTouchingPlayer(float PlayerRadius, const FVector& PlayerLocation) const;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPaperSpriteComponent> Sprite;

	FVector Start = FVector::ZeroVector;
	FVector Direction = FVector::XAxisVector;
	FSettings Settings;
	double ActiveAt = 0.0;
	bool bActive = false;
	bool bHitPlayer = false;
};

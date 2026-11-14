#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArenaBeam.generated.h"

class UPaperSprite;
class USoundBase;
class UPaperSpriteComponent;

// A Cleric's beam: a thin warning line, then a wide beam that hangs in the air
// and stuns the player on touch. Both grow outward from the Cleric, so you can
// see where it comes from, then cross the whole arena.
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
		USoundBase* ChargeSound = nullptr;
		USoundBase* FireSound = nullptr;
	};

	void Fire(UPaperSprite* BeamSprite, const FVector& InStart, const FVector& InDirection, const FSettings& InSettings);
	virtual void Tick(float DeltaTime) override;

private:
	void Draw(double InLength, float Thickness, const FLinearColor& Tint);
	bool IsTouchingPlayer(float PlayerRadius, const FVector& PlayerLocation) const;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPaperSpriteComponent> Sprite;

	FVector Start = FVector::ZeroVector;
	FVector Direction = FVector::XAxisVector;
	UPROPERTY()
	TObjectPtr<USoundBase> FireSound;

	FSettings Settings;
	double FiredAt = 0.0;
	double ActiveAt = 0.0;
	double Length = 0.0;
	bool bActive = false;
	bool bHitPlayer = false;
};

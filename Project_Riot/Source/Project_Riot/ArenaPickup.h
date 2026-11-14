#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArenaPickup.generated.h"

class UPaperSprite;
class UPaperSpriteComponent;

enum class EPickupKind : uint8
{
	Mana,
	Health,
};

// A shard on the floor. Walk over it to collect; it blinks, then despawns if ignored.
UCLASS()
class PROJECT_RIOT_API AArenaPickup : public AActor
{
	GENERATED_BODY()

public:
	AArenaPickup();

	void Init(EPickupKind InKind, UPaperSprite* ShardSprite);
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPaperSpriteComponent> Sprite;

	EPickupKind Kind = EPickupKind::Mana;
	FVector Home = FVector::ZeroVector;
};

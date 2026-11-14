#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArenaChainBall.generated.h"

class AArenaEnemy;
class AArenaPlayerPawn;
class UPaperSprite;
class UPaperSpriteComponent;
class USoundBase;

UENUM()
enum class EChainState : uint8
{
	Outbound,
	Returning
};

UCLASS()
class PROJECT_RIOT_API AArenaChainBall : public AActor
{
	GENERATED_BODY()

public:
	AArenaChainBall();

	virtual void Tick(float DeltaTime) override;

	void Throw(AArenaPlayerPawn* InThrower, AArenaEnemy* InTarget, UPaperSprite* InSprite, float InSpeed, float InDamage, USoundBase* InHitSound);

private:
	bool MoveToward(const FVector& Goal, float DeltaTime);
	void HitEnemiesInPath();
	void HitEnemy(AArenaEnemy* Enemy);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPaperSpriteComponent> Sprite;

	UPROPERTY()
	TObjectPtr<USoundBase> HitSound;

	TWeakObjectPtr<AArenaPlayerPawn> Thrower;
	TWeakObjectPtr<AArenaEnemy> Target;
	TSet<TWeakObjectPtr<AArenaEnemy>> HitThisThrow;

	EChainState State = EChainState::Outbound;
	float Speed = 0.f;
	float Damage = 0.f;
};

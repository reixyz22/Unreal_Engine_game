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

struct FChainLook
{
	UPaperSprite* Ball = nullptr;
	UPaperSprite* Link = nullptr;
	USoundBase* HitSound = nullptr;
};

UCLASS()
class PROJECT_RIOT_API AArenaChainBall : public AActor
{
	GENERATED_BODY()

public:
	AArenaChainBall();

	virtual void Tick(float DeltaTime) override;

	void Throw(AArenaPlayerPawn* InThrower, AArenaEnemy* InTarget, const FChainLook& Look, float InSpeed, float InDamage);

private:
	bool MoveToward(const FVector& Goal, float DeltaTime);
	void HitEnemiesInPath();
	void HitEnemy(AArenaEnemy* Enemy);
	void DrawChain(const FVector& Hand);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPaperSpriteComponent> Sprite;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPaperSpriteComponent> Chain;

	UPROPERTY()
	TObjectPtr<USoundBase> HitSound;

	TWeakObjectPtr<AArenaPlayerPawn> Thrower;
	TWeakObjectPtr<AArenaEnemy> Target;
	TSet<TWeakObjectPtr<AArenaEnemy>> HitThisThrow;

	EChainState State = EChainState::Outbound;
	float Speed = 0.f;
	float Damage = 0.f;
};

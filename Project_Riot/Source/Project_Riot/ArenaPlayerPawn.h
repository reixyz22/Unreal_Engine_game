#pragma once

#include "CoreMinimal.h"
#include "ArenaStage.h"
#include "ArenaStats.h"
#include "GameFramework/Pawn.h"
#include "ArenaPlayerPawn.generated.h"

class AArenaEnemy;
class AStoryScene;
class UCameraComponent;
class UHealthComponent;
class UInputAction;
class UInputMappingContext;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPaperFlipbook;
class UPaperFlipbookComponent;
class UPaperSprite;
class UPaperSpriteComponent;
class USoundBase;
class USphereComponent;
struct FInputActionValue;

// The playable hero. This class is Nikki (flail auto, guitar Q); other heroes subclass it
// and override the attack and special hooks (see ACarlPawn).
UCLASS()
class PROJECT_RIOT_API AArenaPlayerPawn : public APawn
{
	GENERATED_BODY()

public:
	AArenaPlayerPawn();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void NotifyControllerChanged() override;

	bool ReceiveDamage(float Amount, EDamageType Type);
	float GetRadius() const;
	FVector GetHandLocation() const;
	void OnChainReturned();

	bool ReceiveStunHit(float Amount, EDamageType Type, float StunSeconds);
	void Heal(float Amount);
	void AddMana(int32 Amount) { Mana = FMath::Min(Mana + Amount, MaxMana); }

	const UHealthComponent& GetHealth() const { return *Health; }
	int32 GetMana() const { return Mana; }
	int32 GetMaxMana() const { return MaxMana; }
	bool IsManaFull() const { return Mana >= MaxMana; }
	bool IsStunned() const;
	bool IsRooted() const;

	bool IsClickToMove() const { return bClickToMove; }
	void SetClickToMove(bool bEnabled);
	bool IsFogEnabled() const { return bFogEnabled; }
	void SetFogEnabled(bool bEnabled);
	virtual UPaperSprite* GetSpecialIcon() const { return GuitarSprite; }
	const FText& GetHeroName() const { return HeroName; }
	UPaperSprite* GetPortrait() const;
	int32 GetSpecialCost() const { return SpecialManaCost; }
	// Nikki's Q needs the guitar, which is bought at the shop.
	virtual bool HasSpecial() const;
	// The key shown on the specials bar.
	virtual FString GetSpecialKey() const { return TEXT("Q"); }
	// Each hero has their own world-map route and their own script file in Content/Story.
	const TArray<FStageDef>& GetRoute() const { return Route; }
	const FString& GetScriptName() const { return ScriptName; }
	const FArenaStats& GetStats() const { return Stats; }
	// Recomputes stats from base + purchases (call after buying something).
	void RefreshStats();
	// Story: her lightning pyjamas, worn until she gets dressed in the intro.
	void SetPyjamas(bool bWear);

protected:
	virtual void BeginPlay() override;

	// Right-click during combat: the hero's auto attack.
	virtual void HandleRightClick();
	// Checked every frame during combat: the hero's special keys.
	virtual void HandleSpecialKeys(const APlayerController& PlayerController);

	void Move(const FInputActionValue& Value);
	void StopMove();
	void UpdateHover();
	void TickStoryScene(AStoryScene& Scene);
	UPaperFlipbook* Outfit(UPaperFlipbook* Normal, UPaperFlipbook* Pyjama) const;
	void UpdateMovement(float DeltaTime);
	FVector2D ComputeMoveDirection();
	void ThrowChainAt(AArenaEnemy* Target);
	bool GetCursorWorldLocation(FVector& OutLocation) const;
	AArenaEnemy* FindEnemyNear(const FVector& Point) const;
	void UpdateAnimation(const FVector2D& Direction);
	void UpdateBlink();
	void UpdateGuitar();
	void CastSpecial();
	void UpdateSpecial();
	void PlayStrike();
	void Die();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPaperFlipbookComponent> Body;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPaperSpriteComponent> HeldFlail;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPaperSpriteComponent> Guitar;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UHealthComponent> Health;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> InputMapping;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	// Her stats before shop upgrades. Stats (below) is what the game uses.
	UPROPERTY(EditAnywhere, Category = "Stats")
	FArenaStats BaseStats;

	// Off by default: WASD plays better. Right-click still attacks; with this on it also walks.
	UPROPERTY(EditAnywhere, Category = "Movement")
	bool bClickToMove = false;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraViewWidth = 1280.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	TObjectPtr<UMaterialInterface> FogMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Look")
	FText HeroName = NSLOCTEXT("Arena", "Nikki", "Nikki");

	UPROPERTY(EditDefaultsOnly, Category = "Story")
	FString ScriptName = TEXT("Nikki");

	UPROPERTY(EditDefaultsOnly, Category = "Story")
	TArray<FStageDef> Route;

	UPROPERTY(EditAnywhere, Category = "Look")
	TObjectPtr<UPaperFlipbook> IdleAnim;

	UPROPERTY(EditAnywhere, Category = "Look")
	TObjectPtr<UPaperFlipbook> RunAnim;

	UPROPERTY(EditAnywhere, Category = "Look")
	TObjectPtr<UPaperFlipbook> StrikeAnim;

	UPROPERTY(EditAnywhere, Category = "Look")
	TObjectPtr<UPaperFlipbook> PyjamaIdle;

	UPROPERTY(EditAnywhere, Category = "Look")
	TObjectPtr<UPaperFlipbook> PyjamaRun;

	UPROPERTY(EditAnywhere, Category = "Look")
	TObjectPtr<UPaperFlipbook> PyjamaStrike;

	UPROPERTY(EditAnywhere, Category = "Look")
	FLinearColor BodyTint = FLinearColor::White;

	// Hardware cursor images, relative to Content/ and without the extension.
	UPROPERTY(EditDefaultsOnly, Category = "Look")
	FName PointerCursor = TEXT("Slate/Cursors/Nikki_Pointer");

	UPROPERTY(EditDefaultsOnly, Category = "Look")
	FName AttackCursor = TEXT("Slate/Cursors/Nikki_Sword");

	UPROPERTY(EditAnywhere, Category = "Auto Attack")
	TObjectPtr<UPaperSprite> FlailHandleSprite;

	UPROPERTY(EditAnywhere, Category = "Auto Attack")
	TObjectPtr<UPaperSprite> ChainBallSprite;

	UPROPERTY(EditAnywhere, Category = "Auto Attack")
	TObjectPtr<UPaperSprite> ChainLinkSprite;

	// Auto damage = Attack Damage x this (physical).
	UPROPERTY(EditAnywhere, Category = "Auto Attack")
	float AutoDamageRatio = 1.f;

	UPROPERTY(EditAnywhere, Category = "Auto Attack")
	float AutoRange = 650.f;

	UPROPERTY(EditAnywhere, Category = "Auto Attack")
	float ChainSpeed = 1100.f;

	UPROPERTY(EditAnywhere, Category = "Auto Attack")
	float ClickRadius = 90.f;

	UPROPERTY(EditAnywhere, Category = "Special")
	TObjectPtr<UPaperSprite> GuitarSprite;

	UPROPERTY(EditAnywhere, Category = "Special")
	TObjectPtr<UPaperSprite> ShockBurstSprite;

	UPROPERTY(EditAnywhere, Category = "Special")
	int32 MaxMana = 1;

	UPROPERTY(EditAnywhere, Category = "Special")
	int32 SpecialManaCost = 1;

	// Q damage = Ability Power x this (magic).
	UPROPERTY(EditAnywhere, Category = "Special")
	float SpecialDamageRatio = 1.f;

	UPROPERTY(EditAnywhere, Category = "Special")
	float SpecialRadius = 1100.f;

	UPROPERTY(EditAnywhere, Category = "Special")
	float SpecialSeconds = 0.5f;

	// She stands still and strums for this long before the burst, and this long after.
	UPROPERTY(EditAnywhere, Category = "Special")
	float SpecialWindUp = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Special")
	float SpecialWindDown = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Sound")
	TObjectPtr<USoundBase> ThrowSound;

	UPROPERTY(EditAnywhere, Category = "Sound")
	TObjectPtr<USoundBase> ChainHitSound;

	UPROPERTY(EditAnywhere, Category = "Sound")
	TObjectPtr<USoundBase> StrumSound;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> Fog;

	FArenaStats Stats;
	bool bPyjamas = false;
	int32 Mana = 0;
	bool bFogEnabled = true;
	double StunnedUntil = 0.0;
	double StrumEndsAt = 0.0;
	double BurstAt = 0.0;
	bool bBurstPending = false;
	FVector2D MoveInput = FVector2D::ZeroVector;
	TOptional<FVector> MoveTarget;
	TWeakObjectPtr<AArenaEnemy> HoveredEnemy;
	TWeakObjectPtr<AArenaEnemy> BufferedAuto;
	bool bChainOut = false;
	double StrikeEndsAt = 0.0;
};

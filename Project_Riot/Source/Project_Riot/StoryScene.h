#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StoryScript.h"
#include "StoryScene.generated.h"

class AArenaPlayerPawn;
class AStoryProp;
class UArenaSession;
class UPaperSprite;

// The phone that slides up over the scene, as the HUD should draw it.
struct FPhoneView
{
	bool bShown = false;
	FString Sender;
	FString Message;
	float CharsShown = 0.f;
	bool bBlocked = false;
};

// Comic text that pops over the world ("WHAM!").
struct FStoryPopup
{
	FString Text;
	FVector Where = FVector::ZeroVector;
	double Time = 0.0;
};

// Base for the non-combat scenes (Nikki's hut, the village). A scene lays out props,
// decides where the hero can walk, runs dialog from the script file, and exposes what
// the HUD needs to draw. Subclasses build their content in StartScene.
UCLASS(Abstract)
class PROJECT_RIOT_API AStoryScene : public AActor
{
	GENERATED_BODY()

public:
	AStoryScene();

	virtual void Tick(float DeltaTime) override;

	// Movement: the hero stays inside Bounds and out of Blockers, sliding along them.
	FVector ResolveMove(const FVector& From, const FVector& To, float Radius) const;
	// True during cutscenes and dialog; the hero ignores input.
	bool BlocksInput() const { return bLockInput || IsDialogOpen(); }
	// The hero can use props but not walk (still in bed, say).
	bool BlocksMovement() const { return BlocksInput() || bHoldStill; }
	// Uses the prop the hero is standing next to. Returns true if she should swing at it.
	bool UseFocusProp();
	// Plays a section of the script as dialog, then runs Then.
	void Say(const FString& Section, TFunction<void()> Then = nullptr);

	// For the HUD.
	bool IsDialogOpen() const { return Lines.IsValidIndex(LineIndex); }
	const FStoryLine& GetLine() const { return Lines[LineIndex]; }
	int32 GetCharsShown() const { return FMath::FloorToInt32(CharsShown); }
	UPaperSprite* GetPortrait() const;
	UPaperSprite* Art(FName Name) const;
	const FString& GetObjective() const { return Objective; }
	const FPhoneView& GetPhone() const { return Phone; }
	const TArray<FStoryPopup>& GetPopups() const { return Popups; }
	const AStoryProp* GetFocusProp() const { return FocusProp.Get(); }
	// The mace target under a world point (the cursor), if any.
	AStoryProp* FindMaceTargetAt(const FVector& Point) const;

protected:
	// Called once, on the first frame the game is actually running (after any start menu).
	virtual void StartScene() {}

	AStoryProp* AddProp(FName ArtName, const FVector2D& Where, bool bBlocksMovement = true);
	void SetFloor(FName ArtName);
	void Sfx(FName Name) const;
	void After(float Seconds, TFunction<void()> Do);
	void Popup(const FString& Text, const AActor& Over);
	// Bounces an arrow over a prop to show where to go next; null hides it.
	void PointAt(AStoryProp* Target);
	void MoveHeroTo(const FVector2D& Where);
	// Loads the current hero's script file (Content/Story/<Hero>.txt).
	void LoadHeroScript();
	bool IsHero(const TCHAR* Name) const;

	AArenaPlayerPawn* GetHero() const;
	UArenaSession& GetSession() const;

	FBox2D Bounds = FBox2D(FVector2D(-1000.0), FVector2D(1000.0));
	TArray<FBox2D> Blockers;
	FStoryScript Script;
	FPhoneView Phone;
	FString Objective;
	bool bLockInput = false;
	bool bHoldStill = false;
	bool bLeaving = false;

private:
	void TickDialog(float DeltaTime);
	void UpdateFocus();

	UPROPERTY()
	TArray<TObjectPtr<AStoryProp>> Props;

	UPROPERTY()
	TObjectPtr<AStoryProp> Arrow;

	TWeakObjectPtr<AStoryProp> PointTarget;
	TWeakObjectPtr<AStoryProp> FocusProp;
	TArray<FStoryPopup> Popups;

	TArray<FStoryLine> Lines;
	int32 LineIndex = INDEX_NONE;
	float CharsShown = 0.f;
	TFunction<void()> AfterDialog;
	bool bStarted = false;
};

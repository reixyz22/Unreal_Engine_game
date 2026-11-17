#pragma once

#include "CoreMinimal.h"
#include "StoryScene.h"
#include "HutScene.generated.h"

// The hero's home. The first visit plays their intro.
//   Nikki: Carl's text wakes her, she blocks him, her alarm goes off, she swings her mace
//          out of bed, gets dressed, heads out.
//   Carl: already up, he texts his crush, finds he's blocked, goes back to bed, and is
//         woken later by his dog.
UCLASS()
class PROJECT_RIOT_API AHutScene : public AStoryScene
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void StartScene() override;

private:
	void PlayIntro();
	void OnPhoneRead();
	void SoundAlarm();
	void SmashOutOfBed();
	void GetDressed();

	void PlayCarlIntro();
	void TextCrush();
	void BackToBed();
	void WakeWithDog();

	UPROPERTY()
	TObjectPtr<AStoryProp> Bed;

	UPROPERTY()
	TObjectPtr<AStoryProp> Nightstand;

	UPROPERTY()
	TObjectPtr<AStoryProp> Closet;

	UPROPERTY()
	TObjectPtr<AStoryProp> Door;

	UPROPERTY()
	TObjectPtr<AStoryProp> Alert;

	bool bPhoneTyping = false;
	// Nikki is in the bed sprite, so her pawn stays hidden (checked every frame, in case
	// the pawn is replaced while the intro runs).
	bool bInBed = false;
	bool bDoorOpen = false;
};

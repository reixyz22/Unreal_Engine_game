#pragma once

#include "CoreMinimal.h"
#include "StoryScene.h"
#include "HutScene.generated.h"

// Nikki's hut. The first visit plays the intro: Carl's text wakes her, she blocks him,
// smashes the bed, gets dressed, and heads out.
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
	void WakeUp();
	void SmashBed();
	void GetDressed();

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
	bool bDoorOpen = false;
};

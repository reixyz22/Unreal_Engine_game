#pragma once

#include "CoreMinimal.h"
#include "StoryScene.h"
#include "TownScene.generated.h"

// The hero's home town: a walkable hub with no combat. Signs and villagers tell the
// story, and the elder's contract opens the road out. (The shop is on the world map.)
UCLASS()
class PROJECT_RIOT_API ATownScene : public AStoryScene
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void StartScene() override;

private:
	// A prop that plays one script section when used.
	AStoryProp* AddTalker(FName ArtName, const FVector2D& Where, const FString& Prompt, const FString& Section, bool bBlocksMovement = true);
	void SignContract();

	UPROPERTY()
	TObjectPtr<AStoryProp> Contract;

	UPROPERTY()
	TObjectPtr<AStoryProp> Gate;

	bool bWarnedAtGate = false;
};

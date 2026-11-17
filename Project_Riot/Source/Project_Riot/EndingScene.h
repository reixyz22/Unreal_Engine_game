#pragma once

#include "CoreMinimal.h"
#include "StoryScene.h"
#include "EndingScene.generated.h"

// Placeholder ending for a route: the hero says the script's ending lines, the route is
// marked finished and saved, and the game returns to the hero select screen.
UCLASS()
class PROJECT_RIOT_API AEndingScene : public AStoryScene
{
	GENERATED_BODY()

protected:
	virtual void StartScene() override;
};

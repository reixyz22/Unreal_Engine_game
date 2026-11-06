#include "ArenaGameMode.h"

#include "Kismet/GameplayStatics.h"

void AArenaGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (Music)
	{
		UGameplayStatics::SpawnSound2D(this, Music);
	}
}

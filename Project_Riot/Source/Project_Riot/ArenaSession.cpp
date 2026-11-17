#include "ArenaSession.h"

#include "ArenaProgress.h"

#include "Components/AudioComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr float DuckedMusicScale = 0.3f;
	constexpr float MusicFadeInSeconds = 2.5f;
}

UArenaSession* UArenaSession::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UArenaSession>() : nullptr;
}

void UArenaSession::PlayMusic(USoundBase* Music)
{
	// The player outlives level reloads, so walking from the hut to the village keeps the tune going.
	if (IsValid(MusicPlayer) && MusicPlayer->Sound == Music && MusicPlayer->IsPlaying())
	{
		return;
	}
	if (IsValid(MusicPlayer))
	{
		MusicPlayer->Stop();
	}
	MusicPlayer = Music ? UGameplayStatics::CreateSound2D(GetGameInstance()->GetWorld(), Music, 1.f, 1.f, 0.f, nullptr, true, false) : nullptr;
	if (IsValid(MusicPlayer))
	{
		// UI sounds keep playing while the game is paused, so menus have music.
		MusicPlayer->bIsUISound = true;
		ApplyMusicVolume();
		MusicPlayer->FadeIn(MusicFadeInSeconds);
	}
}

namespace
{
	const FString ProgressSlot(TEXT("Progress"));
}

void UArenaSession::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (const UArenaProgress* Saved = Cast<UArenaProgress>(UGameplayStatics::LoadGameFromSlot(ProgressSlot, 0)))
	{
		Gold = Saved->Gold;
		Purchases = Saved->Purchases;
		Outfit = Saved->Outfit;
		FinishedRoutes = Saved->FinishedRoutes;
	}
}

void UArenaSession::SaveProgress() const
{
	UArenaProgress* Progress = Cast<UArenaProgress>(UGameplayStatics::CreateSaveGameObject(UArenaProgress::StaticClass()));
	Progress->Gold = Gold;
	Progress->Purchases = Purchases;
	Progress->Outfit = Outfit;
	Progress->FinishedRoutes = FinishedRoutes;
	UGameplayStatics::SaveGameToSlot(Progress, ProgressSlot, 0);
}

void UArenaSession::FinishRoute(const UObject* WorldContext, const FString& RouteName)
{
	FinishedRoutes.AddUnique(RouteName);
	SaveProgress();

	// Back to the start: a fresh story, with money and purchases kept.
	ChosenHero = nullptr;
	bIntroDone = bFinalRoute = false;
	bIntroSeen = bBedSmashed = bDressed = bContractSigned = false;
	StageIndex = HighestUnlocked = 1;
	PendingSection.Reset();
	Travel(WorldContext, EArenaScene::Hut);
	PreviousScene = EArenaScene::Hut;
}

void UArenaSession::Travel(const UObject* WorldContext, EArenaScene NewScene, int32 NewStageIndex)
{
	PreviousScene = Scene;
	Scene = NewScene;
	if (NewStageIndex != INDEX_NONE)
	{
		StageIndex = NewStageIndex;
	}
	UGameplayStatics::OpenLevel(WorldContext, FName(*UGameplayStatics::GetCurrentLevelName(WorldContext, true)));
}

void UArenaSession::PlaySfx(const UObject* WorldContext, USoundBase* Sound)
{
	const UArenaSession* Session = Get(WorldContext);
	if (Sound && Session && Session->SfxVolume > 0.f)
	{
		UGameplayStatics::PlaySound2D(WorldContext, Sound, Session->SfxVolume);
	}
}

void UArenaSession::SetMusicVolume(float Volume)
{
	MusicVolume = FMath::Clamp(Volume, 0.f, 1.f);
	ApplyMusicVolume();
}

void UArenaSession::SetMusicDucked(bool bDucked)
{
	bMusicDucked = bDucked;
	ApplyMusicVolume();
}

void UArenaSession::ApplyMusicVolume() const
{
	if (IsValid(MusicPlayer))
	{
		MusicPlayer->SetVolumeMultiplier(FMath::Max(0.001f, MusicVolume * (bMusicDucked ? DuckedMusicScale : 1.f)));
	}
}

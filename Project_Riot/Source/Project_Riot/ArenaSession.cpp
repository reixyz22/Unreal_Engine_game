#include "ArenaSession.h"

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
	if (IsValid(MusicPlayer))
	{
		MusicPlayer->Stop();
	}
	MusicPlayer = Music ? UGameplayStatics::CreateSound2D(GetGameInstance()->GetWorld(), Music) : nullptr;
	if (IsValid(MusicPlayer))
	{
		// UI sounds keep playing while the game is paused, so menus have music.
		MusicPlayer->bIsUISound = true;
		ApplyMusicVolume();
		MusicPlayer->FadeIn(MusicFadeInSeconds);
	}
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

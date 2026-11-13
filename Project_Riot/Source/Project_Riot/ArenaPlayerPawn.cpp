#include "ArenaPlayerPawn.h"

#include "ArenaArt.h"
#include "ArenaChainBall.h"
#include "ArenaEnemy.h"
#include "ArenaGameMode.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "HealthComponent.h"
#include "InputActionValue.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "PaperSpriteComponent.h"

namespace
{
	constexpr float PlayerDepth = 10.f;
	constexpr float ChainDepth = 16.f;
	constexpr float CameraDistance = 1000.f;
	constexpr float BlinkPeriod = 0.1f;
	constexpr double ArriveDistance = 10.0;
	// Where her hand sits in the strike frames, in sprite pixels from the centre.
	const FVector HandOffset(6.f, 1.f, -4.f);

	// One grade over every art pack so they read as one game (DECISIONS.md 008).
	void ApplyArenaLook(FPostProcessSettings& Post)
	{
		Post.bOverride_BloomIntensity = true;
		Post.BloomIntensity = 1.4f;
		Post.bOverride_BloomThreshold = true;
		Post.BloomThreshold = 0.55f;
		Post.bOverride_VignetteIntensity = true;
		Post.VignetteIntensity = 0.7f;
		Post.bOverride_ColorSaturation = true;
		Post.ColorSaturation = FVector4(0.9, 0.9, 0.9, 1.0);
		Post.bOverride_ColorContrast = true;
		Post.ColorContrast = FVector4(1.1, 1.1, 1.1, 1.0);
		Post.bOverride_SceneColorTint = true;
		Post.SceneColorTint = FLinearColor(0.96f, 0.92f, 1.f);
	}
}

AArenaPlayerPawn::AArenaPlayerPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(28.f);
	Collision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	SetRootComponent(Collision);

	Body = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Body"));
	Body->SetupAttachment(Collision);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetRelativeScale3D(FVector(ArenaArt::PixelScale));

	HeldFlail = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("HeldFlail"));
	HeldFlail->SetupAttachment(Body);
	HeldFlail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeldFlail->SetRelativeLocation(HandOffset);
	HeldFlail->SetRelativeScale3D(FVector(ArenaArt::FlailScale));
	HeldFlail->SetVisibility(false);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Collision);
	Camera->SetRelativeLocation(FVector(0.f, CameraDistance, 0.f));
	Camera->SetUsingAbsoluteRotation(true);
	Camera->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);
	ApplyArenaLook(Camera->PostProcessSettings);

	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
}

void AArenaPlayerPawn::BeginPlay()
{
	Super::BeginPlay();

	FVector Start = GetActorLocation();
	Start.Y = PlayerDepth;
	SetActorLocation(Start);

	Camera->SetOrthoWidth(CameraViewWidth);
	if (FogMaterial)
	{
		Camera->PostProcessSettings.AddBlendable(FogMaterial, 1.f);
	}
	Body->SetFlipbook(IdleAnim);
	Body->SetSpriteColor(BodyTint);
	HeldFlail->SetSprite(FlailHandleSprite);

	Health->OnDepleted.AddUObject(this, &AArenaPlayerPawn::Die);
}

void AArenaPlayerPawn::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	PlayerController->bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	PlayerController->SetInputMode(InputMode);

	if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
	{
		Viewport->SetHardwareCursor(EMouseCursor::Default, PointerCursor, FVector2D::ZeroVector);
		Viewport->SetHardwareCursor(EMouseCursor::Crosshairs, AttackCursor, FVector2D::ZeroVector);
	}

	if (InputMapping)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(InputMapping, 0);
		}
	}
}

void AArenaPlayerPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (Input && MoveAction)
	{
		Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AArenaPlayerPawn::Move);
		Input->BindAction(MoveAction, ETriggerEvent::Completed, this, &AArenaPlayerPawn::StopMove);
	}
}

void AArenaPlayerPawn::Move(const FInputActionValue& Value)
{
	MoveInput = Value.Get<FVector2D>();
	MoveTarget.Reset();
	BufferedAuto.Reset();
}

void AArenaPlayerPawn::StopMove()
{
	MoveInput = FVector2D::ZeroVector;
}

void AArenaPlayerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Health->IsDead())
	{
		return;
	}

	UpdateHover();
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController && PlayerController->WasInputKeyJustPressed(EKeys::RightMouseButton))
	{
		HandleRightClick();
	}
	UpdateMovement(DeltaTime);
	UpdateBlink();
}

void AArenaPlayerPawn::UpdateHover()
{
	FVector Cursor;
	AArenaEnemy* Enemy = GetCursorWorldLocation(Cursor) ? FindEnemyNear(Cursor) : nullptr;
	if (Enemy != HoveredEnemy.Get())
	{
		if (HoveredEnemy.IsValid())
		{
			HoveredEnemy->SetHovered(false);
		}
		if (Enemy)
		{
			Enemy->SetHovered(true);
		}
		HoveredEnemy = Enemy;
	}

	// Set every frame: a hovered enemy can die, which would otherwise leave the sword cursor up.
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		PlayerController->CurrentMouseCursor = Enemy ? EMouseCursor::Crosshairs : EMouseCursor::Default;
	}
}

void AArenaPlayerPawn::HandleRightClick()
{
	MoveTarget.Reset();
	BufferedAuto.Reset();

	AArenaEnemy* Enemy = HoveredEnemy.Get();
	if (!Enemy)
	{
		FVector Cursor;
		if (GetCursorWorldLocation(Cursor))
		{
			MoveTarget = Cursor;
		}
		return;
	}

	FVector ToEnemy = Enemy->GetActorLocation() - GetActorLocation();
	ToEnemy.Y = 0.0;
	const double Gap = ToEnemy.Size() - AutoRange;
	if (Gap <= 0.0)
	{
		ThrowChainAt(Enemy);
		return;
	}

	// Out of range: walk to where it was in range, then fire (DECISIONS.md 006).
	MoveTarget = GetActorLocation() + ToEnemy.GetSafeNormal() * (Gap + ArriveDistance);
	BufferedAuto = Enemy;
}

void AArenaPlayerPawn::UpdateMovement(float DeltaTime)
{
	const FVector2D Direction = ComputeMoveDirection();

	FVector Location = GetActorLocation();
	Location.X += Direction.X * MoveSpeed * DeltaTime;
	Location.Z += Direction.Y * MoveSpeed * DeltaTime;

	const double Limit = ArenaArt::ArenaHalfSize - ArenaArt::WallThickness - Collision->GetScaledSphereRadius();
	Location.X = FMath::Clamp(Location.X, -Limit, Limit);
	Location.Z = FMath::Clamp(Location.Z, -Limit, Limit);

	SetActorLocation(Location);
	UpdateAnimation(Direction);
}

FVector2D AArenaPlayerPawn::ComputeMoveDirection()
{
	if (!MoveInput.IsNearlyZero())
	{
		return MoveInput.GetClampedToMaxSize(1.0);
	}
	if (!MoveTarget.IsSet())
	{
		return FVector2D::ZeroVector;
	}

	const FVector2D ToTarget(MoveTarget->X - GetActorLocation().X, MoveTarget->Z - GetActorLocation().Z);
	if (ToTarget.Size() > ArriveDistance)
	{
		return ToTarget.GetSafeNormal();
	}

	MoveTarget.Reset();
	if (AArenaEnemy* Target = BufferedAuto.Get())
	{
		ThrowChainAt(Target);
	}
	BufferedAuto.Reset();
	return FVector2D::ZeroVector;
}

void AArenaPlayerPawn::ThrowChainAt(AArenaEnemy* Target)
{
	if (bChainOut)
	{
		return;
	}

	FVector SpawnAt = GetActorLocation();
	SpawnAt.Y = ChainDepth;

	if (AArenaChainBall* Ball = GetWorld()->SpawnActor<AArenaChainBall>(SpawnAt, FRotator::ZeroRotator))
	{
		Ball->Throw(this, Target, ChainBallSprite, FLinearColor::White, ChainSpeed, AutoDamage);
		bChainOut = true;
		PlayStrike();
	}
}

bool AArenaPlayerPawn::GetCursorWorldLocation(FVector& OutLocation) const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	FVector Direction;
	return PlayerController && PlayerController->DeprojectMousePositionToWorld(OutLocation, Direction);
}

AArenaEnemy* AArenaPlayerPawn::FindEnemyNear(const FVector& Point) const
{
	AArenaEnemy* Best = nullptr;
	double BestDistance = TNumericLimits<double>::Max();

	for (TActorIterator<AArenaEnemy> It(GetWorld()); It; ++It)
	{
		const FVector EnemyLocation = It->GetActorLocation();
		const double Distance = FVector2D(EnemyLocation.X - Point.X, EnemyLocation.Z - Point.Z).Size();
		if (Distance < It->GetRadius() + ClickRadius && Distance < BestDistance)
		{
			Best = *It;
			BestDistance = Distance;
		}
	}
	return Best;
}

void AArenaPlayerPawn::OnChainReturned()
{
	bChainOut = false;
}

bool AArenaPlayerPawn::ReceiveDamage(float Amount)
{
	return Health->ApplyDamage(Amount);
}

float AArenaPlayerPawn::GetRadius() const
{
	return Collision->GetScaledSphereRadius();
}

FVector AArenaPlayerPawn::GetHandLocation() const
{
	return HeldFlail->GetComponentLocation();
}

void AArenaPlayerPawn::PlayStrike()
{
	if (!StrikeAnim)
	{
		return;
	}

	Body->SetFlipbook(StrikeAnim);
	Body->SetLooping(false);
	Body->PlayFromStart();
	StrikeEndsAt = GetWorld()->GetTimeSeconds() + StrikeAnim->GetTotalDuration();
}

void AArenaPlayerPawn::UpdateAnimation(const FVector2D& Direction)
{
	const bool bStriking = GetWorld()->GetTimeSeconds() < StrikeEndsAt;
	HeldFlail->SetVisibility(bStriking);
	if (!bStriking)
	{
		const bool bMoving = !Direction.IsNearlyZero();
		UPaperFlipbook* Wanted = (bMoving && RunAnim) ? RunAnim.Get() : IdleAnim.Get();
		if (Body->GetFlipbook() != Wanted)
		{
			Body->SetFlipbook(Wanted);
			Body->SetLooping(true);
			Body->Play();
		}
	}

	if (FMath::Abs(Direction.X) > KINDA_SMALL_NUMBER)
	{
		const float Facing = Direction.X < 0.f ? -1.f : 1.f;
		Body->SetRelativeScale3D(FVector(Facing * ArenaArt::PixelScale, ArenaArt::PixelScale, ArenaArt::PixelScale));
	}
}

void AArenaPlayerPawn::UpdateBlink()
{
	const bool bBlinkOff = Health->IsInvulnerable() &&
		FMath::Fmod(GetWorld()->GetTimeSeconds(), BlinkPeriod) < BlinkPeriod * 0.5f;
	Body->SetVisibility(!bBlinkOff);
}

void AArenaPlayerPawn::Die()
{
	Body->SetVisibility(false, true);

	if (AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>())
	{
		GameMode->OnPlayerDied();
	}
}

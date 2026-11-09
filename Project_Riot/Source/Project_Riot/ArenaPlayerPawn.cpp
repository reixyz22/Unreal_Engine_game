#include "ArenaPlayerPawn.h"

#include "ArenaArt.h"
#include "ArenaChainBall.h"
#include "ArenaEnemy.h"
#include "ArenaGameMode.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/Engine.h"
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
	constexpr int32 StatusMessageKey = 2;
	const FVector HandOffset(5.f, 1.f, -3.f);
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

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Collision);
	Camera->SetRelativeLocation(FVector(0.f, CameraDistance, 0.f));
	Camera->SetUsingAbsoluteRotation(true);
	Camera->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);

	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
}

void AArenaPlayerPawn::BeginPlay()
{
	Super::BeginPlay();

	FVector Start = GetActorLocation();
	Start.Y = PlayerDepth;
	SetActorLocation(Start);

	Camera->SetOrthoWidth(CameraViewWidth);
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

	const FVector2D Direction = MoveInput.GetClampedToMaxSize(1.0);

	FVector Location = GetActorLocation();
	Location.X += Direction.X * MoveSpeed * DeltaTime;
	Location.Z += Direction.Y * MoveSpeed * DeltaTime;

	const double Limit = ArenaHalfSize - WallThickness - Collision->GetScaledSphereRadius();
	Location.X = FMath::Clamp(Location.X, -Limit, Limit);
	Location.Z = FMath::Clamp(Location.Z, -Limit, Limit);

	SetActorLocation(Location);
	UpdateAnimation(Direction);

	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController && PlayerController->WasInputKeyJustPressed(EKeys::RightMouseButton))
	{
		TryAutoAttack();
	}

	const bool bBlinkOff = Health->IsInvulnerable() &&
		FMath::Fmod(GetWorld()->GetTimeSeconds(), BlinkPeriod) < BlinkPeriod * 0.5f;
	Body->SetVisibility(!bBlinkOff, true);

	UpdateStatusText();
}

void AArenaPlayerPawn::TryAutoAttack()
{
	if (bChainOut)
	{
		return;
	}

	AArenaEnemy* Target = FindEnemyUnderCursor();
	if (!Target)
	{
		return;
	}

	FVector SpawnAt = GetActorLocation();
	SpawnAt.Y = ChainDepth;

	AArenaChainBall* Ball = GetWorld()->SpawnActor<AArenaChainBall>(SpawnAt, FRotator::ZeroRotator);
	if (Ball)
	{
		Ball->Throw(this, Target, ChainBallSprite, FLinearColor::White, ChainSpeed, AutoDamage);
		bChainOut = true;
		PlayStrike();
	}
}

AArenaEnemy* AArenaPlayerPawn::FindEnemyUnderCursor() const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	FVector CursorWorld;
	FVector CursorDirection;
	if (!PlayerController || !PlayerController->DeprojectMousePositionToWorld(CursorWorld, CursorDirection))
	{
		return nullptr;
	}

	AArenaEnemy* Best = nullptr;
	double BestDistance = TNumericLimits<double>::Max();

	for (TActorIterator<AArenaEnemy> It(GetWorld()); It; ++It)
	{
		const FVector EnemyLocation = It->GetActorLocation();

		const double FromCursor = FVector2D(EnemyLocation.X - CursorWorld.X, EnemyLocation.Z - CursorWorld.Z).Size();
		const double FromPlayer = FVector2D(EnemyLocation.X - GetActorLocation().X, EnemyLocation.Z - GetActorLocation().Z).Size();

		if (FromCursor < It->GetRadius() + ClickRadius && FromPlayer <= AutoRange && FromCursor < BestDistance)
		{
			Best = *It;
			BestDistance = FromCursor;
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

void AArenaPlayerPawn::UpdateStatusText() const
{
	if (GEngine)
	{
		const FString Status = FString::Printf(TEXT("Nikki HP %.0f / %.0f%s"),
			Health->GetHealth(), Health->GetMaxHealth(), bChainOut ? TEXT("   (chain out)") : TEXT(""));
		GEngine->AddOnScreenDebugMessage(StatusMessageKey, 1.f, FColor(255, 140, 200), Status);
	}
}

void AArenaPlayerPawn::Die()
{
	Body->SetVisibility(false, true);
	UpdateStatusText();

	if (AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>())
	{
		GameMode->OnPlayerDied();
	}
}

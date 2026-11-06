#include "ArenaPlayerPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "PaperSpriteComponent.h"

namespace
{
	constexpr float PlayerDepth = 10.f;
	constexpr float CameraDistance = 1000.f;
}

AArenaPlayerPawn::AArenaPlayerPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(28.f);
	Collision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	SetRootComponent(Collision);

	Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(Collision);
	Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Collision);
	Camera->SetUsingAbsoluteLocation(true);
	Camera->SetUsingAbsoluteRotation(true);
	Camera->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);
}

void AArenaPlayerPawn::BeginPlay()
{
	Super::BeginPlay();

	FVector Start = GetActorLocation();
	Start.Y = PlayerDepth;
	SetActorLocation(Start);

	Camera->SetOrthoWidth(CameraViewWidth);
	Camera->SetWorldLocation(FVector(Start.X, CameraDistance, Start.Z));
}

void AArenaPlayerPawn::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController && InputMapping)
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

	const FVector2D Direction = MoveInput.GetClampedToMaxSize(1.0);

	FVector Location = GetActorLocation();
	Location.X += Direction.X * MoveSpeed * DeltaTime;
	Location.Z += Direction.Y * MoveSpeed * DeltaTime;

	const double Limit = ArenaHalfSize - WallThickness - Collision->GetScaledSphereRadius();
	Location.X = FMath::Clamp(Location.X, -Limit, Limit);
	Location.Z = FMath::Clamp(Location.Z, -Limit, Limit);

	SetActorLocation(Location);
	UpdateCamera(DeltaTime);
}

void AArenaPlayerPawn::UpdateCamera(float DeltaTime)
{
	const FVector From = Camera->GetComponentLocation();
	FVector To(GetActorLocation().X, From.Y, GetActorLocation().Z);

	const double HalfWidth = CameraViewWidth * 0.5;
	const double HalfHeight = HalfWidth / Camera->AspectRatio;
	To.X = FMath::Clamp(To.X, -ArenaHalfSize + HalfWidth, ArenaHalfSize - HalfWidth);
	To.Z = FMath::Clamp(To.Z, -ArenaHalfSize + HalfHeight, ArenaHalfSize - HalfHeight);

	Camera->SetWorldLocation(FMath::VInterpTo(From, To, DeltaTime, CameraFollowSpeed));
}

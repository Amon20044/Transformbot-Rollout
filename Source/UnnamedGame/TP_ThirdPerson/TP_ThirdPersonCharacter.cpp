#include "TP_ThirdPersonCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Components/SkeletalMeshComponent.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);


ATP_ThirdPersonCharacter::ATP_ThirdPersonCharacter()
{

    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
    FVector CurrentMeshLocation = GetMesh()->GetRelativeLocation();
    CurrentMeshLocation.Z -= NormalizeFactor; // Adjust by half the difference in capsule heights
    GetMesh()->SetRelativeLocation(CurrentMeshLocation);


    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
    GetCharacterMovement()->JumpZVelocity = 700.f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->MaxWalkSpeed = 500.f;
    GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
    GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
    NormalizeFactor = 32.f;


    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.0f;
    CameraBoom->bUsePawnControlRotation = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    MaxForwardSpeed = 1200.f; // Set your desired max speed
    MaxSteerAngle = 45.f; // Set your desired max steer angle
}

void ATP_ThirdPersonCharacter::BeginPlay()
{
    Super::BeginPlay();

    canTransformToCar = false;
    canTransformToRobot = true;
}


void ATP_ThirdPersonCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATP_ThirdPersonCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }

    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(TransformAction, ETriggerEvent::Completed, this, &ATP_ThirdPersonCharacter::Transform);
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATP_ThirdPersonCharacter::Move);
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATP_ThirdPersonCharacter::Look);
    }
    else
    {
        UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
    }
}
void ATP_ThirdPersonCharacter::Transform(const FInputActionValue& Value)
{
    // Toggle between robot and car forms
    canTransformToCar = !canTransformToCar;
    canTransformToRobot = !canTransformToRobot;

    if (canTransformToCar)
    {
        // Change capsule size to match car form
        GetCapsuleComponent()->SetCapsuleSize(42.f, 42.f);

        // Adjust mesh location relative to the capsule
        FVector CurrentMeshLocation = GetMesh()->GetRelativeLocation();
        CurrentMeshLocation.Z += NormalizeFactor ; // Adjust by half the difference in capsule heights
        GetMesh()->SetRelativeLocation(CurrentMeshLocation);
    }
    else
    {
        // Change capsule size to match robot form
        GetCapsuleComponent()->SetCapsuleSize(42.f, 96.0f);

        // Adjust mesh location relative to the capsule
        FVector CurrentMeshLocation = GetMesh()->GetRelativeLocation();
        CurrentMeshLocation.Z -= NormalizeFactor ; // Adjust by half the difference in capsule heights
        GetMesh()->SetRelativeLocation(CurrentMeshLocation);
    }
}

void ATP_ThirdPersonCharacter::Move(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        if (canTransformToRobot)
        {
            AddMovementInput(ForwardDirection, MovementVector.Y);
            AddMovementInput(RightDirection, MovementVector.X);
        }
        else
        {
            Throttle = MovementVector.Y;
            Steering = MovementVector.X;

            // Log the steering value
            UE_LOG(LogTemp, Log, TEXT("Steering Value: %f"), Steering);

            if (Throttle != 0)
            {
                // Increment elapsed time
                ElapsedTime += GetWorld()->GetDeltaSeconds();
            }
            else
            {
                // Reset elapsed time if no throttle
                ElapsedTime = 0;
            }

            // Get the speed factor from the curve
            float SpeedFactor = SpeedCurve ? SpeedCurve->GetFloatValue(ElapsedTime) : 1.0f;

            // Calculate the target speed based on throttle and curve
            float TargetSpeed = (Throttle > 0)
                ? FMath::Lerp(0.0f, MaxForwardSpeed, SpeedFactor * FMath::Abs(Throttle))
                : MaxForwardSpeed - (MaxForwardSpeed / 6.0f); // Reverse speed

            // Interpolate the velocity
            float InterpSpeed = 1.0f / 3.0f; // Time to reach max speed is 3 seconds
            Velocity = FMath::VInterpTo(Velocity, ForwardDirection * TargetSpeed * FMath::Sign(Throttle), GetWorld()->GetDeltaSeconds(), InterpSpeed);

            // Clamp velocity to maximum speed
            Velocity = Velocity.GetClampedToMaxSize(MaxForwardSpeed);

            // Apply steering if the car is moving
            if (!Velocity.IsNearlyZero())
            {
                // Determine the steering input direction based on throttle
                float SteeringInput = Throttle >= 0 ? Steering : -Steering;

                // Smooth steering proportional to the forward velocity
                float SteeringSpeedFactor = Velocity.Size() / MaxForwardSpeed;
                float SteerAngle = FMath::Clamp(SteeringInput, -1.0f, 1.0f) * MaxSteerAngle * SteeringSpeedFactor;
                FRotator DesiredRotation = GetActorRotation() + FRotator(0.0f, SteerAngle, 0.0f);
                FRotator InterpRotation = FMath::RInterpTo(GetActorRotation(), DesiredRotation, GetWorld()->GetDeltaSeconds(), 10.0f);
                SetActorRotation(InterpRotation);
            }

            // Update forward direction based on new rotation
            FVector NewForwardDirection = GetActorForwardVector();

            // Update position based on new forward direction and velocity
            if (Throttle < 0)
            {
                AddActorWorldOffset(NewForwardDirection * MaxForwardSpeed / 7 * GetWorld()->GetDeltaSeconds() * FMath::Sign(Throttle), true);
            }
            else
            {
                AddActorWorldOffset(NewForwardDirection * Velocity.Size() * GetWorld()->GetDeltaSeconds() * FMath::Sign(Throttle), true);
            }

        }
    }
}
void ATP_ThirdPersonCharacter::Look(const FInputActionValue& Value)
{
    FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        AddControllerYawInput(LookAxisVector.X);
        AddControllerPitchInput(LookAxisVector.Y);
    }
}


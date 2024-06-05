#include "VehiclePhysics.h"
#include "GameFramework/Actor.h"

UVehiclePhysicsComponent::UVehiclePhysicsComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UVehiclePhysicsComponent::BeginPlay()
{
    Super::BeginPlay();
    CurrentSpeed = 0.0f;
    SteeringAngle = 0.0f;
}

void UVehiclePhysicsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (FMath::Abs(CurrentSpeed) > KINDA_SMALL_NUMBER)
    {
        FVector Forward = GetOwner()->GetActorForwardVector();
        FVector Movement = Forward * CurrentSpeed * DeltaTime;

        FHitResult Hit;
        GetOwner()->AddActorWorldOffset(Movement, true, &Hit);

        if (Hit.IsValidBlockingHit())
        {
            CurrentSpeed = 0.0f;
        }

        FRotator NewRotation = GetOwner()->GetActorRotation();
        NewRotation.Yaw += SteeringAngle * DeltaTime;
        GetOwner()->SetActorRotation(NewRotation);
    }

    // Decelerate the car over time
    CurrentSpeed = FMath::FInterpTo(CurrentSpeed, 0.0f, DeltaTime, 0.1f);
}

void UVehiclePhysicsComponent::AccelerateCar(float Value)
{
    CurrentSpeed = FMath::Clamp(CurrentSpeed + Value * Acceleration * GetWorld()->GetDeltaSeconds(), -MaxSpeed, MaxSpeed);
}

void UVehiclePhysicsComponent::BrakeCar(float Value)
{
    CurrentSpeed = FMath::Clamp(CurrentSpeed - Value * Acceleration * GetWorld()->GetDeltaSeconds(), 0.0f, MaxSpeed);
}

void UVehiclePhysicsComponent::TurnCar(float Value)
{
    SteeringAngle = Value * TurnSpeed;
}

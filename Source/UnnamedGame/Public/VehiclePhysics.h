#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VehiclePhysics.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNNAMEDGAME_API UVehiclePhysicsComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UVehiclePhysicsComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    void AccelerateCar(float Value);
    void BrakeCar(float Value);
    void TurnCar(float Value);

private:
    UPROPERTY(EditAnywhere)
    float Acceleration = 1000.0f;

    UPROPERTY(EditAnywhere)
    float MaxSpeed = 5000.0f;

    UPROPERTY(EditAnywhere)
    float TurnSpeed = 50.0f;

    FVector Velocity;
    float CurrentSpeed;
    float SteeringAngle;
};

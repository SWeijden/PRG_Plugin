// Copyright 2022 Steven Weijden

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseGizmos/GizmoActor.h"
#include "Engine/StaticMeshActor.h"
#include "PRG_Room.generated.h"

DECLARE_DELEGATE_OneParam(FOnRoomDeletionDelegate, TObjectPtr<APRG_Room>);

class UTransformProxy;
class UCombinedTransformGizmo;

UCLASS()
class PRG_PLUGIN_API AWall : public AStaticMeshActor
{
	GENERATED_BODY()

	AWall();
};

UCLASS()
class PRG_PLUGIN_API ATile : public AStaticMeshActor
{
	GENERATED_BODY()

	ATile();
};

UCLASS()
class PRG_PLUGIN_API APRG_Room : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APRG_Room();
	// Must have default ctor for UObject initialization. So set input based init afterwards
	void InitRoom(FIntPoint NewSize = FIntPoint(0, 0), int NewHeight = 0);

	// Delegate to handle cleanup of room deletion, if not done via tool
	FOnRoomDeletionDelegate OnRoomDeletion;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called when this actor is explicitly being destroyed during gameplay or in the editor
	virtual void Destroyed() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Store proxy
	void SetProxyTransform(TObjectPtr<UTransformProxy> Proxy);
	// Get proxy. Used to find moved room
	TObjectPtr<UTransformProxy> GetProxyTransform() const;

	// Store gizmo
	void SetRoomGizmo(TObjectPtr<UCombinedTransformGizmo> Gizmo);
	// Destroy gizmo.
	void DestroyRoomGizmo(UInteractiveGizmoManager* GizmoManager);

	// Get array of persistent walls of room
	TArray<TObjectPtr<AWall>>& GetWalls();
	// Get array of persistent tiles of room
	TArray<TObjectPtr<ATile>>& GetTiles();

	// Calculate the tile index based on the local tile position
	int GetTileIndexByPosition(FVector Position, int TileSizeCM) const;
	// Calculate the wall index based on the local wall position
	int GetWallIndexByPosition(FVector Position, int TileSizeCM) const;

	// Calculate the local tile position based on tile index
	FVector GetTilePositionFromIndex(int Index, int TileSizeCM) const;
	// Calculate the local wall position based on wall index
	FVector GetWallPositionFromIndex(int Index, int TileSizeCM) const;

	// Assign given tile to persistent tile array at given index
	void SetTileAtIndex(int Index, TObjectPtr<ATile> NewTile);
	// Assign given wall to persistent wall array at given index
	void SetWallAtIndex(int Index, TObjectPtr<AWall> NewWall);

	// Calculate wall rotation based on index
	FRotator GetWallRotationByIndex(int Index) const;

	// Get room size. XY in tile count, Z in meters
	FVector GetRoomSize() { return FVector(RoomSize.X, RoomSize.Y, RoomHeight); }

protected:
	// Handle to control room gizmo lifetime
	TObjectPtr<UCombinedTransformGizmo> StoredGizmo;
	// Handle to identify room on gizmo movement
	TObjectPtr<UTransformProxy> ProxyTransform;

	// Room size in tile count
	UPROPERTY(EditAnywhere)
	FIntPoint RoomSize = { 1, 1 };
	// Room height in meters
	UPROPERTY(EditAnywhere)
	int RoomHeight = 1;

private:
	// Root component
	UPROPERTY(EditAnywhere)
	USceneComponent* BaseComponent;

	// Array of all possible walls within a room
	TArray<TObjectPtr<AWall>> Walls;
	// Array of all possible tiles within a room
	TArray<TObjectPtr<ATile>> Tiles;
};
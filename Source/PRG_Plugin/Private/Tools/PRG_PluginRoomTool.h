// Copyright 2022 Steven Weijden

#pragma once

#include "CoreMinimal.h"

#include "UObject/NoExportTypes.h"
#include "InteractiveToolBuilder.h"
#include "BaseTools/SingleClickTool.h"
#include "GameFramework/Actor.h"
#include <PRG_Room.h>

#include "PRG_PluginRoomTool.generated.h"

class UCombinedTransformGizmo;
class UInteractiveGizmo;
class UTransformProxy;

UENUM()
enum class EEditMode : uint8
{
	ViewMode,			// View content without any overlays.
	ManageRooms,	// Manage Rooms. OnClick sets the spawn point for new rooms
	EditWalls,		// Allow changing of walls in rooms. OnClick highlights selected.
	EditTiles			// Allow changing of tiles in rooms. OnClick highlights selected.
};

UENUM()
enum class ESnapSizes
{
	NoSnapping = 0,
	SnapX10 = 10,
	SnapX100 = 100,
	SnapX1000 = 1000
};

UCLASS()
class PRG_PLUGIN_API ARoomBounds : public AStaticMeshActor
{
	GENERATED_BODY()

public:
	ARoomBounds();

	TObjectPtr<UStaticMesh> CubeMesh;
	TObjectPtr<UMaterial> CubeMaterial;
};

/**
 * Builder for UPRG_PluginRoomTool
 */
UCLASS()
class PRG_PLUGIN_API UPRG_PluginRoomToolBuilder : public UInteractiveToolWithToolTargetsBuilder
{
	GENERATED_BODY()

public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override { return true; }
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;

protected:
	virtual const FToolTargetTypeRequirements& GetTargetRequirements() const override;
};

/**
 * Settings UObject for UPRG_PluginRoomTool. This UClass inherits from UInteractiveToolPropertySet,
 * which provides an OnModified delegate that the Tool will listen to for changes in property values.
 */
UCLASS(Transient)
class PRG_PLUGIN_API UPRG_PluginRoomToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()

public:
	UPRG_PluginRoomToolProperties();

	// Select editing mode
	UPROPERTY(EditAnywhere, Category = Options, meta = (DisplayName = "Edit mode"))
	EEditMode EditMode;
	// Snap rooms to the selected grid size when moving
	UPROPERTY(EditAnywhere, Category = Options, meta = (DisplayName = "Grid snap size"))
	ESnapSizes GridSnapSize;
	// Mesh used to spawn new tiles with when spawning a new room
	UPROPERTY(EditAnywhere, Category = Options, meta = (DisplayName = "Floor Object", EditCondition = "EditMode == EEditMode::ManageRooms || EditMode == EEditMode::EditTiles"))
	TObjectPtr<UStaticMesh> FloorMesh;
	// Mesh used to spawn new walls with when spawning a new room
	UPROPERTY(EditAnywhere, Category = Options, meta = (DisplayName = "Wall Object", EditCondition = "EditMode == EEditMode::ManageRooms || EditMode == EEditMode::EditWalls"))
	TObjectPtr<UStaticMesh> WallMesh;

	// Initial room size on spawn along X-axis
	UPROPERTY(EditAnywhere, Category = Spawn, meta = (DisplayName = "Room tiles", ClampMin = "1", ClampMax = "50", UIMin = "1", UIMax = "20", EditCondition = "EditMode == EEditMode::ManageRooms"))
	FIntPoint InitSize;
	// Initial room size on spawn along Y-axis
	UPROPERTY(EditAnywhere, Category = Spawn, meta = (DisplayName = "Room height (m)", ClampMin = "1", ClampMax = "50", UIMin = "1", UIMax = "20", EditCondition = "EditMode == EEditMode::ManageRooms"))
	int InitHeight;
	// Initial size used to create a room
	UPROPERTY(EditAnywhere, Category = Spawn, meta = (DisplayName = "Tile size (m)", ClampMin = "1", ClampMax = "100", UIMin = "1", UIMax = "10", EditCondition = "EditMode == EEditMode::ManageRooms"))
	int TileSize;
	// Indicator of where a new room will be spawned
	//		TODO: SUPPLEMENT OR REPLACE WITH A MARKER IN VIEWPORT?
	UPROPERTY(EditAnywhere, Category = Spawn, meta = (DisplayName = "Spawn Position", EditCondition = "EditMode == EEditMode::ManageRooms"))
	FVector SpawnPosition;

	// Overview of all room actors in the scene
	UPROPERTY(EditAnywhere, Category = Manage, meta = (DisplayName = "Room Overview", NoElementDuplicate, OnlyPlaceable, EditCondition = "EditMode == EEditMode::ManageRooms", EditFixedOrder), NoClear)
	TArray<TObjectPtr<APRG_Room>> RoomArray;

	// Default material used to revert to after edit mode
	//		TODO: Replace with restoring original material for each object
	TObjectPtr<UMaterial> DefaultMat;
	// Material shown for persistent room objects when selected in edit mode
	UPROPERTY(EditAnywhere, Category = Materials, meta = (DisplayName = "Persistent selected"))
	TObjectPtr<UMaterial> PersistSelectedMat;
	// Material shown for persistent room objects when not selected in edit mode
	UPROPERTY(EditAnywhere, Category = Materials, meta = (DisplayName = "Persistent unselected"))
	TObjectPtr<UMaterial> PersistUnselectedMat;
	// Material shown for temporary room objects when selected in edit mode
	UPROPERTY(EditAnywhere, Category = Materials, meta = (DisplayName = "Temporary selected"))
	TObjectPtr<UMaterial> TempSelectedMat;
	// Material shown for temporary room objects when not selected in edit mode
	UPROPERTY(EditAnywhere, Category = Materials, meta = (DisplayName = "Temporary unselected"))
	TObjectPtr<UMaterial> TempUnselectedMat;
};

/**
 * UPRG_PluginRoomTool is an example Tool that opens a message box displaying info about an actor that the user
 * clicks left mouse button. All the action is in the ::OnClicked handler.
 */
UCLASS()
class PRG_PLUGIN_API UPRG_PluginRoomTool : public USingleClickTool
{
	GENERATED_BODY()

public:
	UPRG_PluginRoomTool();

	virtual void SetWorld(UWorld* World);

	virtual void Setup() override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;

	virtual void OnTick(float DeltaTime) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;

	// Handle changes to properties of the tool
	virtual void OnPropertyModified(UObject* PropertySet, FProperty* Property);
	
	// Handle OnClick events in the map
	virtual void OnClicked(const FInputDeviceRay& ClickPos);

	//void DeleteRoomInScene();
	void DeleteRoomInScene(TObjectPtr<APRG_Room> DeletedRoom);

protected:
	// Handle when a gizmo is moved. Listens to OnTransformChanged on TransformProxy
	//		TODO: Fix when actor is moved or rotated, then changing gizmo reverts rather than is affected by that change. Ideally, changing actor also changes the gizmo.
	void GizmoTransformChanged(UTransformProxy* Proxy, FTransform Transform);

	// Create and store a new room
	void SpawnRoom();
	// Find rooms in scene
	void FindRoomsInScene();
	// Setup room found in the scene
	void SetupFoundRoom(TObjectPtr<APRG_Room> addRoom);

	// Remove stored room and remove from scene
	//		TODO: Add UNDO/REDO support (check where needed, currently moving gizmo of new room and undoing, doesn't remove created bounding box, creating multiple)
	void DeleteRoom(TObjectPtr<APRG_Room> removeRoom);
	// Delete any temporary actors used for editing walls and floors
	void DeleteTempActors();

	// Set current room and UI info
	void SetCurrentRoom(TObjectPtr<APRG_Room> setRoom);
	// Create a room gizmo
	void CreateRoomGizmo(TObjectPtr<APRG_Room> room, bool loadPosition);

	// Reset room from edit state. Deletes temporary actors and resets materials
	void ResetRoomEditMode(EEditMode EditMode);
	// Set room to current EditMode. Spawns appropriate temporary actors and changes material
	void SetRoomEditMode();
	// Set given material on given actor
	void SetMaterial(TObjectPtr<AStaticMeshActor> Actor, TObjectPtr<UMaterial> Material);
	// Reset materials on all persistent actors for the given EditMode
	void ResetPersistMaterials(EEditMode EditMode);
	// Try to get the current or first room available
	TObjectPtr<APRG_Room> TryFetchCurrentRoom();

private:
	// Spawn tile actor
	TObjectPtr<ATile> SpawnTile(APRG_Room& ParentRoom, int IndexInRoom, FVector SpawnPos);
	// Spawn wall actor and calculate rotation
	TObjectPtr<AWall> SpawnWall(APRG_Room& ParentRoom, int IndexInRoom, FVector SpawnPos);
	// Spawn wall actor with given rotation
	TObjectPtr<AWall> SpawnWallRot(APRG_Room& ParentRoom, FVector SpawnPos, FRotator SpawnRot);
	// Create a selection bounding box for the current room
	void SpawnRoomBoundingBox();
	// Remove the selection bounding box for the current room
	void RemoveRoomBoundingBox();

protected:
	// Handle edit mode interaction with valid actors OnClick
	template <class T>
	void OnClickEditMode(EEditMode EditMode, FHitResult& TraceResult, TArray<TObjectPtr<T>>& TempArray, TArray<TObjectPtr<T>>& PersistArray)
	{
		if (auto ActiveRoom = TryFetchCurrentRoom())
		{
			TObjectPtr<T> FoundActor = static_cast<T*>(TraceResult.GetActor());

			// 1. Toggle between temporary and persistent mode
			if (CurrentSelectedActorInRoom == FoundActor)
			{
				TogglePersistance(TempArray, PersistArray);
			}
			// 2. Set active selection. Check if current selection is part of current room OR initial selection of current room
			else if (CurrentSelectedActorInRoom && CurrentSelectedActorInRoom->GetAttachParentActor() == FoundActor->GetAttachParentActor() ||
				TempArray.Contains(FoundActor) || PersistArray.Contains(FoundActor))
			{
				DeselectPriorActor(TempArray, PersistArray);
				SetSelectedActorInRoom(TempArray, PersistArray, ActiveRoom, FoundActor);
			}
			// 3. Switch selection to new room
			else
			{
				DeselectPriorActor(TempArray, PersistArray);
				ResetRoomEditMode(EditMode);

				if (TObjectPtr<APRG_Room> ChangeRoom = static_cast<APRG_Room*>(FoundActor->GetAttachParentActor()))
				{
					SetCurrentRoom(ChangeRoom);
					SetRoomEditMode();

					// When changing rooms, the passed temp and persist arrays will be outdated, so get them fresh
					if constexpr (std::is_same<T, AWall>())
						SetSelectedActorInRoom(TempWalls, ChangeRoom->GetWalls(), ChangeRoom, FoundActor);
					else if constexpr (std::is_same<T, ATile>())
						SetSelectedActorInRoom(TempTiles, ChangeRoom->GetTiles(), ChangeRoom, FoundActor);
				}
			}
		}
	}

	// Toggle current actor persistance state, switching between temp and peristent arrays
	template <class T>
	void TogglePersistance(TArray<TObjectPtr<T>>& TempArray, TArray<TObjectPtr<T>>& PersistArray)
	{
		if (TObjectPtr<T> ToggleActor = static_cast<T*>(CurrentSelectedActorInRoom))
		{
			int FoundIndex;
			if (TempArray.Find(ToggleActor, FoundIndex))
			{
				PersistArray[FoundIndex] = ToggleActor;
				TempArray[FoundIndex] = nullptr;
				SetMaterial(ToggleActor, Properties->PersistSelectedMat);
			}
			else if (PersistArray.Find(ToggleActor, FoundIndex))
			{
				TempArray[FoundIndex] = ToggleActor;
				PersistArray[FoundIndex] = nullptr;
				SetMaterial(ToggleActor, Properties->TempSelectedMat);
			}
		}
	}

	// Deselect previous actor, changing the material to appropriate UnselectedMat
	template <class T>
	void DeselectPriorActor(const TArray<TObjectPtr<T>>& TempArray, const TArray<TObjectPtr<T>>& PersistArray)
	{
		if (TObjectPtr<T> DeselectActor = static_cast<T*>(CurrentSelectedActorInRoom))
		{
			if (TempArray.Contains(DeselectActor))
				SetMaterial(DeselectActor, Properties->TempUnselectedMat);
			else if (PersistArray.Contains(DeselectActor))
				SetMaterial(DeselectActor, Properties->PersistUnselectedMat);
		}
	}

	// Set current actor as selected and changes material to appropriate SelectedMat
	template <class T>
	void SetSelectedActorInRoom(const TArray<TObjectPtr<T>>& TempArray, const TArray<TObjectPtr<T>>& PersistArray, const TObjectPtr<APRG_Room> ActiveRoom, TObjectPtr<T> SetActor)
	{
		if (TempArray.Contains(SetActor))
			SetMaterial(SetActor, Properties->TempSelectedMat);
		else if (ActiveRoom)
		{
			if (PersistArray.Contains(SetActor))
				SetMaterial(SetActor, Properties->PersistSelectedMat);
		}
		CurrentSelectedActorInRoom = SetActor;

		if constexpr (std::is_same<T, AWall>())
			Properties->WallMesh = CurrentSelectedActorInRoom->GetStaticMeshComponent()->GetStaticMesh();
		else if constexpr (std::is_same<T, ATile>())
			Properties->FloorMesh = CurrentSelectedActorInRoom->GetStaticMeshComponent()->GetStaticMesh();
	}

	// Set Room EditMode. Change material on persistent and temporary actors. Spawns temporary actors if needed
	template <class T, typename FPtrSpawn, typename FPtrGetPos>
	void SetRoomEditModeT(TArray<TObjectPtr<T>>& TempArray, TArray<TObjectPtr<T>>& PersistArray, FPtrSpawn SpawnFunc, FPtrGetPos GetPosFunc)
	{
		if (auto ActiveRoom = TryFetchCurrentRoom())
		{
			TempArray.SetNum(PersistArray.Num());

			for (int i = 0; i < PersistArray.Num(); i++)
			{
				// Change existing actor material
				if (PersistArray[i])
					SetMaterial(PersistArray[i], Properties->PersistUnselectedMat);
				// Check to spawn temporary actor and change material
				else
				{
					if (!TempArray[i])
					{
						// Get position from GetPosFunc, then call SpawnFunc to create actor
						TempArray[i] = (this->*SpawnFunc)(*ActiveRoom, i, (ActiveRoom->*GetPosFunc)(i, TileSizeCM));
					}
					SetMaterial(TempArray[i], Properties->TempUnselectedMat);
				}
			}
		}
	}

protected:
	UPROPERTY()
	TObjectPtr<UPRG_PluginRoomToolProperties> Properties;
	UPROPERTY()
	TObjectPtr<UCombinedTransformGizmo> TransformGizmo = nullptr;
	UPROPERTY()
	TObjectPtr<UTransformProxy> TransformProxy = nullptr;

	/** target World we will raycast into to find actors */
	UWorld* TargetWorld;

private:
	// Duplicate of RoomArray. Required to handle changes in OnPropertyModified 
	TArray<TObjectPtr<APRG_Room>> RoomArrayCopy;
	// Array of possible temporary walls used in EditMode::EditWalls
	TArray<TObjectPtr<AWall>> TempWalls;
	// Array of possible temporary tiles used in EditMode::EditTiles
	TArray<TObjectPtr<ATile>> TempTiles;
	// Array of temporary stored Materials used in EditMode::EditWalls or EditMode::EditTiles
	TArray<TObjectPtr<UMaterial>> StoredMaterials;

	// Currently active Room
	TObjectPtr<APRG_Room> CurrentRoom;
	// Bounding box of active Room
	TObjectPtr<ARoomBounds> CurrentBoundingBox;
	// Currently active Tile or Wall StaticMeshActor
	TObjectPtr<AStaticMeshActor> CurrentSelectedActorInRoom;

	// Prior EditMode. Required to handle changes in OnPropertyModified 
	EEditMode PrevEditMode;
	// RoomArray size. Required to handle changes in OnPropertyModified
	int RoomArraySize;

	// Separates UI in meters from internal calculations requiring more precision
	int TileSizeCM;
};


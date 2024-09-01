// Copyright 2022 Steven Weijden

#pragma once

#include "CoreMinimal.h"

#include "UObject/NoExportTypes.h"
#include "InteractiveToolBuilder.h"
#include "BaseTools/SingleClickTool.h"
#include "GameFramework/Actor.h"
#include <PRG_Room.h>

#include "PRG_PluginRoomTool.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPRGTool, Log, All);

class UCombinedTransformGizmo;
class UInteractiveGizmo;
class UTransformProxy;
class APRG_Settings;

UENUM()
enum class EEditMode : uint8
{
	CreateRooms,	// View content without any overlays.
	ManageRooms,	// Manage Rooms. OnClick sets the spawn point for new rooms
	EditWalls,		// Allow changing of walls in rooms. OnClick highlights selected.
	EditTiles			// Allow changing of tiles in rooms. OnClick highlights selected.
};

UENUM()
enum class EPosSnap
{
	NoSnapping = 0,
	SnapX10 = 10,
	SnapX100 = 100,
	SnapX1000 = 1000
};

UENUM()
enum class ERotSnap
{
	NoSnapping = 0,
	SnapZ5 = 5,
	SnapZ15 = 15,
	SnapZ45 = 45
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
	UPROPERTY(EditAnywhere, Category = "Options", meta = (DisplayName = "Edit mode"))
	EEditMode EditMode;
	// Snap rooms to the selected intervals
	UPROPERTY(EditAnywhere, Category = "Options", meta = (DisplayName = "Position snapping (cm)"))
	EPosSnap PositionSnap;
	// Snap rooms to the selected rotations
	UPROPERTY(EditAnywhere, Category = "Options", meta = (DisplayName = "Rotation snapping (deg)"))
	ERotSnap RotationSnap;
	// Toggle visibility of room gizmo's
	UPROPERTY(EditAnywhere, Category = "Options", meta = (DisplayName = "Show All Gizmos"))
	bool ShowAllGizmos;
	// Reset the floor of a room
	UPROPERTY(EditAnywhere, Category = "Options|Reset/Clear Floor", meta = (DisplayName = "Reset Floor", EditCondition = "EditMode == EEditMode::ManageRooms"))
	bool ResetRoomFloor;
	// Reset the floor of a room
	UPROPERTY(EditAnywhere, Category = "Options|Reset/Clear Floor", meta = (DisplayName = "Clear Floor", EditCondition = "EditMode == EEditMode::ManageRooms"))
	bool ClearRoomFloor;
	// Reset the walls of a room
	UPROPERTY(EditAnywhere, Category = "Options|Reset/Clear Walls", meta = (DisplayName = "Reset Walls", EditCondition = "EditMode == EEditMode::ManageRooms"))
	bool ResetRoomWalls;
	// Reset the walls of a room
	UPROPERTY(EditAnywhere, Category = "Options|Reset/Clear Walls", meta = (DisplayName = "Clear Walls", EditCondition = "EditMode == EEditMode::ManageRooms"))
	bool ClearRoomWalls;

	// Initial room size on spawn
	UPROPERTY(EditAnywhere, Category = "Data", meta = (DisplayName = "Room tiles", NoResetToDefault, ClampMin = "1", ClampMax = "50", UIMin = "1", UIMax = "50", EditCondition = "EditMode == EEditMode::CreateRooms || EditMode == EEditMode::ManageRooms"))
	FIntPoint RoomSize;
	// Set scale of room gizmo's
	/*UPROPERTY(EditAnywhere, Category = Data, meta = (DisplayName = "Gizmos Scale", ClampMin = "0.05", ClampMax = "2.0", UIMin = "0.05", UIMax = "2.0"))
	float GizmoScale;*/
	// Coordinates of where a new room will be spawned
	UPROPERTY(EditAnywhere, Category = "Data|Spawn Room", meta = (DisplayName = "Spawn Position", EditCondition = "EditMode == EEditMode::CreateRooms"))
	FVector SpawnPosition;
	// Room height used for displaying the room selection box
	UPROPERTY(EditAnywhere, Category = "Data|Spawn Room", meta = (DisplayName = "Room height(m)", ClampMin = "1", ClampMax = "20", UIMin = "1", UIMax = "20", EditCondition = "EditMode == EEditMode::CreateRooms"))
	int InitHeight;
	// Size of each tile
	UPROPERTY(EditAnywhere, Category = "Data|Spawn Room", meta = (DisplayName = "Tile size (m)", ClampMin = "1", ClampMax = "100", UIMin = "1", UIMax = "10", EditCondition = "EditMode == EEditMode::CreateRooms"))
	int TileSize;
	// Mesh used to spawn new tiles with when spawning a new room
	UPROPERTY(EditAnywhere, Category = "Data|Objects", meta = (DisplayName = "Floor Object", EditCondition = "EditMode == EEditMode::CreateRooms || EditMode == EEditMode::ManageRooms || EditMode == EEditMode::EditTiles"))
	TObjectPtr<UStaticMesh> FloorMesh;
	// Mesh used to spawn new walls with when spawning a new room
	UPROPERTY(EditAnywhere, Category = "Data|Objects", meta = (DisplayName = "Wall Object", EditCondition = "EditMode == EEditMode::CreateRooms || EditMode == EEditMode::ManageRooms || EditMode == EEditMode::EditWalls"))
	TObjectPtr<UStaticMesh> WallMesh;

	// Provide option to select a room from the scene
	UPROPERTY(EditAnywhere, Category = Overview, meta = (DisplayName = "Selected Room", GetOptions = "GetRoomSelection", EditCondition = "EditMode == EEditMode::ManageRooms"))
	TObjectPtr<APRG_Room> RoomSelection;
	// Overview of all room actors in the scene
	UPROPERTY(EditAnywhere, Category = Overview, meta = (DisplayName = "Rooms", NoElementDuplicate, OnlyPlaceable, EditCondition = "EditMode == EEditMode::CreateRooms", EditFixedOrder), NoClear)
	TArray<TObjectPtr<APRG_Room>> RoomArray;

	// Default material used to revert to after edit mode
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

	UFUNCTION()
	TArray<APRG_Room*> GetRoomSelection() const
	{
		return RoomArray;
	}
};

/**
 * UPRG_PluginRoomTool is an Tool that allows the user to create and manage procedurally created rooms
 * Functionality changes depending on the selected edit mode
 */
UCLASS()
class PRG_PLUGIN_API UPRG_PluginRoomTool : public USingleClickTool
{
	GENERATED_BODY()

public:
	UPRG_PluginRoomTool();

	virtual void SetWorld(UWorld* World);

	// Setup tool data
	virtual void Setup() override;
	// Clean up tool data
	virtual void Shutdown(EToolShutdownType ShutdownType) override;

	virtual void OnTick(float DeltaTime) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;

	// Handle changes to properties of the tool
	virtual void OnPropertyModified(UObject* PropertySet, FProperty* Property);
	
	// Handle OnClick events in the scene
	virtual void OnClicked(const FInputDeviceRay& ClickPos);

protected:
	// Try to get the current or first room available
	TObjectPtr<APRG_Room> TryGetCurrentRoom();
	// Set current room and UI info
	void SetCurrentRoom(TObjectPtr<APRG_Room> setRoom);
	// Find any room objects in the scene and add them to the tool
	void FindRoomsInScene();
	// Create and store a new room
	void SpawnRoom();
	// Set room to have all floor tiles filled
	void SetRoomFloorDefault(TObjectPtr<APRG_Room> SetRoom);
	// Set room to have only exterior walls
	void SetRoomWallsDefault(TObjectPtr<APRG_Room> SetRoom);
	// Set room to have all floor tiles filled
	void ClearRoomFloor(TObjectPtr<APRG_Room> SetRoom);
	// Set room to have only exterior walls
	void ClearRoomWalls(TObjectPtr<APRG_Room> SetRoom);
	// Setup room found in the scene
	void SetupFoundRoom(TObjectPtr<APRG_Room> addRoom);
	// Change the size of the current room
	void ResizeRoom();
	// Handle deleting a room in the scene
	void DeleteRoomInScene(TObjectPtr<APRG_Room> DeletedRoom);
	// Delete a room from the tool and scene
	void DeleteRoom(TObjectPtr<APRG_Room> removeRoom);
	// Delete any temporary actors used for editing walls and floors
	void DeleteTempActors();

	// Create a room gizmo
	void CreateCustomRoomGizmo(TObjectPtr<APRG_Room> room, bool loadTransform);
	// Handle when a gizmo is moved. Listens to OnTransformChanged on TransformProxy
	void GizmoTransformChanged(UTransformProxy* Proxy, FTransform Transform);
	// Toggle visibility of room gizmo's
	//void SetGizmoScale(float Scale);
	// Toggle visibility of room gizmo's
	void ToggleGizmoVisibility(bool Visible);

	// Reset room from edit state. Deletes temporary actors and resets materials
	void ResetRoomEditMode(EEditMode EditMode);
	// Set room to current EditMode. Spawns appropriate temporary actors and changes material
	void SetRoomEditMode();
	// Set temporary material on given actor during editing
	void SetEditModeMaterial(TObjectPtr<AStaticMeshActor> Actor, TObjectPtr<UMaterial> Material);
	// Set array of materials to actor
	void SetArrayMaterials(TObjectPtr<AStaticMeshActor> Actor, TArray<TObjectPtr<UMaterial>> Materials);
	// Reset materials on all persistent actors for the given EditMode
	void ResetPersistMaterials(EEditMode EditMode);

private:
	// Spawn tile actor
	TObjectPtr<ATile> SpawnTile(APRG_Room& ParentRoom, int IndexInRoom, FVector SpawnPos);
	// Spawn wall actor and get rotation
	TObjectPtr<AWall> SpawnWall(APRG_Room& ParentRoom, int IndexInRoom, FVector SpawnPos);
	// Spawn wall actor with given rotation
	TObjectPtr<AWall> SpawnWallRot(APRG_Room& ParentRoom, FVector SpawnPos, FRotator SpawnRot);

	// Create a bounding box for the currently selected room
	void SpawnRoomBoundingBox();
	// Remove the bounding box for the currently selected room
	void RemoveRoomBoundingBox();

	void SpawnCreateRoomGizmo();
	void UpdateCreateRoomGizmo(FVector NewPos);
	void RemoveCreateRoomGizmo();

	void ResetToolState();

protected:
	// Handle edit mode interaction with valid actors OnClick
	template <class T>
	void OnClickEditModeInteraction(EEditMode EditMode, FHitResult& TraceResult, TArray<TObjectPtr<T>>& TempArray, TArray<TObjectPtr<T>>& PersistArray)
	{
		if (auto ActiveRoom = TryGetCurrentRoom())
		{
			TObjectPtr<T> FoundActor = static_cast<T*>(TraceResult.GetActor());

			// 1. Toggle selected object between temporary and persistent arrays
			if (CurrentSelectedActorInRoom.Value == FoundActor)
			{
				TogglePersistance(TempArray, PersistArray);
			}
			// 2. Set selected object to traced object if of matching type. Check if current selection is part of current room OR initial selection of current room
			else if (CurrentSelectedActorInRoom.Value && CurrentSelectedActorInRoom.Value->GetAttachParentActor() == FoundActor->GetAttachParentActor() ||
				TempArray.Contains(FoundActor) || PersistArray.Contains(FoundActor))
			{
				DeselectPriorActor(TempArray, PersistArray);
				SetSelectedActorInRoom(TempArray, PersistArray, ActiveRoom, FoundActor);
			}
			// 3. Switch selection to new room when trace hits object of an unselected room
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

	// Toggle current actor persistance state, switching selected object between temp and peristent arrays
	template <class T>
	void TogglePersistance(TArray<TObjectPtr<T>>& TempArray, TArray<TObjectPtr<T>>& PersistArray)
	{
		if (TObjectPtr<T> ToggleActor = static_cast<T*>(CurrentSelectedActorInRoom.Value))
		{
			int FoundIndex;
			if (TempArray.Find(ToggleActor, FoundIndex))
			{
				PersistArray[FoundIndex] = ToggleActor;
				TempArray[FoundIndex] = nullptr;
				SetEditModeMaterial(ToggleActor, Properties->PersistSelectedMat);
			}
			else if (PersistArray.Find(ToggleActor, FoundIndex))
			{
				TempArray[FoundIndex] = ToggleActor;
				PersistArray[FoundIndex] = nullptr;
				SetEditModeMaterial(ToggleActor, Properties->TempSelectedMat);
			}
		}
	}

	// Deselect previous actor, changing the material to appropriate UnselectedMat
	template <class T>
	void DeselectPriorActor(const TArray<TObjectPtr<T>>& TempArray, const TArray<TObjectPtr<T>>& PersistArray)
	{
		if (TObjectPtr<T> DeselectActor = static_cast<T*>(CurrentSelectedActorInRoom.Value))
		{
			if (TempArray.Contains(DeselectActor))
				SetEditModeMaterial(DeselectActor, Properties->TempUnselectedMat);
			else if (PersistArray.Contains(DeselectActor))
				SetEditModeMaterial(DeselectActor, Properties->PersistUnselectedMat);
		}
	}

	// Set current actor as selected and changes material to appropriate SelectedMat
	template <class T>
	void SetSelectedActorInRoom(const TArray<TObjectPtr<T>>& TempArray, const TArray<TObjectPtr<T>>& PersistArray, const TObjectPtr<APRG_Room> ActiveRoom, TObjectPtr<T> SetActor)
	{
		int FoundIndex = -1;
		if (TempArray.Find(SetActor, FoundIndex))
			SetEditModeMaterial(SetActor, Properties->TempSelectedMat);
		else if (ActiveRoom)
		{
			if (PersistArray.Find(SetActor, FoundIndex))
				SetEditModeMaterial(SetActor, Properties->PersistSelectedMat);
		}
		CurrentSelectedActorInRoom = { FoundIndex, SetActor };

		if constexpr (std::is_same<T, AWall>())
			Properties->WallMesh = SetActor->GetStaticMeshComponent()->GetStaticMesh();
		else if constexpr (std::is_same<T, ATile>())
			Properties->FloorMesh = SetActor->GetStaticMeshComponent()->GetStaticMesh();
	}

	// Set Room EditMode and set/update materials on persistent and temporary actors. Spawns temporary actors if needed
	template <class T, typename FPtrSpawn, typename FPtrGetPos>
	void SetEditModeMaterials(TArray<TObjectPtr<T>>& TempArray, TArray<TObjectPtr<T>>& PersistArray, FPtrSpawn SpawnFunc, FPtrGetPos GetPosFunc)
	{
		if constexpr (std::is_base_of<AStaticMeshActor, T>::value)
		{
			// Lambda - Store original materials
			auto StoreOriginalMats = [&](TObjectPtr<T>& StaticMesh, int index, TObjectPtr<UMaterial> Material)
			{
				TArray<UMaterialInterface*> OutMaterials;
				StaticMesh->GetStaticMeshComponent()->GetUsedMaterials(OutMaterials);
				for (int j = 0; j < OutMaterials.Num(); j++)
					OriginalMaterials[index].Add(OutMaterials[j]->GetMaterial());

				SetEditModeMaterial(StaticMesh, Material);
			};

			// Call lambda for correct array
			if (auto ActiveRoom = TryGetCurrentRoom())
			{
				// Reset and resize array of original materials
				for (int i = 0; i < OriginalMaterials.Num(); i++)
					OriginalMaterials[i].Empty();
				OriginalMaterials.Empty();
				OriginalMaterials.SetNum(PersistArray.Num());

				TempArray.SetNum(PersistArray.Num());
				for (int i = 0; i < PersistArray.Num(); i++)
				{
					// Store original materials and change existing material
					if (PersistArray[i])
					{
						if (!PersistArray[i]->GetStaticMeshComponent())
						{
							// This can happen if a static mesh was manually deleted in the scene while the tool was open
							UE_LOG(LogPRGTool, Warning, TEXT("Missing static mesh component detected. Adding TempArray actor to recover internal state."));

							PersistArray[i] = nullptr;
							// Get position from GetPosFunc, then call SpawnFunc to create actor
							TempArray[i] = (this->*SpawnFunc)(*ActiveRoom, i, (ActiveRoom->*GetPosFunc)(i, TileSizeCM));
							StoreOriginalMats(TempArray[i], i, Properties->TempUnselectedMat);
							continue;
						}

						StoreOriginalMats(PersistArray[i], i, Properties->PersistUnselectedMat);
					}
					// Check to spawn temporary actor and change material
					else
					{
						if (!TempArray[i])
						{
							// Get position from GetPosFunc, then call SpawnFunc to create actor
							TempArray[i] = (this->*SpawnFunc)(*ActiveRoom, i, (ActiveRoom->*GetPosFunc)(i, TileSizeCM));
						}
						StoreOriginalMats(TempArray[i], i, Properties->TempUnselectedMat);
					}
				}
			}
		}
	}

protected:
	UPROPERTY()
	TObjectPtr<UPRG_PluginRoomToolProperties> Properties = nullptr;
	UPROPERTY()
	TObjectPtr<UCombinedTransformGizmo> TransformGizmo = nullptr;
	UPROPERTY()
	TObjectPtr<UTransformProxy> TransformProxy = nullptr;
	UPROPERTY()
	TObjectPtr<UCombinedTransformGizmo> SpawnGizmo = nullptr;
	UPROPERTY()
	TObjectPtr<UTransformProxy> SpawnProxy = nullptr;

	/** Target World we will raycast into to find actors */
	UWorld* TargetWorld = nullptr;

private:
	// Settings file
	TObjectPtr<APRG_Settings> PRGSettings;
	// Duplicate of RoomArray. Required to handle changes in OnPropertyModified 
	TArray<TObjectPtr<APRG_Room>> RoomArrayCopy;
	// Array of possible temporary walls used in EditMode::EditWalls
	TArray<TObjectPtr<AWall>> TempWalls;
	// Array of possible temporary tiles used in EditMode::EditTiles
	TArray<TObjectPtr<ATile>> TempTiles;
	// Array of stored original Materials used in EditMode::EditWalls or EditMode::EditTiles
	TArray<TArray<TObjectPtr<UMaterial>>> OriginalMaterials;

	// Room used with OriginalMaterials
	TObjectPtr<APRG_Room> OriginMatRoom = nullptr;
	// Last active Room
	TObjectPtr<APRG_Room> LastActiveRoom = nullptr;
	// Currently active Room
	TObjectPtr<APRG_Room> CurrentRoom = nullptr;
	// Bounding box of active Room
	TObjectPtr<ARoomBounds> CurrentBoundingBox = nullptr;
	// Currently active Tile or Wall StaticMeshActor
	TPair<int, TObjectPtr<AStaticMeshActor>> CurrentSelectedActorInRoom;

	// Prior EditMode. Required to handle changes in OnPropertyModified 
	EEditMode PrevEditMode = EEditMode::CreateRooms;
	// RoomArray size. Required to handle changes in OnPropertyModified
	int RoomArraySize = 0;

	// Tile size internal. Separates UI in meters from internal calculations requiring more precision
	int TileSizeCM = 200;
};


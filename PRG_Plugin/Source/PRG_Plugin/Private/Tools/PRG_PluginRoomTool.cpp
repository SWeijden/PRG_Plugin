// Copyright 2022 Steven Weijden

#include "PRG_PluginRoomTool.h"
#include <PRG_Settings.h>
#include "InteractiveToolManager.h"
#include "ToolBuilderUtil.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "BaseGizmos/TransformGizmoUtil.h"
#include "UObject/ConstructorHelpers.h"
#include "Editor.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "Kismet/GameplayStatics.h"

// localization namespace
#define LOCTEXT_NAMESPACE "UPRG_PluginRoomTool"

DEFINE_LOG_CATEGORY(LogPRGTool);

ARoomBounds::ARoomBounds()
{
	CubeMesh = ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'")).Object;
	CubeMaterial = ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("/PRG_Plugin/Materials/Mat_RoomBounds.Mat_RoomBounds")).Object;
}

/*
 * ToolBuilder implementation
 */

UInteractiveTool* UPRG_PluginRoomToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	UPRG_PluginRoomTool* NewTool = NewObject<UPRG_PluginRoomTool>(SceneState.ToolManager);
	NewTool->SetWorld(SceneState.World);
	return NewTool;
}

const FToolTargetTypeRequirements& UPRG_PluginRoomToolBuilder::GetTargetRequirements() const
{
	static FToolTargetTypeRequirements TypeRequirements({});
	return TypeRequirements;
}

/*
 * ToolProperties implementation
 */

UPRG_PluginRoomToolProperties::UPRG_PluginRoomToolProperties()
{
	EditMode = EEditMode::CreateRooms;
	PositionSnap = EPosSnap::SnapX10;
	RoomSize = { 3, 2 };
	ShowAllGizmos = true;
	ResetRoomFloor = false;
	ClearRoomFloor = false;
	ResetRoomWalls = false;
	ClearRoomWalls = false;
	//GizmoScale = 1.0f;
	InitHeight = 2;
	TileSize = 2;

	// Set default values for objects
	FloorMesh							= ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("/PRG_Plugin/Meshes/SM_PRG_Floor.SM_PRG_Floor")).Object;
	WallMesh							= ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("/PRG_Plugin/Meshes/SM_PRG_Wall.SM_PRG_Wall")).Object;
	DefaultMat						= ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("/PRG_Plugin/Materials/Mat_Default.Mat_Default")).Object;
	PersistSelectedMat		= ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("/PRG_Plugin/Materials/Mat_PersistSelected.Mat_PersistSelected")).Object;
	PersistUnselectedMat	= ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("/PRG_Plugin/Materials/Mat_PersistUnselected.Mat_PersistUnselected")).Object;
	TempSelectedMat				= ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("/PRG_Plugin/Materials/Mat_TempSelected.Mat_TempSelected")).Object;
	TempUnselectedMat			= ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("/PRG_Plugin/Materials/Mat_TempUnselected.Mat_TempUnselected")).Object;
}

/*
 * Tool implementation
 */

// ***************************************************************************************************
// ******************************** PUBLIC FUNCTIONS *************************************************
// ***************************************************************************************************

UPRG_PluginRoomTool::UPRG_PluginRoomTool()
{
	ResetToolState();
}

void UPRG_PluginRoomTool::SetWorld(UWorld* World)
{
	this->TargetWorld = World;
}

void UPRG_PluginRoomTool::Setup()
{
	USingleClickTool::Setup();

	Properties = NewObject<UPRG_PluginRoomToolProperties>(this);
	AddToolPropertySource(Properties);

	// Find and load or spawn APRG_Settings object
	if (TargetWorld)
	{
		if (AActor* SettingsActor = UGameplayStatics::GetActorOfClass(TargetWorld, APRG_Settings::StaticClass()))
		{
			PRGSettings = static_cast<APRG_Settings*>(SettingsActor);
			Properties->PositionSnap		= PRGSettings->PositionSnap;
			Properties->RotationSnap		= PRGSettings->RotationSnap;
			Properties->RoomSize				= PRGSettings->RoomSize;
			Properties->ShowAllGizmos		= PRGSettings->ShowAllGizmos;
			//Properties->GizmoScale			= PRGSettings->GizmoScale;
			Properties->InitHeight			= PRGSettings->InitHeight;
			Properties->TileSize				= PRGSettings->TileSize;
			Properties->SpawnPosition		= PRGSettings->SpawnPosition;
			Properties->FloorMesh				= PRGSettings->FloorMesh;
			Properties->WallMesh				= PRGSettings->WallMesh;
		}
		else
		{
			const FTransform SpawnLocAndRotation = FTransform(FRotator(0.0f, 0.0f, 0.0f), Properties->SpawnPosition);
			PRGSettings = TargetWorld->SpawnActorDeferred<APRG_Settings>(APRG_Settings::StaticClass(), SpawnLocAndRotation);
			PRGSettings->InitSettings(Properties->PositionSnap, Properties->RotationSnap, Properties->RoomSize, 
				Properties->ShowAllGizmos, /*Properties->GizmoScale,*/ Properties->InitHeight,Properties->TileSize,
				Properties->SpawnPosition, Properties->FloorMesh, Properties->WallMesh);
			PRGSettings->FinishSpawning(SpawnLocAndRotation);
		}
	}

	FindRoomsInScene();
	ToggleGizmoVisibility(Properties->ShowAllGizmos);

	SpawnCreateRoomGizmo();
}

void UPRG_PluginRoomTool::Shutdown(EToolShutdownType ShutdownType)
{
	DeleteTempActors();
	ResetPersistMaterials(Properties->EditMode);

	GetToolManager()->GetPairedGizmoManager()->DestroyAllGizmosByOwner(this);

	// Clear delegates of rooms
	for (APRG_Room* FoundRoom : Properties->RoomArray)
	{
		if (FoundRoom)
		{
			FoundRoom->CleanupRoom();
			FoundRoom->OnRoomDeletion.Unbind();
		}
	}

	ResetToolState();

	Properties->RoomArray.Empty();
	RoomArrayCopy.Empty();
}

void UPRG_PluginRoomTool::OnTick(float DeltaTime)
{
}

void UPRG_PluginRoomTool::Render(IToolsContextRenderAPI* RenderAPI)
{
}

void UPRG_PluginRoomTool::OnPropertyModified(UObject* PropertySet, FProperty* Property)
{
	// Enum - EditMode, PositionSnap, RotationSnap
	if (Property->IsA(FEnumProperty::StaticClass()))
	{
		if (Property->GetFName() == "EditMode")
		{
			ResetRoomEditMode(PrevEditMode);
			SetRoomEditMode();
		}
		else if (Property->GetFName() == "PositionSnap")
		{
			PRGSettings->PositionSnap = Properties->PositionSnap;
			PRGSettings->MarkPackageDirty();
		}
		else if (Property->GetFName() == "RotationSnap")
		{
			PRGSettings->RotationSnap = Properties->RotationSnap;
			PRGSettings->MarkPackageDirty();
		}
	}
	// Struct - SpawnPosition
	else if (Property->IsA(FStructProperty::StaticClass()))
	{
		if (Property->GetFName() == "SpawnPosition")
		{
			PRGSettings->SpawnPosition = Properties->SpawnPosition;
			PRGSettings->MarkPackageDirty();
		}
	}
	// Bool - ShowAllGizmos, ResetRoomFloor, ClearRoomFloor, ResetRoomWalls, ClearRoomWalls
	else if (Property->IsA(FBoolProperty::StaticClass()))
	{
		if (Property->GetFName() == "ShowAllGizmos")
		{
			ToggleGizmoVisibility(Properties->ShowAllGizmos);
			PRGSettings->ShowAllGizmos = Properties->ShowAllGizmos;
			PRGSettings->MarkPackageDirty();
		}
		else if (Property->GetFName() == "ResetRoomFloor")
		{
			// Only reset with an already selected current room
			if (CurrentRoom && Properties->ResetRoomFloor)
			{
				ClearRoomFloor(CurrentRoom);
				SetRoomFloorDefault(CurrentRoom);
			}
			Properties->ResetRoomFloor = false;
		}
		else if (Property->GetFName() == "ClearRoomFloor")
		{
			// Only reset with an already selected current room
			if (CurrentRoom && Properties->ClearRoomFloor)
				ClearRoomFloor(CurrentRoom);

			Properties->ClearRoomFloor = false;
		}
		else if (Property->GetFName() == "ResetRoomWalls")
		{
			// Only reset with an already selected current room
			if (CurrentRoom && Properties->ResetRoomWalls)
			{
				ClearRoomWalls(CurrentRoom);
				SetRoomWallsDefault(CurrentRoom);
			}
			Properties->ResetRoomWalls = false;
		}
		else if (Property->GetFName() == "ClearRoomWalls")
		{
			// Only reset with an already selected current room
			if (CurrentRoom && Properties->ClearRoomWalls)
				ClearRoomWalls(CurrentRoom);

			Properties->ClearRoomWalls = false;
		}
	}
	// Int - RoomSize (X,Y), TileSize, InitHeight
	else if (Property->IsA(FIntProperty::StaticClass()))
	{
		if (Property->GetFName() == "X" || Property->GetFName() == "Y")
		{
			PRGSettings->RoomSize = Properties->RoomSize;
			PRGSettings->MarkPackageDirty();
			ResizeRoom();
		}
		else if (Property->GetFName() == "TileSize")
		{
			TileSizeCM = Properties->TileSize * 100;
			PRGSettings->TileSize = Properties->TileSize;
			PRGSettings->MarkPackageDirty();
		}
		else if (Property->GetFName() == "InitHeight")
		{
			PRGSettings->InitHeight = Properties->InitHeight;
			PRGSettings->MarkPackageDirty();
		}
	}
	// Float - GizmoScale
	/*else if (Property->IsA(FFloatProperty::StaticClass()))
	{
		if (Property->GetFName() == "GizmoScale")
		{
			SetGizmoScale(Properties->GizmoScale);
			PRGSettings->GizmoScale = Properties->GizmoScale;
			PRGSettings->MarkPackageDirty();
		}
	}*/
	// UObject - StaticMesh
	else if (Property->IsA(FObjectProperty::StaticClass()))
	{
		// Handle RoomSelection dropdown
		if (Property->GetFName() == "RoomSelection")
		{
			const FObjectProperty* ObjectProperty = static_cast<FObjectProperty*>(Property);
			if (APRG_Room* SelectedRoom = static_cast<APRG_Room*>(ObjectProperty->GetPropertyValue_InContainer(PropertySet)))
			{
				if (!SelectedRoom->IsPendingKill())
					SetCurrentRoom(SelectedRoom);
			}
		}
		// Apply a new static mesh to CurrentSelectedActorInRoom
		else if (CurrentSelectedActorInRoom.Value)
		{
			const FObjectProperty* ObjectProperty = static_cast<FObjectProperty*>(Property);
			if (UStaticMesh* StaticMesh = static_cast<UStaticMesh*>(ObjectProperty->GetPropertyValue_InContainer(PropertySet)))
			{
				// Store original materials of new static mesh
				OriginalMaterials[CurrentSelectedActorInRoom.Key].Empty();
				auto& OutMaterials = StaticMesh->GetStaticMaterials();
				for (int j = 0; j < OutMaterials.Num(); j++)
					OriginalMaterials[CurrentSelectedActorInRoom.Key].Add(OutMaterials[j].MaterialInterface->GetMaterial());

				// Assign new static mesh
				if (Property->GetFName() == "WallMesh" && Properties->EditMode == EEditMode::EditWalls)
					CurrentSelectedActorInRoom.Value->GetStaticMeshComponent()->SetStaticMesh(StaticMesh);
				else if (Property->GetFName() == "FloorMesh" && Properties->EditMode == EEditMode::EditTiles)
					CurrentSelectedActorInRoom.Value->GetStaticMeshComponent()->SetStaticMesh(StaticMesh);
			}
		}
		// Set a new static mesh as default
		else
		{
			if (Property->GetFName() == "WallMesh")
			{
				PRGSettings->WallMesh = Properties->WallMesh;
				PRGSettings->MarkPackageDirty();
			}
			else if (Property->GetFName() == "FloorMesh")
			{
				PRGSettings->FloorMesh = Properties->FloorMesh;
				PRGSettings->MarkPackageDirty();
			}
		}

	}
	// Array - SpawnPosition, RoomArray
	else if (Property->IsA(FArrayProperty::StaticClass()))
	{
		// Handle interactions with RoomArray
		if (Property->GetFName() == "RoomArray")
		{
			// 1. Delete all Rooms
			if (Properties->RoomArray.Num() == 0)
			{
				for (size_t i = RoomArrayCopy.Num(); i > 0; i--)
					DeleteRoom(RoomArrayCopy[i-1]);
			}
			// 2. Delete a Removed or cleared ArrayRoom entry
			else if (RoomArraySize >= Properties->RoomArray.Num())
			{
				for (APRG_Room* RoomCopy : RoomArrayCopy)
				{
					if (!Properties->RoomArray.Contains(RoomCopy))
					{
						DeleteRoom(RoomCopy);
						break;
					}
				}
			}
			// 3. Added Room
			else if (RoomArraySize < Properties->RoomArray.Num())
				SpawnRoom();
		}
	}
}

void UPRG_PluginRoomTool::OnClicked(const FInputDeviceRay& ClickPos)
{
	// Trace a ray into the World
	FCollisionObjectQueryParams QueryParams(FCollisionObjectQueryParams::AllObjects);
	FHitResult Result;
	bool bHitWorld = TargetWorld->LineTraceSingleByObjectType(Result, ClickPos.WorldRay.Origin, ClickPos.WorldRay.PointAt(999999), QueryParams);
	if (bHitWorld)
	{
		// Handle trace based on current EditMode
		switch (Properties->EditMode)
		{
		// Set spawn position for new room
		case EEditMode::CreateRooms:
			Properties->SpawnPosition = Result.ImpactPoint;
			PRGSettings->SpawnPosition = Properties->SpawnPosition;
			UpdateCreateRoomGizmo(Properties->SpawnPosition);
			break;

		// Select a room when clicking on a child of a room
		case EEditMode::ManageRooms:
			if (APRG_Room* Room = static_cast<APRG_Room*>(Result.GetActor()->GetAttachParentActor()))
				SetCurrentRoom(Room);
			break;

		// Edit walls in viewport
		case EEditMode::EditWalls:
			if (Result.GetActor()->IsA(AWall::StaticClass()))
				OnClickEditModeInteraction(EEditMode::EditWalls, Result, TempWalls, CurrentRoom->GetWalls());
			break;

		// Edit tiles in viewport
		case EEditMode::EditTiles:
			if (Result.GetActor()->IsA(ATile::StaticClass()))
				OnClickEditModeInteraction(EEditMode::EditTiles, Result, TempTiles, CurrentRoom->GetTiles());
			break;

		// Ignore input for EEditMode::ManageRooms
		default:
			break;
		}
	}
}

// ***************************************************************************************************
// ******************************** PROTECTED FUNCTIONS **********************************************
// ***************************************************************************************************

// ********************************** Room Functions *************************************************

TObjectPtr<APRG_Room> UPRG_PluginRoomTool::TryGetCurrentRoom()
{
	if (CurrentRoom && CurrentRoom->IsPendingKill())
		CurrentRoom = nullptr;

	// If no current room is set
	if (CurrentRoom == nullptr)
	{
		// Try to recover last room selection
		if (LastActiveRoom && !LastActiveRoom->IsPendingKill())
			CurrentRoom = LastActiveRoom;
		// Take first entry in room array, if valid
		else if (Properties->RoomArray.Num() > 0)
		{
			if (Properties->RoomArray[0])
			{
				CurrentRoom = Properties->RoomArray[0];
				CurrentRoom->InitRoom();
				SetCurrentRoom(CurrentRoom);
			}
		}
	}

	// Can still be nullptr
	return CurrentRoom;
}

void UPRG_PluginRoomTool::SetCurrentRoom(TObjectPtr<APRG_Room> SetRoom)
{
	// Reset current room state
	if (CurrentRoom && !CurrentRoom->IsPendingKill())
	{
		// Validate room state. Room will be in an invalid state when undoing/redoing a room deletion or creation, which is not (yet) supported.
		if (!CurrentRoom->GetRoomGizmo())
		{
			checkf(false, TEXT("Room in invalid state! Exit the tool and reopen to recover internal state."));
			return;
		}

		if (CurrentRoom != SetRoom)
			RemoveRoomBoundingBox();

		CurrentRoom->GetRoomGizmo()->SetVisibility(Properties->ShowAllGizmos);

		LastActiveRoom = CurrentRoom;
	}
	else
		LastActiveRoom = nullptr;

	CurrentRoom = SetRoom;

	// Set new room state
	if (SetRoom)
	{
		// Validate room state. Room will be in an invalid state when undoing/redoing a room deletion or creation, which is not (yet) supported.
		if (!SetRoom->GetRoomGizmo())
		{
			checkf(false, TEXT("Room in invalid state! Exit the tool and reopen to recover internal state."));
			return;
		}

		if (Properties->EditMode == EEditMode::ManageRooms)
		{
			SpawnRoomBoundingBox();
			Properties->RoomSize = SetRoom->GetRoomSize();
			Properties->InitHeight = SetRoom->GetRoomHeight();
		}

		SetRoom->GetRoomGizmo()->SetVisibility(Properties->EditMode != EEditMode::CreateRooms || Properties->ShowAllGizmos);
		Properties->RoomSelection = SetRoom;

#if WITH_EDITOR
		GEditor->SelectNone(false, true, false);
		GEditor->SelectActor(SetRoom, true, false, false, true);
#endif
	}
}

void UPRG_PluginRoomTool::FindRoomsInScene()
{
	if (TargetWorld)
	{
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(TargetWorld, APRG_Room::StaticClass(), FoundActors);

		for (AActor* Actor : FoundActors)
		{
			APRG_Room* Room = static_cast<APRG_Room*>(Actor);
			SetupFoundRoom(Room);
		}
	}
}

void UPRG_PluginRoomTool::SpawnRoom()
{
	// Validate room array size to catch if something changed with the array that allows breaking the internal state.
	if (RoomArraySize != Properties->RoomArray.Num() - 1)
	{
		checkf(false, TEXT("RoomArraySize mismatching Properties->RoomArray! Exit the tool and reopen to recover internal state."));
		return;
	}

	// Spawn new room object
	const FTransform SpawnLocAndRotation = FTransform(FRotator(0.0f, 0.0f, 0.0f), Properties->SpawnPosition);
	TObjectPtr<APRG_Room> NewRoom = TargetWorld->SpawnActorDeferred<APRG_Room>(APRG_Room::StaticClass(), SpawnLocAndRotation);
	NewRoom->InitRoom(Properties->RoomSize, Properties->InitHeight);
	NewRoom->FinishSpawning(SpawnLocAndRotation);
	NewRoom->OnRoomDeletion.BindUObject(this, &UPRG_PluginRoomTool::DeleteRoomInScene);
	CreateCustomRoomGizmo(NewRoom, false);

	// Store room in arrays
	int EmptyIndex = Properties->RoomArray.Find(nullptr);
	if (EmptyIndex != INDEX_NONE)
		Properties->RoomArray[EmptyIndex] = NewRoom;
	else
		Properties->RoomArray[RoomArraySize] = NewRoom;
	RoomArrayCopy.Add(NewRoom);

	SetCurrentRoom(NewRoom);
	RoomArraySize = Properties->RoomArray.Num();

	SetRoomFloorDefault(NewRoom);
	SetRoomWallsDefault(NewRoom);
}

void UPRG_PluginRoomTool::SetRoomFloorDefault(TObjectPtr<APRG_Room> SetRoom)
{
	int SetIndex = 0;
	float OffsetX = 0, OffsetY = 0;

	// Columns
	for (size_t iY = 0; iY < Properties->RoomSize.Y; iY++)
	{
		OffsetY = 0.5f * TileSizeCM + TileSizeCM * iY;

		// Rows
		for (size_t iX = 0; iX < Properties->RoomSize.X; iX++)
		{
			OffsetX = 0.5f * TileSizeCM + TileSizeCM * iX;
			SetIndex = iX + iY * Properties->RoomSize.X;
			ATile* NewTile = SpawnTile(*SetRoom, SetIndex, FVector(OffsetX, OffsetY, 0.0f));
			SetRoom->SetTileAtIndex(SetIndex, NewTile);
		}
	}
}

void UPRG_PluginRoomTool::SetRoomWallsDefault(TObjectPtr<APRG_Room> SetRoom)
{
	int SetIndex = 0;
	float OffsetX = 0, OffsetY = 0;

	// Spawn initial room walls
	/* Index progression example on a 3x2 grid. First vertical walls, then horizontal walls:
	 *
	 *     0       1      2
	 *  9     10      11     12
	 *     3       4      5
	 * 13     14      15     16
	 *     6       7      8
	 */
	int AddIndex = Properties->RoomSize.X * (Properties->RoomSize.Y + 1);
	FRotator WallRotation = FRotator(0.0f, 0.0f, 0.0f);

	// Spawn X-aligned walls. Traverse X*(Y+1)
	// Increment by RoomSize.Y to only spawn the outer edges
	for (size_t iY = 0; iY <= Properties->RoomSize.Y; iY += Properties->RoomSize.Y)
	{
		OffsetY = TileSizeCM * iY;
		for (size_t iX = 0; iX < Properties->RoomSize.X; iX++)
		{
			OffsetX = 0.5f * TileSizeCM + TileSizeCM * iX;
			SetIndex = iX + iY * Properties->RoomSize.X;
			AWall* NewWall = SpawnWallRot(*SetRoom, FVector(OffsetX, OffsetY, 0.0f), WallRotation);
			SetRoom->SetWallAtIndex(SetIndex, NewWall);
		}
	}

	// Spawn Y-aligned walls. Traverse (X+1)*Y
	// Increment by RoomSize.X to only spawn the outer edges
	WallRotation = FRotator(0.0f, 90.0f, 0.0f);
	for (size_t iY = 0; iY < Properties->RoomSize.Y; iY++)
	{
		OffsetY = 0.5f * TileSizeCM + TileSizeCM * iY;
		for (size_t iX = 0; iX <= Properties->RoomSize.X; iX += Properties->RoomSize.X)
		{
			OffsetX = TileSizeCM * iX;
			SetIndex = AddIndex + iX + iY * (Properties->RoomSize.X + 1);
			AWall* NewWall = SpawnWallRot(*SetRoom, FVector(OffsetX, OffsetY, 0.0f), WallRotation);
			SetRoom->SetWallAtIndex(SetIndex, NewWall);
		}
	}
}

void UPRG_PluginRoomTool::ClearRoomFloor(TObjectPtr<APRG_Room> SetRoom)
{
	// Clear any old tiles
	TArray<TObjectPtr<ATile>>& OldTiles = SetRoom->GetTiles();
	for (size_t i = 0; i < OldTiles.Num(); i++)
	{
		if (OldTiles[i])
		{
			TargetWorld->DestroyActor(OldTiles[i]);
			OldTiles[i] = nullptr;
		}
	}
}

void UPRG_PluginRoomTool::ClearRoomWalls(TObjectPtr<APRG_Room> SetRoom)
{
	// Clear any old walls
	TArray<TObjectPtr<AWall>>& OldWalls = SetRoom->GetWalls();
	for (size_t i = 0; i < OldWalls.Num(); i++)
	{
		if (OldWalls[i])
		{
			TargetWorld->DestroyActor(OldWalls[i]);
			OldWalls[i] = nullptr;
		}
	}
}

void UPRG_PluginRoomTool::SetupFoundRoom(TObjectPtr<APRG_Room> FoundRoom)
{
	// Initialize room data
	FoundRoom->InitRoom();

	// Bind cleanup delegate
	FoundRoom->OnRoomDeletion.BindUObject(this, &UPRG_PluginRoomTool::DeleteRoomInScene);

	// Register tool with plugin variables
	RoomArrayCopy.Add(FoundRoom);
	Properties->RoomArray.Add(FoundRoom);
	RoomArraySize = Properties->RoomArray.Num();

	// Create a room gizmo
	CreateCustomRoomGizmo(FoundRoom, true);

	// Recreate room from map in room data
	TArray<AActor*> ChildActors;
	FoundRoom->GetAttachedActors(ChildActors);
	for (auto Child : ChildActors)
	{
		if (ATile* Tile = Cast<ATile>(Child))
			FoundRoom->SetTileAtIndex(FoundRoom->GetTileIndexByPosition(Tile->GetStaticMeshComponent()->GetRelativeLocation(), TileSizeCM), Tile);

		if (AWall* Wall = Cast<AWall>(Child))
			FoundRoom->SetWallAtIndex(FoundRoom->GetWallIndexByPosition(Wall->GetStaticMeshComponent()->GetRelativeLocation(), TileSizeCM), Wall);
	}
}

void UPRG_PluginRoomTool::ResizeRoom()
{
	/* INFO: Resizes the walls and tiles arrays
	 * Note that the UI allows changing only 1 axis at a time
	 * 
	 * If new size is smaller:	remove walls and tiles
	 * If new size is larger:		spawn only tiles
	 */

	if (Properties->EditMode != EEditMode::ManageRooms)
		return;

	if (auto ActiveRoom = TryGetCurrentRoom())
	{
		const FIntPoint OldRoomSize = ActiveRoom->GetRoomSize();
		const FIntPoint NewRoomSize = Properties->RoomSize;

		// Validate if anything changed
		if (OldRoomSize.X == NewRoomSize.X && OldRoomSize.Y == NewRoomSize.Y)
			return;

		ActiveRoom->SetRoomSize(NewRoomSize);

		TArray<TObjectPtr<AWall>>& RoomWalls = ActiveRoom->GetWalls();
		TArray<TObjectPtr<ATile>>& RoomTiles = ActiveRoom->GetTiles();
		TempWalls.Empty();
		TempTiles.Empty();
		TempWalls.SetNum(NewRoomSize.X * (NewRoomSize.Y + 1) + (NewRoomSize.X + 1) * NewRoomSize.Y);
		TempTiles.SetNum(NewRoomSize.X * NewRoomSize.Y);

		int OldIndex = 0, OldIndexLocal = 0, NewIndex = 0;
		int OffsetOldIndex = OldRoomSize.X * (OldRoomSize.Y + 1);
		int OffsetNewIndex = NewRoomSize.X * (NewRoomSize.Y + 1);
		FIntPoint OldCoords;

		// Traverse X-aligned walls: X*(Y+1)
		for (size_t iY = 0; iY <= OldRoomSize.Y; iY++)
		{
			for (size_t iX = 0; iX < OldRoomSize.X; iX++)
			{
				OldIndex = iX + iY * OldRoomSize.X;
				OldCoords.X = OldIndex % OldRoomSize.X;
				OldCoords.Y = OldIndex / OldRoomSize.X;

				// Destroy actor if outside new bounds
				// Comparing coords vs size, but X-aligned Walls have +1 on Y-axis, making equal inside bounds
				if (OldCoords.X >= NewRoomSize.X || OldCoords.Y > NewRoomSize.Y)
				{
					if (RoomWalls[OldIndex])
						TargetWorld->DestroyActor(RoomWalls[OldIndex]);
				}
				// Otherwise remap to new index
				else
				{
					NewIndex = iX + iY * NewRoomSize.X;
					TempWalls[NewIndex] = RoomWalls[OldIndex];
					RoomWalls[OldIndex] = nullptr;
				}
			}
		}
		// Traverse Y-aligned walls: (X+1)*Y
		for (size_t iY = 0; iY < OldRoomSize.Y; iY++)
		{
			for (size_t iX = 0; iX <= OldRoomSize.X; iX++)
			{
				OldIndexLocal = iX + iY * (OldRoomSize.X + 1);
				OldCoords.X = OldIndexLocal % (OldRoomSize.X + 1);
				OldCoords.Y = OldIndexLocal / (OldRoomSize.X + 1);
				OldIndex = OffsetOldIndex + OldIndexLocal;

				// Destroy actor if outside new bounds
				// Comparing coords vs size, but Y-aligned Walls have +1 on X-axis, making equal inside bounds
				if (OldCoords.X > NewRoomSize.X || OldCoords.Y >= NewRoomSize.Y)
				{
					if (RoomWalls[OldIndex])
						TargetWorld->DestroyActor(RoomWalls[OldIndex]);
				}
				// Otherwise remap to new index
				else
				{
					NewIndex = OffsetNewIndex + iX + iY * (NewRoomSize.X + 1);
					TempWalls[NewIndex] = RoomWalls[OldIndex];
					RoomWalls[OldIndex] = nullptr;
				}
			}
		}

		// Traverse tiles: X*Y
		for (size_t iY = 0; iY < OldRoomSize.Y; iY++)
		{
			for (size_t iX = 0; iX < OldRoomSize.X; iX++)
			{
				OldIndex = iX + iY * OldRoomSize.X;
				OldCoords.X = iX;
				OldCoords.Y = iY;

				// Destroy actor if outside new bounds
				// Comparing coords vs size, so equal is out of bounds for Tiles
				if (OldCoords.X >= NewRoomSize.X || OldCoords.Y >= NewRoomSize.Y)
				{
					if (RoomTiles[OldIndex])
						TargetWorld->DestroyActor(RoomTiles[OldIndex]);
				}
				// Otherwise remap to new index
				else
				{
					NewIndex = iX + iY * NewRoomSize.X;
					TempTiles[NewIndex] = RoomTiles[OldIndex];
					RoomTiles[OldIndex] = nullptr;
				}
			}
		}

		// Switch to persistent arrays
		RoomWalls.Empty();
		RoomTiles.Empty();
		RoomWalls = std::move(TempWalls);
		RoomTiles = std::move(TempTiles);

		// Add new tiles based on RoomSizes
		float OffsetX = 0, OffsetY = 0;

		// Lambda - Check to spawn tiles in given ranges
		auto CheckSpawnTiles = [&](int StartIndexX, int EndIndexX, int StartIndexY, int EndIndexY)
		{
			for (size_t iY = StartIndexY; iY < EndIndexY; iY++)
			{
				OffsetY = 0.5f * TileSizeCM + TileSizeCM * iY;
				for (size_t iX = StartIndexX; iX < EndIndexX; iX++)
				{
					OffsetX = 0.5f * TileSizeCM + TileSizeCM * iX;
					NewIndex = iX + iY * NewRoomSize.X;
					ATile* NewTile = SpawnTile(*ActiveRoom, NewIndex, FVector(OffsetX, OffsetY, 0.0f));
					ActiveRoom->SetTileAtIndex(NewIndex, NewTile);
				}
			}
		};

		// Check to spawn new tiles, if X larger
		CheckSpawnTiles(OldRoomSize.X, NewRoomSize.X, 0,             OldRoomSize.Y);
		// Check to spawn new tiles, if Y larger
		CheckSpawnTiles(0,             NewRoomSize.X, OldRoomSize.Y, NewRoomSize.Y);

		// Reset bounding box
		RemoveRoomBoundingBox();
		SpawnRoomBoundingBox();
	}
}

void UPRG_PluginRoomTool::DeleteRoomInScene(TObjectPtr<APRG_Room> DeletedRoom)
{
	Properties->RoomArray.Remove(DeletedRoom);
	DeleteRoom(DeletedRoom);
}

void UPRG_PluginRoomTool::DeleteRoom(TObjectPtr<APRG_Room> removeRoom)
{
	// Validation
	if (!TargetWorld || !removeRoom || !RoomArrayCopy.Contains(removeRoom))
		return;

	if (removeRoom == CurrentRoom)
		SetCurrentRoom(nullptr);

	RemoveRoomBoundingBox();
	DeleteTempActors();

	// If an entry was cleared, then delete the empty entry from RoomArray
	if (RoomArrayCopy.Num() == Properties->RoomArray.Num())
		Properties->RoomArray.RemoveSingle(nullptr);

	// Remove gizmo from scene
	removeRoom->RemoveRoomGizmo(GetToolManager()->GetPairedGizmoManager());

	RoomArrayCopy.RemoveSingle(removeRoom);
	TargetWorld->DestroyActor(removeRoom);

	RoomArraySize = Properties->RoomArray.Num();

	// Set the first room as selected
	TryGetCurrentRoom();
}

void UPRG_PluginRoomTool::DeleteTempActors()
{
	for (auto& TempWall : TempWalls)
	{
		if (TempWall)
			TargetWorld->DestroyActor(TempWall);
	}
	TempWalls.Empty();

	for (auto& TempTile : TempTiles)
	{
		if (TempTile)
			TargetWorld->DestroyActor(TempTile);
	}
	TempTiles.Empty();
}

// ********************************** Gizmo Functions ************************************************

void UPRG_PluginRoomTool::CreateCustomRoomGizmo(TObjectPtr<APRG_Room> room, bool loadTransform)
{
	FTransform StartTransform = FTransform::Identity;

	// When restoring, set initial transform to room transform
	if (loadTransform)
	{
		StartTransform = room->GetActorTransform();
		Properties->SpawnPosition = room->GetActorLocation();
	}
	else
		StartTransform.SetTranslation(Properties->SpawnPosition);

	TransformProxy = NewObject<UTransformProxy>(this);
	TransformProxy->SetTransform(StartTransform);
	TransformGizmo = UE::TransformGizmoUtil::CreateCustomTransformGizmo(GetToolManager()->GetPairedGizmoManager(),
		ETransformGizmoSubElements::TranslateAllAxes | ETransformGizmoSubElements::TranslateAllPlanes | ETransformGizmoSubElements::RotateAxisZ, this);
	
	if (Properties->EditMode == EEditMode::CreateRooms && !Properties->ShowAllGizmos)
		TransformGizmo->SetVisibility(false);
	
	TransformGizmo->SetActiveTarget(TransformProxy, GetToolManager());

	// Listen for changes to the proxy and update the room when that happens
	TransformProxy->OnTransformChanged.AddUObject(this, &UPRG_PluginRoomTool::GizmoTransformChanged);

	// Store proxy with room
	room->SetRoomGizmo(TransformGizmo);
	room->SetProxyTransform(TransformProxy);
}

void UPRG_PluginRoomTool::GizmoTransformChanged(UTransformProxy* Proxy, FTransform Transform)
{
	// Handle position snapping
	if (Properties->PositionSnap != EPosSnap::NoSnapping)
	{
		int SnapPosSize = int(Properties->PositionSnap);
		FVector3d TransformPos = Transform.GetLocation();
		TransformPos.X = roundf(TransformPos.X / SnapPosSize) * SnapPosSize;
		TransformPos.Y = roundf(TransformPos.Y / SnapPosSize) * SnapPosSize;
		TransformPos.Z = roundf(TransformPos.Z / SnapPosSize) * SnapPosSize;
		Transform.SetLocation(TransformPos);
	}

	// Handle rotation snapping
	if (Properties->RotationSnap != ERotSnap::NoSnapping)
	{
		FQuat4d TransformRot = Transform.GetRotation();
		float RotSnapped = TransformRot.Euler().Z;
		int SnapRotSize = int(Properties->RotationSnap);
		RotSnapped = roundf(RotSnapped / SnapRotSize) * SnapRotSize;
		TransformRot = FQuat4d::MakeFromEuler(FVector3d(0, 0, RotSnapped));
		Transform.SetRotation(TransformRot);
	}

	// Check if able to modify gizmo's
	if (Properties->EditMode != EEditMode::CreateRooms || Properties->ShowAllGizmos)
	{
		// Find Room matching activated gizmo
		for (APRG_Room* FoundRoom : Properties->RoomArray)
		{
			if (!FoundRoom)
				return;

			if (FoundRoom->GetProxyTransform() == Proxy)
			{
				if (FoundRoom != CurrentRoom)
				{
					ResetRoomEditMode(Properties->EditMode);
					SetCurrentRoom(FoundRoom);
					SetRoomEditMode();
				}

				// Set room as selected actor
				if (UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>())
					EditorActorSubsystem->SetSelectedLevelActors({ FoundRoom });

				FoundRoom->SetActorTransform(Transform);
				FoundRoom->MarkPackageDirty();

				return;
			}
		}
	}

	// Update SpawnPosition only if not already changing an existing room gizmo
	if (Properties->EditMode == EEditMode::CreateRooms)
	{
		if (SpawnProxy->GetTransform().GetLocation() != Transform.GetLocation())
		{
			SpawnProxy->SetTransform(Transform);
			Properties->SpawnPosition = Transform.GetLocation();
			PRGSettings->SpawnPosition = Properties->SpawnPosition;
		}
	}
}

//void UPRG_PluginRoomTool::SetGizmoScale(float Scale)
//{
//	if (TargetWorld)
//	{
//		TArray<AActor*> FoundActors;
//		UGameplayStatics::GetAllActorsOfClass(TargetWorld, APRG_Room::StaticClass(), FoundActors);
//
//		for (AActor* Actor : FoundActors)
//		{
//      // NOPE - RESCALES EVERYTHING EXCEPT GIZMO
//			//APRG_Room* Room = static_cast<APRG_Room*>(Actor);
//			//FTransform Transform = Room->GetRoomGizmo()->GetGizmoTransform();
//			//Room->SetProxyTransform(Proxy);
//
//			// NOPE - NOTHING
//			//APRG_Room* Room = static_cast<APRG_Room*>(Actor);
//			//TObjectPtr<UCombinedTransformGizmo> Gizmo = Room->GetRoomGizmo();
//			//Gizmo->GetGizmoActor()->SetActorScale3D(FVector3d(Scale));
//		}
//	}
//}

void UPRG_PluginRoomTool::ToggleGizmoVisibility(bool Visible)
{
	if (TargetWorld)
	{
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(TargetWorld, APRG_Room::StaticClass(), FoundActors);

		for (AActor* Actor : FoundActors)
		{
			APRG_Room* Room = static_cast<APRG_Room*>(Actor);
			if (CurrentRoom != Room)
			{
				Room->GetRoomGizmo()->SetVisibility(Visible);
				if (Visible)
					Room->GetRoomGizmo()->SetActiveTarget(Room->GetProxyTransform());
			}
			// Ensure that outside CreateRooms mode the CurrentRoom always has its gizmo
			else
			{
				Room->GetRoomGizmo()->SetVisibility(Properties->EditMode != EEditMode::CreateRooms || Properties->ShowAllGizmos);
			}
		}
	}
}

// ******************************** Edit Mode Functions **********************************************

// Reset room from edit state. Deletes temporary actors and resets materials
void UPRG_PluginRoomTool::ResetRoomEditMode(EEditMode EditMode)
{
	// Clear old state
	DeleteTempActors();
	ResetPersistMaterials(EditMode);
}

// Set room to selected EditMode. Spawns appropriate temporary actors and changes material
void UPRG_PluginRoomTool::SetRoomEditMode()
{
	// Only spawn temporary actors if there is a room available
	if (auto ActiveRoom = TryGetCurrentRoom())
	{
		if (ActiveRoom->IsPendingKill())
		{
			UE_LOG(LogTemp, Warning, TEXT("DEBUGPRINT - PENDING KILL"));
#if WITH_EDITOR
			GEditor->SelectNone(false, true, false);
#endif
			return;
		}
		// Validate room state. Room will be in an invalid state when undoing/redoing a room deletion or creation, which is not (yet) supported.
		if (!ActiveRoom->GetRoomGizmo())
		{
			checkf(false, TEXT("Room in invalid state! Exit the tool and reopen to recover internal state."));
			return;
		}

		if (Properties->EditMode == EEditMode::EditWalls)
		{
			SetEditModeMaterials(TempWalls, ActiveRoom->GetWalls(), &UPRG_PluginRoomTool::SpawnWall, &APRG_Room::GetWallPositionFromIndex);
		}
		else if (Properties->EditMode == EEditMode::EditTiles)
		{
			SetEditModeMaterials(TempTiles, ActiveRoom->GetTiles(), &UPRG_PluginRoomTool::SpawnTile, &APRG_Room::GetTilePositionFromIndex);
		}

		if (Properties->EditMode != EEditMode::CreateRooms)
		{
			Properties->RoomSize = ActiveRoom->GetRoomSize();
			Properties->InitHeight = ActiveRoom->GetRoomHeight();
			ActiveRoom->GetRoomGizmo()->SetVisibility(true);
		}
	}

	// Update gizmo's when switching to CreateRooms mode
	if (Properties->EditMode == EEditMode::CreateRooms)
	{
		SetCurrentRoom(nullptr);
		SpawnCreateRoomGizmo();
	}
	else
		RemoveCreateRoomGizmo();

	// Create or remove the room selection bounding box
	if (Properties->EditMode == EEditMode::ManageRooms)
		SpawnRoomBoundingBox();
	else
		RemoveRoomBoundingBox();

	PrevEditMode = Properties->EditMode;

	CurrentSelectedActorInRoom = { -1, nullptr };
}

void UPRG_PluginRoomTool::SetEditModeMaterial(TObjectPtr<AStaticMeshActor> Actor, TObjectPtr<UMaterial> Material)
{
	if (UStaticMeshComponent* Mesh = Actor->GetStaticMeshComponent())
	{
		for (int i = 0; i < Mesh->GetNumMaterials(); i++)
			Mesh->SetMaterial(i, Material);
	}
}

void UPRG_PluginRoomTool::SetArrayMaterials(TObjectPtr<AStaticMeshActor> Actor, TArray<TObjectPtr<UMaterial>> Materials)
{
	if (UStaticMeshComponent* Mesh = Actor->GetStaticMeshComponent())
	{
		for (int i = 0; i < Mesh->GetNumMaterials(); i++)
		{
			if (Materials[i])
				Mesh->SetMaterial(i, Materials[i]);
			else
				Mesh->SetMaterial(i, Properties->DefaultMat);
		}
	}
}

void UPRG_PluginRoomTool::ResetPersistMaterials(EEditMode EditMode)
{
	// Reset materials based on selected mode
	if (CurrentRoom)
	{
		if (EditMode == EEditMode::EditWalls)
		{
			TArray<TObjectPtr<AWall>> Walls = CurrentRoom->GetWalls();
			for (size_t i = 0; i < Walls.Num(); i++)
			{
				if (Walls[i])
				{
					// Reset original materials
					SetArrayMaterials(Walls[i], OriginalMaterials[i]);
				}
			}
		}
		else if (EditMode == EEditMode::EditTiles)
		{
			TArray<TObjectPtr<ATile>> Tiles = CurrentRoom->GetTiles();
			for (size_t i = 0; i < Tiles.Num(); i++)
			{
				if (Tiles[i])
				{
					//  Reset original materials
					SetArrayMaterials(Tiles[i], OriginalMaterials[i]);
				}
			}
		}
	}
}

// ***************************************************************************************************
// ******************************** PRIVATE FUNCTIONS ************************************************
// ***************************************************************************************************

// ********************************* Spawn Objects Functions *****************************************

TObjectPtr<ATile> UPRG_PluginRoomTool::SpawnTile(APRG_Room& ParentRoom, int IndexInRoom, FVector SpawnPos)
{
	// INFO: IndexInRoom is added to allow passing functor as template argument for SpawnTile / SpawnWall
	FActorSpawnParameters SpawnInfoTile;
	TObjectPtr<ATile> NewTile = TargetWorld->SpawnActor<ATile>(SpawnPos, FRotator(0.0f, 0.0f, 0.0f), SpawnInfoTile);
	NewTile->AttachToActor(&ParentRoom, FAttachmentTransformRules::KeepRelativeTransform);
	NewTile->GetStaticMeshComponent()->SetStaticMesh(Properties->FloorMesh);

	return NewTile;
}

TObjectPtr<AWall> UPRG_PluginRoomTool::SpawnWall(APRG_Room& ParentRoom, int IndexInRoom, FVector SpawnPos)
{
	return SpawnWallRot(ParentRoom, SpawnPos, ParentRoom.GetWallRotationByIndex(IndexInRoom));
}

TObjectPtr<AWall> UPRG_PluginRoomTool::SpawnWallRot(APRG_Room& ParentRoom, FVector SpawnPos, FRotator SpawnRot)
{
	FActorSpawnParameters SpawnInfo;
	TObjectPtr<AWall> NewWall = TargetWorld->SpawnActor<AWall>(SpawnPos, SpawnRot, SpawnInfo);
	NewWall->AttachToActor(&ParentRoom, FAttachmentTransformRules::KeepRelativeTransform);
	NewWall->GetStaticMeshComponent()->SetStaticMesh(Properties->WallMesh);

	return NewWall;
}

// ********************************** Boundingbox Functions ******************************************

void UPRG_PluginRoomTool::SpawnRoomBoundingBox()
{
	// Check if already added
	if (CurrentBoundingBox && CurrentBoundingBox->GetAttachParentActor() == CurrentRoom)
		return;

	if (auto ActiveRoom = TryGetCurrentRoom())
	{
		// Get room size. Has XY in tiles, so convert these to meters
		FVector RoomSize = FVector(ActiveRoom->GetRoomSize());
		RoomSize.X *= Properties->TileSize;
		RoomSize.Y *= Properties->TileSize;
		RoomSize.Z = ActiveRoom->GetRoomHeight();
		FVector SpawnScale = RoomSize + 0.1f;
		FVector CenterOffset = RoomSize * 50.0f; // M to CM, divided by 2 for center

		FTransform SpawnTransform = FTransform(FRotator(0.0f, 0.0f, 0.0f), CenterOffset, SpawnScale);
		FActorSpawnParameters SpawnInfoTile;
		TObjectPtr<ARoomBounds> NewActor = TargetWorld->SpawnActor<ARoomBounds>(ARoomBounds::StaticClass(), SpawnTransform, SpawnInfoTile);
		NewActor->AttachToActor(ActiveRoom, FAttachmentTransformRules::KeepRelativeTransform);

		NewActor->GetStaticMeshComponent()->SetStaticMesh(NewActor->CubeMesh);
		SetEditModeMaterial(NewActor, NewActor->CubeMaterial);

		CurrentBoundingBox = NewActor;
	}
}

void UPRG_PluginRoomTool::RemoveRoomBoundingBox()
{
	if (CurrentBoundingBox)
	{
		TargetWorld->DestroyActor(CurrentBoundingBox);
		CurrentBoundingBox = nullptr;
	}
}

void UPRG_PluginRoomTool::SpawnCreateRoomGizmo()
{
	// Check if valid mode and whether gizmo already exists
	if (Properties->EditMode == EEditMode::CreateRooms && !SpawnGizmo)
	{
		SpawnProxy = NewObject<UTransformProxy>(this);
		SpawnProxy->SetTransform(FTransform(Properties->SpawnPosition));
		SpawnGizmo = UE::TransformGizmoUtil::CreateCustomTransformGizmo(GetToolManager()->GetPairedGizmoManager(),
			ETransformGizmoSubElements::TranslateAllAxes | ETransformGizmoSubElements::TranslateAllPlanes, this);
		SpawnGizmo->SetActiveTarget(SpawnProxy, GetToolManager());

		// Listen for changes to the proxy and update the gizmo when that happens
		SpawnProxy->OnTransformChanged.AddUObject(this, &UPRG_PluginRoomTool::GizmoTransformChanged);
	}
}

void UPRG_PluginRoomTool::UpdateCreateRoomGizmo(FVector NewPos)
{
	if (Properties->EditMode == EEditMode::CreateRooms && SpawnGizmo)
	{
		SpawnGizmo->SetNewGizmoTransform(FTransform(Properties->SpawnPosition));
	}
}

void UPRG_PluginRoomTool::RemoveCreateRoomGizmo()
{
	if (SpawnGizmo)
	{
		GetToolManager()->GetPairedGizmoManager()->DestroyGizmo(SpawnGizmo);
		SpawnGizmo = nullptr;
		SpawnProxy->OnTransformChanged.RemoveAll(this);
		SpawnProxy = nullptr;
	}
}

// ************************************* Other Functions *********************************************

void UPRG_PluginRoomTool::ResetToolState()
{
	CurrentSelectedActorInRoom = { -1, nullptr };
	PrevEditMode = EEditMode::CreateRooms;
	CurrentRoom = nullptr;
	RoomArraySize = 0;
	RemoveRoomBoundingBox();
}

#undef LOCTEXT_NAMESPACE
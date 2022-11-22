// Copyright 2022 Steven Weijden

#include "PRG_PluginRoomTool.h"
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
	EditMode = EEditMode::ViewMode;
	GridSnapSize = ESnapSizes::SnapX10;
	InitSize = { 3, 2 };
	InitHeight = 2;
	TileSize = 2;

	// Set default values for objects
	FloorMesh							= ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("/PRG_Plugin/Meshes/SM_Floor_Solid.SM_Floor_Solid")).Object;
	WallMesh							= ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("/PRG_Plugin/Meshes/SM_Wall_Solid.SM_Wall_Solid")).Object;
	DefaultMat						= ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("/PRG_Plugin/Materials/Mat_Default.Mat_Default")).Object;
	PersistSelectedMat		= ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("/PRG_Plugin/Materials/Mat_PersistSelected.Mat_PersistSelected")).Object;
	PersistUnselectedMat	= ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("/PRG_Plugin/Materials/Mat_PersistUnselected.Mat_PersistUnselected")).Object;
	TempSelectedMat				= ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("/PRG_Plugin/Materials/Mat_TempSelected.Mat_TempSelected")).Object;
	TempUnselectedMat			= ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("/PRG_Plugin/Materials/Mat_TempUnselected.Mat_TempUnselected")).Object;
}

/*
 * Tool implementation
 */

UPRG_PluginRoomTool::UPRG_PluginRoomTool()
{
	CurrentSelectedActorInRoom = nullptr;
	PrevEditMode = EEditMode::ViewMode;
	CurrentRoom = nullptr;
	RoomArraySize = 0;
	TileSizeCM = 200;
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

	FindRoomsInScene();
}

void UPRG_PluginRoomTool::Shutdown(EToolShutdownType ShutdownType)
{
	DeleteTempActors();
	RemoveRoomBoundingBox();
	ResetPersistMaterials(Properties->EditMode);

	GetToolManager()->GetPairedGizmoManager()->DestroyAllGizmosByOwner(this);

}

void UPRG_PluginRoomTool::OnTick(float DeltaTime)
{
}

void UPRG_PluginRoomTool::Render(IToolsContextRenderAPI* RenderAPI)
{
}

void UPRG_PluginRoomTool::GizmoTransformChanged(UTransformProxy* Proxy, FTransform Transform)
{
	if (Properties->GridSnapSize != ESnapSizes::NoSnapping)
	{
		int SnapSize = int(Properties->GridSnapSize);
		FVector3d TransformPos = Transform.GetLocation();
		TransformPos[0] = roundf(TransformPos[0]/SnapSize)*SnapSize;
		TransformPos[1] = roundf(TransformPos[1]/SnapSize)*SnapSize;
		TransformPos[2] = roundf(TransformPos[2]/SnapSize)*SnapSize;
		Transform.SetLocation(TransformPos);
	}

	// Find Room matching activated gizmo
	for (APRG_Room* FoundRoom : Properties->RoomArray)
	{
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
			break;
		}
	}
}

void UPRG_PluginRoomTool::OnPropertyModified(UObject* PropertySet, FProperty* Property)
{
	// Enum - EditMode
	if (Property->IsA(FEnumProperty::StaticClass()))
	{
		if (Property->GetFName() == "EditMode")
		{
			ResetRoomEditMode(PrevEditMode);
			SetRoomEditMode();
		}
	}
	// Int - TileSize
	else if (Property->IsA(FIntProperty::StaticClass()))
	{
		if (Property->GetFName() == "TileSize")
			TileSizeCM = Properties->TileSize * 100;
	}
	// UObject - StaticMesh
	else if (Property->IsA(FObjectProperty::StaticClass()))
	{
		// Set the assigned static mesh to the currently selected actor
		if (CurrentSelectedActorInRoom)
		{
			const FObjectProperty* ObjectProperty = static_cast<FObjectProperty*>(Property);
			if (UStaticMesh* StaticMesh = static_cast<UStaticMesh*>(ObjectProperty->GetPropertyValue_InContainer(PropertySet)))
			{
				if (Property->GetFName() == "WallMesh"       && Properties->EditMode == EEditMode::EditWalls)
						CurrentSelectedActorInRoom->GetStaticMeshComponent()->SetStaticMesh(StaticMesh);
				else if (Property->GetFName() == "FloorMesh" && Properties->EditMode == EEditMode::EditTiles)
						CurrentSelectedActorInRoom->GetStaticMeshComponent()->SetStaticMesh(StaticMesh);
			}
		}
	}
	// Array - RoomArray
	else if (Property->IsA(FArrayProperty::StaticClass()))
	{
		if (Property->GetFName() == "RoomArray")
		{
			// 1. Delete all Rooms
			if (Properties->RoomArray.Num() == 0)
			{
				for (size_t i = RoomArrayCopy.Num(); i > 0; i--)
					DeleteRoom(RoomArrayCopy[i-1]);
			}
			// 2. Delete a Removed or cleared ArrayRoom entry
			// TODO: Update RoomArray if Room is deleted in the map => HANDLE IN ROOM DESTRUCTOR
			//			 But how to retain isolation of Room towards Tool then?
			//			 Add a listener in Tool when Room is destroyed?!
			else if (RoomArraySize >= Properties->RoomArray.Num())
			{
				// Find entry that has been deleted from RoomArray
				// If an entry is cleared instead, this is handled in DeleteRoom
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
		case EEditMode::ManageRooms:
			Properties->SpawnPosition = Result.ImpactPoint;
			break;

		// Edit walls in viewport
		case EEditMode::EditWalls:

			if (Result.GetActor()->IsA(AWall::StaticClass()))
				OnClickEditMode(EEditMode::EditWalls, Result, TempWalls, CurrentRoom->GetWalls());
			break;

		// Edit tiles in viewport
		case EEditMode::EditTiles:

			if (Result.GetActor()->IsA(ATile::StaticClass()))
				OnClickEditMode(EEditMode::EditTiles, Result, TempTiles, CurrentRoom->GetTiles());
			break;

		// Ignore input for EEditMode::ViewMode
		default:
			break;
		}
	}
}

void UPRG_PluginRoomTool::DeleteRoomInScene(TObjectPtr<APRG_Room> DeletedRoom)
{
	//UE_LOG(LogTemp, Warning, TEXT("ROOM DELETED IN SCENE: %s"), *DeletedRoom->GetName());

	// Trigger the room deletion via OnPropertyModified
	Properties->RoomArray.Remove(DeletedRoom);
	DeleteRoom(DeletedRoom);
}

// Spawn the room
void UPRG_PluginRoomTool::SpawnRoom()
{
	// Validation
	checkf(RoomArraySize == Properties->RoomArray.Num() - 1, TEXT("RoomArraySize mismatching Properties->RoomArray!"));

	// Spawn new room object
	FActorSpawnParameters SpawnInfoRoom;
	APRG_Room* NewRoom = GetWorld()->SpawnActor<APRG_Room>(Properties->SpawnPosition, FRotator(0.0f, 0.0f, 0.0f), SpawnInfoRoom);
	NewRoom->InitRoom(Properties->InitSize, Properties->InitHeight);
	NewRoom->OnRoomDeletion.BindUObject(this, &UPRG_PluginRoomTool::DeleteRoomInScene);

	// Store room in arrays
	if (int EmptyIndex = Properties->RoomArray.Find(nullptr))
		Properties->RoomArray[EmptyIndex] = NewRoom;
	else
		Properties->RoomArray[RoomArraySize] = NewRoom;
	RoomArrayCopy.Add(NewRoom);

	SetCurrentRoom(NewRoom);
	RoomArraySize = Properties->RoomArray.Num();

	CreateRoomGizmo(NewRoom, false);

	// Spawn initial room tiles
	{
		int SetIndex;
		int SwitchToHorizonWalls = 0;
		float OffsetX, OffsetY;
		// Columns
		for (size_t iY = 0; iY < Properties->InitSize.Y; iY++)
		{
			OffsetY = 0.5f * TileSizeCM + TileSizeCM * iY;

			// Rows
			for (size_t iX = 0; iX < Properties->InitSize.X; iX++)
			{
				OffsetX = 0.5f * TileSizeCM + TileSizeCM * iX;
				SetIndex = iX + iY * Properties->InitSize.X;
				ATile* NewTile = SpawnTile(*NewRoom, SetIndex, FVector(OffsetX, OffsetY, 0.0f));
				NewRoom->SetTileAtIndex(SetIndex, NewTile);
			}
		}
	}

	// Spawn initial room walls
	/* Index progression example on a 3x2 grid. First vertical walls, then horizontal walls:
	 * 
	 *     0       1      2
	 *  9     10      11     12
	 *     3       4      5
	 * 13     14      15     16
	 *     6       7      8
	 */
	{
		int SetIndex = 0;
		int AddIndex = Properties->InitSize.X * (Properties->InitSize.Y + 1);
		float OffsetX = 0, OffsetY = 0;
		FRotator WallRotation = FRotator(0.0f, 0.0f, 0.0f);
		// Spawn X-aligned walls, limited to outside edges using X*(Y+1)
		for (size_t iY = 0; iY <= Properties->InitSize.Y; iY += Properties->InitSize.Y)
		//for (size_t iY = 0; iY <= Properties->InitSizeY; iY++) // Traverse all
		{
			OffsetY = TileSizeCM * iY;
			for (size_t iX = 0; iX < Properties->InitSize.X; iX++)
			{
				OffsetX = 0.5f * TileSizeCM + TileSizeCM * iX;
				SetIndex = iX + iY * Properties->InitSize.X;
				AWall* NewWall = SpawnWallRot(*NewRoom, FVector(OffsetX, OffsetY, 0.0f), WallRotation);
				NewRoom->SetWallAtIndex(SetIndex, NewWall);
			}
		}

		WallRotation = FRotator(0.0f, 90.0f, 0.0f);
		// Spawn Y-aligned walls, limited to outside edges using (X+1)*Y 
		for (size_t iY = 0; iY < Properties->InitSize.Y; iY++)
		{
			OffsetY = 0.5f * TileSizeCM + TileSizeCM * iY;
			for (size_t iX = 0; iX <= Properties->InitSize.X; iX += Properties->InitSize.X)
			//for (size_t iX = 0; iX <= Properties->InitSizeX; iX++) // Traverse all
			{
				OffsetX = TileSizeCM * iX;
				SetIndex = AddIndex + iX + iY * (Properties->InitSize.X+1);
				AWall* NewWall = SpawnWallRot(*NewRoom, FVector(OffsetX, OffsetY, 0.0f), WallRotation);
				NewRoom->SetWallAtIndex(SetIndex, NewWall);
			}
		}
	}

	// CODE WITH SPAWN ACTOR STUFF AND SET PARENT
	// 
	//GetToolManager()->BeginUndoTransaction(LOCTEXT("AddPivotActorTransactionName", "Add Pivot Actor"));

	//// Create an empty actor at the location of the gizmo. The way we do it here, using this factory, is
	//// editor-only.
	//UActorFactoryEmptyActor* EmptyActorFactory = NewObject<UActorFactoryEmptyActor>();
	//FAssetData AssetData(EmptyActorFactory->GetDefaultActorClass(FAssetData()));
	//FActorSpawnParameters SpawnParams;
	////SpawnParams.Name = TEXT("PivotActor");
	//SpawnParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
	//AActor* NewActor = EmptyActorFactory->CreateActor(AssetData.GetAsset(),
	//	TargetWorld->GetCurrentLevel(),
	//	TransformProxy->GetTransform(),
	//	SpawnParams);

	//// Grab the first selected target. It will have the same parent as the other ones, so
	//// we'll use it to figure out the empty actor's parent.
	//AActor* TargetActor = UE::ToolTarget::GetTargetActor(Targets[0]);

	//// This is also editor-only: it's the label that shows up in the hierarchy
	//NewActor->SetActorLabel(Targets.Num() == 1 ? TargetActor->GetActorLabel() + TEXT("_Pivot")
	//	: TEXT("Pivot"));

	//// Attach the empty actor in the correct place in the hierarchy. This can be done in a non-editor-only
	//// way, but it's easier to use the editor's function to do it so that undo/redo and level saving are 
	//// properly done.
	//if (AActor* ParentActor = TargetActor->GetAttachParentActor())
	//{
	//	GEditor->ParentActors(ParentActor, NewActor, NAME_None);
	//}
	//for (TObjectPtr<UToolTarget> Target : Targets)
	//{
	//	TargetActor = UE::ToolTarget::GetTargetActor(Target);
	//	GEditor->ParentActors(NewActor, TargetActor, NAME_None);
	//}
	//ToolSelectionUtil::SetNewActorSelection(GetToolManager(), NewActor);

	//GetToolManager()->EndUndoTransaction();
}

// Find any room objects in the scene and add them to the tool
void UPRG_PluginRoomTool::FindRoomsInScene()
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APRG_Room::StaticClass(), FoundActors);

	for (AActor* actor : FoundActors)
	{
		APRG_Room* room = static_cast<APRG_Room*>(actor);
		SetupFoundRoom(room);
	}
}

// Setup room found in scene
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
	CreateRoomGizmo(FoundRoom, true);

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

// Delete the given room
void UPRG_PluginRoomTool::DeleteRoom(TObjectPtr<APRG_Room> removeRoom)
{
	// Validation
	if (!removeRoom || !RoomArrayCopy.Contains(removeRoom))
		return;

	if (removeRoom == CurrentRoom)
		SetCurrentRoom(nullptr);

	// If an entry was cleared, then delete the empty entry from RoomArray
	if (RoomArrayCopy.Num() == Properties->RoomArray.Num())
		Properties->RoomArray.RemoveSingle(nullptr);

	// Remove all children (tiles and walls)
	removeRoom->ForEachAttachedActors([this](AActor* AttachedActor)
		{
			GetWorld()->DestroyActor(AttachedActor);
			return true;
		}
	);

	// Remove gizmo from scene
	removeRoom->DestroyRoomGizmo(GetToolManager()->GetPairedGizmoManager());

	RoomArrayCopy.RemoveSingle(removeRoom);
	GetWorld()->DestroyActor(removeRoom);

	RoomArraySize = Properties->RoomArray.Num();
}

// Clear any temp actors
void UPRG_PluginRoomTool::DeleteTempActors()
{
	for (auto& TempWall : TempWalls)
	{
		if (TempWall)
			GetWorld()->DestroyActor(TempWall);
	}
	TempWalls.Empty();

	for (auto& TempTile : TempTiles)
	{
		if (TempTile)
			GetWorld()->DestroyActor(TempTile);
	}
	TempTiles.Empty();
}

TObjectPtr<APRG_Room> UPRG_PluginRoomTool::TryFetchCurrentRoom()
{
	// If no current room is set, try to assign first in room array
	if (CurrentRoom == nullptr && Properties->RoomArray.Num() > 0)
	{
		CurrentRoom = Properties->RoomArray[0];
		CurrentRoom->InitRoom();
		SetCurrentRoom(CurrentRoom);
	}

	// Can still be nullptr
	return CurrentRoom;
}

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
	//		TODO: BLOCK EditWall/EditTiles IF NO ROOM EXISTS, EXIT OUT OF EditWalls/EditTiles IF LAST ROOM IS REMOVED
	if (auto ActiveRoom = TryFetchCurrentRoom())
	{
		if (Properties->EditMode == EEditMode::EditWalls)
			SetRoomEditModeT(TempWalls, ActiveRoom->GetWalls(), &UPRG_PluginRoomTool::SpawnWall, &APRG_Room::GetWallPositionFromIndex);
		else if (Properties->EditMode == EEditMode::EditTiles)
			SetRoomEditModeT(TempTiles, ActiveRoom->GetTiles(), &UPRG_PluginRoomTool::SpawnTile, &APRG_Room::GetTilePositionFromIndex);
	}

	// Create or remove the room selection bounding box
	if (Properties->EditMode == EEditMode::ManageRooms)
		SpawnRoomBoundingBox();
	else
		RemoveRoomBoundingBox();

	PrevEditMode = Properties->EditMode;

	CurrentSelectedActorInRoom = nullptr;
}

void UPRG_PluginRoomTool::SetMaterial(TObjectPtr<AStaticMeshActor> Actor, TObjectPtr<UMaterial> Material)
{
	if (UStaticMeshComponent* Mesh = Actor->GetStaticMeshComponent())
		Mesh->SetMaterial(0, Material);
}

void UPRG_PluginRoomTool::ResetPersistMaterials(EEditMode EditMode)
{
	if (CurrentRoom)
	{
		// Reset material colors
		//		TODO: RESET TO PREVIOUS INSTEAD OF DEFAULT - Allow for custom materials!
		if (EditMode == EEditMode::EditWalls)
		{
			for (auto& Wall : CurrentRoom->GetWalls())
			{
				if (Wall)
					SetMaterial(Wall, Properties->DefaultMat);
			}
		}
		else if (EditMode == EEditMode::EditTiles)
		{
			for (auto& Tile : CurrentRoom->GetTiles())
			{
				if (Tile)
					SetMaterial(Tile, Properties->DefaultMat);
			}
		}
	}
}

// Spawn tile actor
TObjectPtr<ATile> UPRG_PluginRoomTool::SpawnTile(APRG_Room& ParentRoom, int IndexInRoom, FVector SpawnPos)
{
	// INFO: IndexInRoom is added to allow passing functor as template argument for SpawnTile / SpawnWall
	FActorSpawnParameters SpawnInfoTile;
	TObjectPtr<ATile> NewTile = GetWorld()->SpawnActor<ATile>(SpawnPos, FRotator(0.0f, 0.0f, 0.0f), SpawnInfoTile);
	NewTile->AttachToActor(&ParentRoom, FAttachmentTransformRules::KeepRelativeTransform);
	NewTile->GetStaticMeshComponent()->SetStaticMesh(Properties->FloorMesh);

	return NewTile;
}

// Spawn wall actor and calculate rotation
TObjectPtr<AWall> UPRG_PluginRoomTool::SpawnWall(APRG_Room& ParentRoom, int IndexInRoom, FVector SpawnPos)
{
	return SpawnWallRot(ParentRoom, SpawnPos, ParentRoom.GetWallRotationByIndex(IndexInRoom));
}

// Spawn wall actor with given rotation
TObjectPtr<AWall> UPRG_PluginRoomTool::SpawnWallRot(APRG_Room& ParentRoom, FVector SpawnPos, FRotator SpawnRot)
{
	FActorSpawnParameters SpawnInfo;
	TObjectPtr<AWall> NewWall = GetWorld()->SpawnActor<AWall>(SpawnPos, SpawnRot, SpawnInfo);
	NewWall->AttachToActor(&ParentRoom, FAttachmentTransformRules::KeepRelativeTransform);
	NewWall->GetStaticMeshComponent()->SetStaticMesh(Properties->WallMesh);

	return NewWall;
}

// Create a selection bounding box for the current room
void UPRG_PluginRoomTool::SpawnRoomBoundingBox()
{
	// Check if already added
	if (CurrentBoundingBox && CurrentBoundingBox->GetAttachParentActor() == CurrentRoom)
		return;

	auto ActiveRoom = TryFetchCurrentRoom();

	// Get room size. Has XY in tiles, so convert these to meters
	FVector RoomSize = ActiveRoom->GetRoomSize();
	RoomSize.X *= Properties->TileSize;
	RoomSize.Y *= Properties->TileSize;
	FVector SpawnScale = RoomSize + 0.1f;
	FVector CenterOffset = RoomSize * 50.0f; // M to CM, divided by 2 for center

	FTransform SpawnTransform = FTransform(FRotator(0.0f, 0.0f, 0.0f), CenterOffset, SpawnScale);
	FActorSpawnParameters SpawnInfoTile;
	TObjectPtr<ARoomBounds> NewActor = GetWorld()->SpawnActor<ARoomBounds>(ARoomBounds::StaticClass(), SpawnTransform, SpawnInfoTile);
	NewActor->AttachToActor(ActiveRoom, FAttachmentTransformRules::KeepRelativeTransform);

	NewActor->GetStaticMeshComponent()->SetStaticMesh(NewActor->CubeMesh);
	SetMaterial(NewActor, NewActor->CubeMaterial);

	CurrentBoundingBox = NewActor;
}

// Remove the selection bounding box for the current room
void UPRG_PluginRoomTool::RemoveRoomBoundingBox()
{
	if (CurrentBoundingBox)
		GetWorld()->DestroyActor(CurrentBoundingBox);
}

// Set current room and UI info
void UPRG_PluginRoomTool::SetCurrentRoom(TObjectPtr<APRG_Room> SetRoom)
{
	if (CurrentRoom && CurrentRoom != SetRoom)
		RemoveRoomBoundingBox();

	CurrentRoom = SetRoom;

	if (SetRoom)
		SpawnRoomBoundingBox();
}

// Create a room gizmo
void UPRG_PluginRoomTool::CreateRoomGizmo(TObjectPtr<APRG_Room> room, bool loadPosition)
{
	FTransform StartTransform = FTransform::Identity;

	// When restoring, set initial position to room position
	if (loadPosition)
		Properties->SpawnPosition = room->GetActorLocation();

	StartTransform.SetTranslation(Properties->SpawnPosition);

	TransformProxy = NewObject<UTransformProxy>(this);
	TransformProxy->SetTransform(StartTransform);
	TransformGizmo = UE::TransformGizmoUtil::CreateCustomTransformGizmo(GetToolManager()->GetPairedGizmoManager(),
		ETransformGizmoSubElements::StandardTranslateRotate, this);
	TransformGizmo->SetActiveTarget(TransformProxy, GetToolManager());

	// Listen for changes to the proxy and update the room when that happens
	TransformProxy->OnTransformChanged.AddUObject(this, &UPRG_PluginRoomTool::GizmoTransformChanged);

	// Store proxy with room
	room->SetRoomGizmo(std::move(TransformGizmo));
	room->SetProxyTransform(TransformProxy);
}

#undef LOCTEXT_NAMESPACE
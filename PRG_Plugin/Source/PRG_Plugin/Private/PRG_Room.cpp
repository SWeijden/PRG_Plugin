// Copyright 2022 Steven Weijden

#include "PRG_Room.h"

#include "BaseGizmos/TransformGizmoUtil.h"
#include "Tools/PRG_PluginRoomTool.h"

// localization namespace
#define LOCTEXT_NAMESPACE "APRG_Room"

// Sets default values
AWall::AWall()
{
	// Disable this actor to call Tick() every frame.
	PrimaryActorTick.bCanEverTick = false;
}

// Sets default values
ATile::ATile()
{
	// Disable this actor to call Tick() every frame.
	PrimaryActorTick.bCanEverTick = false;
}

// Sets default values
APRG_Room::APRG_Room()
{
	// Disable this actor to call Tick() every frame.
	PrimaryActorTick.bCanEverTick = false;

	BaseComponent = CreateDefaultSubobject<USceneComponent>(TEXT("BaseComponent"));
	BaseComponent->SetMobility(EComponentMobility::Type::Static);
	RootComponent = BaseComponent;
}

void APRG_Room::InitRoom(FIntPoint NewSize, int NewHeight)
{
	if (NewSize.X > 0 && NewSize.Y > 0)
		RoomSize = NewSize;
	if (NewHeight > 0)
		RoomHeight = NewHeight;

	Tiles.SetNum(RoomSize.X * RoomSize.Y, false);
	Walls.SetNum((RoomSize.X + 1) * RoomSize.Y + RoomSize.X * (RoomSize.Y + 1), false);
}

// Called when the game starts or when spawned
void APRG_Room::BeginPlay()
{
	Super::BeginPlay();

}

// Called when this actor is explicitly being destroyed during gameplay or in the editor
void APRG_Room::Destroyed()
{
	OnRoomDeletion.ExecuteIfBound(this);

	Super::Destroyed();
}

// Called every frame
void APRG_Room::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APRG_Room::SetProxyTransform(TObjectPtr<UTransformProxy> Proxy)
{
	ProxyTransform = Proxy;
}

void APRG_Room::SetRoomGizmo(TObjectPtr<UCombinedTransformGizmo> Gizmo)
{
	StoredGizmo = Gizmo;
}

TObjectPtr<UCombinedTransformGizmo> APRG_Room::GetRoomGizmo()
{
	return StoredGizmo;
}

TObjectPtr<UTransformProxy> APRG_Room::GetProxyTransform() const
{
	return ProxyTransform;
}

void APRG_Room::RemoveRoomGizmo(UInteractiveGizmoManager* GizmoManager)
{
	GizmoManager->DestroyGizmo(StoredGizmo);
}

TArray<TObjectPtr<ATile>>& APRG_Room::GetTiles()
{
	return Tiles;
}

TArray<TObjectPtr<AWall>>& APRG_Room::GetWalls()
{
	return Walls;
}

int APRG_Room::GetTileIndexByPosition(FVector Position, int TileSizeCM) const
{
	int XPos = int(Position.X);
	int YPos = int(Position.Y);
	int QuarterTileSize = TileSizeCM / 4;

	// Tiles are at 0.5 TileSize x and y pos. Check (0.25; 0.75) range by shifting -0.25
	int IndexX = (XPos - QuarterTileSize) / TileSizeCM;
	int IndexY = (YPos - QuarterTileSize) / TileSizeCM;

	return IndexX + IndexY * RoomSize.X;
}

int APRG_Room::GetWallIndexByPosition(FVector Position, int TileSizeCM) const
{
	/* Index progression example on a 3x2 grid. First vertical walls, then horizontal walls:
	 * 
	 *     0       1      2
	 *  9     10      11     12
	 *     3       4      5
	 * 13     14      15     16
	 *     6       7      8
	 */

	/* Index assignment - Set index to nearest index
	 * 
	 * Shift to positive axis by 1/4th and check if resulting modulo is less than half
	 * So for TileSizeCM of 200, at position 400, for position 350-450 consider 400
	 * Then apply normal division and drop remainder to get index. E.g. Index = 400/200
	 */

	int XPos = int(Position.X);
	int YPos = int(Position.Y);
	int QuarterTileSize = TileSizeCM / 4;
	int HalfTileSize = TileSizeCM / 2;

	// X-aligned walls at 0.5 TileSize x pos. Check (0.25; 0.75) range by shifting -0.25
	if ((TileSizeCM + XPos - QuarterTileSize) % TileSizeCM < HalfTileSize)
	{
		int IndexX = (XPos - QuarterTileSize) / TileSizeCM;
		int IndexY = (YPos + QuarterTileSize) / TileSizeCM;

		return IndexX + IndexY * RoomSize.X;
	}
	// Y-aligned walls at each TileSize x pos. Check (-0.25; 0.25) range by shifting +0.25
	else //if ((XPos + QuarterTileSize) % TileSizeCM < HalfTileSize)
	{
		int IndexX = (XPos + QuarterTileSize) / TileSizeCM;
		int IndexY = (YPos - QuarterTileSize) / TileSizeCM;

		int AddIndex = RoomSize.X * (RoomSize.Y + 1);
		return AddIndex + IndexX + IndexY * (RoomSize.X + 1);
	}
}

FVector APRG_Room::GetTilePositionFromIndex(int Index, int TileSizeCM) const
{
	int HalfTileSize = TileSizeCM / 2;
	return FVector(
		(Index % RoomSize.X) * TileSizeCM + HalfTileSize,
		(Index / RoomSize.X) * TileSizeCM + HalfTileSize,
		0.0f
	);
}

FVector APRG_Room::GetWallPositionFromIndex(int Index, int TileSizeCM) const
{
	int HalfTileSize = TileSizeCM / 2;
	// X-aligned walls
	if (Index < RoomSize.X * (RoomSize.Y + 1))
	{
		return FVector(
			(Index % RoomSize.X) * TileSizeCM + HalfTileSize,
			(Index / RoomSize.X) * TileSizeCM,
			0.0f
		);
	}
	// Y-aligned walls
	else // (RoomSize.X + 1) * SizeY
	{
		Index -= RoomSize.X * (RoomSize.Y + 1);

		return FVector(
			(Index % (RoomSize.X + 1)) * TileSizeCM,
			(Index / (RoomSize.X + 1)) * TileSizeCM + HalfTileSize,
			0.0f
		);
	}
}

void APRG_Room::SetTileAtIndex(int Index, TObjectPtr<ATile> NewTile)
{
	if (NewTile && Index < Tiles.Num())
		Tiles[Index] = NewTile;
}

void APRG_Room::SetWallAtIndex(int Index, TObjectPtr<AWall> NewWall)
{
	if (NewWall && Index < Walls.Num())
		Walls[Index] = NewWall;
}

FRotator APRG_Room::GetWallRotationByIndex(int Index) const
{
	if (Index < RoomSize.X * (RoomSize.Y + 1))
		return FRotator(0.0f, 0.0f, 0.0f);
	else
		return FRotator(0.0f, 90.0f, 0.0f);
}

#undef LOCTEXT_NAMESPACE
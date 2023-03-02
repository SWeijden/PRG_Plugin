// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include <Tools/PRG_PluginRoomTool.h>

#include "PRG_Settings.generated.h"

/**
 * PRG settings to be saved in the scene
 */
UCLASS()
class PRG_PLUGIN_API APRG_Settings : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY()
	EPosSnap PositionSnap;
	UPROPERTY()
	ERotSnap RotationSnap;
	UPROPERTY()
	FIntPoint RoomSize;
	UPROPERTY()
	bool ShowAllGizmos;
	//UPROPERTY()
	//float GizmoScale;
	UPROPERTY()
	int InitHeight;
	UPROPERTY()
	int TileSize;
	UPROPERTY()
	FVector SpawnPosition;
	UPROPERTY()
	TObjectPtr<UStaticMesh> FloorMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> WallMesh;

	void InitSettings(EPosSnap _PositionSnap, ERotSnap _RotationSnap, FIntPoint _InitSize,
		bool _ShowAllGizmos, /*float _GizmoScale,*/ int _InitHeight, int _TileSize,
		FVector _SpawnPosition, TObjectPtr<UStaticMesh> _FloorMesh, TObjectPtr<UStaticMesh> _WallMesh)
	{
		PositionSnap		= _PositionSnap;
		RotationSnap		= _RotationSnap;
		RoomSize				= _InitSize;
		ShowAllGizmos		= _ShowAllGizmos;
		//GizmoScale			= _GizmoScale;
		InitHeight			= _InitHeight;
		TileSize				= _TileSize;
		SpawnPosition		= _SpawnPosition;
		FloorMesh				= _FloorMesh;
		WallMesh				= _WallMesh;
	};
};

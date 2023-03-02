# PRG_Plugin
A procedural room generator plugin for UE5
Version 1.1

----------------------------------------------------------------------------------------------------------------------

## Setup
Add the project files to in their own folder inside the Unreal Engine Plugins folder.
This should be adjacent to your Content folder, if it doesn't exist then create it.
You should end up with the files inside Plugins/PRG_Plugin/

After rebuilding on the next time you start up your project, the plugin will be ready.
In the menu bar, under 'Edit -> Plugins', under 'Project -> Editor Tools' you can view the plugin details.

----------------------------------------------------------------------------------------------------------------------

## Using the tool

After selecting the Room Generator Mode, you will see a list of options.
Edit Modes provides 4 modes for manipulating rooms and their content, discussed below.
Position and rotation snapping allow for locking these to fixed increments.
ShowAllGizmos allows toggling between showing only a gizmo on the active room or on all rooms.

Details about the Edit Modes:
  - Create Rooms:
  This mode allows creating a room of the given size at the given position by adding an element to the Rooms array in the Overview
    * Clicking in the scene sets the spawn position for a new room.
    * Room height is used for showing the bounding box when a room is selected.
	* Tile size should match the size in meters of the floor tile mesh.
    * Wall and Floor objects change be changed here from their defaults.
  - Manage Rooms:
    * Clicking in the scene will switch selection to another room if part of it.
	* Changing the room size will add tiles or remove walls and tiles where appropriate.
	* Changing the default meshes will cause these to be used when changing the room size.
	* Rooms can be deleted via the scene or by clearing its Rooms array entry.
  - Edit Walls:
  For the currently selected room you can add or remove walls.
    * Clicking on a wall will select it, turning it green. You can click again to toggle between keeping or removing the wall.
    * Clicking on the wall of another room switches the selection to that room and wall.
    * In the tool tab, you can set a new mesh onto the Wall Object to replace the current mesh.
    * Some example alternative wall objects are provided in the PRG_Plugin Content/Meshes folder.
  - Edit Tiles:
  For the currently selected room you can add or remove tiles.
    * Clicking on a tile will select it, turning it green. You can click again to toggle between keeping or removing the tile.
	* Clicking on the tile of another room switches the selection to that room and tile.
    * In the tool tab, you can drag and drop a new mesh onto the Floor Object to replace the current mesh.

----------------------------------------------------------------------------------------------------------------------

## Extra details

#### Changes in version 1.1:
  - Fixed a issue preventing the tool from working in UE 5.1.
  - Settings will now be stored in an object in the map to retain the tool options and data variables.
  - Materials of meshes will be restored when leaving edit modes for walls and tiles.
  - Room selection can be switched by clicking on part of a room in the Manage Rooms mode.
  - The default walls and tiles now have 2 materials on each side for easy manipulation.

#### Known issues:
  - Do not insert items in the Rooms array. This will break the internal state of the tool and may cause a crash on subsequent interactions. Exit / enter the tool again to restore the internal state.
  - Do not delete walls or tiles while inside the tool. This will break the internal state of the tool and will cause a crash when editing walls or tiles of that room. Exit / enter the tool again to restore the internal state.
  - Do not change the content of an item in the room array. This may delete a room and break the internal state of the tool. You can undo the deletion to restore the room and exit / enter the tool again to restore the internal state.
  - Undo/Redo is not yet supported within the tool. Movement and rotation can be reverted as normal, but undoing room creation or deletion will break the internal state of the tool. Exit / enter the tool again to restore the internal state.
  - Moving rooms without the gizmo is not supported. These changes will revert when afterwards moving the room with the gizmo.

#### Lazy workarounds:
- Room duplication:
Rooms and their content can be duplicated outside of the tool and will be recognized upon entering the tool.
- Walls and tiles duplication:
As long as walls and tiles remain within the bounds of the room and are parented to the room, these can be added or removed outside of the tool.

#### Future considerations:
- Adding a spawn marker for the place where a new room will be created.
- Change tile size and room height to be set in cm instead of meters.
- Try to find ways to prevent interactions that will break the tool's internal state.

----------------------------------------------------------------------------------------------------------------------

#### Acknowledgements:
The example walls and tiles were made with Blender and their collisions were made using Xavier150's Blender For Unreal addon for Blender.
This can be used in combination with the Send to Unreal addon from Epic Games to easily transfer meshes with collisions.
- Blender for Unreal:	https://github.com/xavier150/Blender-For-UnrealEngine-Addons
- Send to Unreal: https://epicgames.github.io/BlenderTools/send2ue/introduction/quickstart.html

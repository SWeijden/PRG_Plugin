# PRG_Plugin
A procedural room generator plugin for UE5, Version 1.3

Tested with Unreal Engine 5.1

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
  	* Can clear or reset the walls or floors of a room using the toggle in the menu.
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

#### Changes in version 1.2:
  - Added ability to move any room using their gizmo when ShowAllGizmos is enabled.
  - Added spawn room marker to indicate where a new room will be created.
  - Added options to reset or clear the walls or floors of a room while in EditRoom mode.

#### Changes in version 1.3:
  - Fixed an issue when deleting all rooms when a room had been selected, causing a crash when adding a new room.
  - Fixed potential crash when inserting items into the room array. Now this is treated the same as adding a room.
  - Fixed an issue with showing new room gizmo when ShowAllGizmo's is on.
  - Fixed an issue when manually deleting a Wall or Tile while using the tool.
  - Fixed issues when manually deleting a room while in Wall or Tile editmode.
  - Fixed cleanup on exiting the tool, so that relevant data is cleared.
  - Added syncing tool and scene selection of rooms, allowing for easy deletion of rooms.
  - Added checks to trigger execution halt when internal state of tool is broken due to currently unsupported actions such as undoing/redoing room deletion/addition, to inform user to exit and reopen the tool to avoid further issues.

#### Known issues:
  - Undo/Redo is not (yet) supported within the tool. Movement and rotation can be reverted as normal, but undoing room creation or deletion will break the internal state of the tool. Exit / enter the tool again to restore the internal state.
  - Moving rooms within the tool without using a gizmo is not supported. These changes will revert when moving the room with the gizmo.

#### Usage tips:
- Room duplication:
Rooms and their content can be duplicated outside of the tool and will be recognized upon entering the tool.
- Walls and tiles duplication:
Walls and tiles within a room can be duplicated outside the tool and will be recognized upon entering the tool.
- Room deletion:
Rooms can be deleted using the keyboard while using the tool in any edit mode.

#### Future considerations:
- Change tile size and room height to be set in cm instead of meters.
- Add support for undo/redo.

----------------------------------------------------------------------------------------------------------------------

#### Acknowledgements:
The example walls and tiles were made with Blender and their collisions were made using Xavier150's Blender For Unreal addon for Blender.
This can be used in combination with the Send to Unreal addon from Epic Games to easily transfer meshes with collisions.
- Blender for Unreal:	https://github.com/xavier150/Blender-For-UnrealEngine-Addons
- Send to Unreal: 	https://epicgames.github.io/BlenderTools/send2ue/introduction/quickstart.html

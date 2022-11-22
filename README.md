# PRG_Plugin
A procedural room generator plugin for UE5

----------------------------------------------------------------------------------------------------------------------

## Setup
Add the project files to in their own folder inside the Unreal Engine Plugins folder.
This should be adjacent to your Content folder, if it doesn't exist, then create it.
You should end up with the files inside Plugins/PRG_Plugin/

After rebuilding on the next time you start up your project, the plugin will now be ready.
In the menu bar, under Edit -> Plugins, under Project -> Editor Tools you can view the plugin details.

----------------------------------------------------------------------------------------------------------------------

## Using the tool
In the top left, you can now change the Select Mode to the Room Generator mode.
Here you will have 4 modes at your disposal:
  - View Mode:
  Only shows the default transform handles for each room created with the plugin in the scene.
  - Manage Rooms:
  This allows you to add new rooms and shows a bounding box overlay for the currently selected room.
    * Clicking in the scene will set the Spawn Position for placing a new room.
    * Creating a new room is done via the Manage -> Room Overview and adding an array item
      The Floor and Wall object's will be used to create the room with.
    * Deleting a room can be done in the scene or via clearing / deleting the array item
  - Edit Walls:
  For the currently selected room you can add or remove walls.
    * Clicking on a wall will select it, turning it green. You can click again to toggle between keeping or removing the wall.
      Clicking on the wall of another room switches the selection to that room.
    * In the tool tab, you can also drag and drop a new mesh onto the Wall Object to replace the current one.
    * Some example alternative objects are provided in the PRG_Plugin Content/Meshes folder.
  - Edit Tiles:
  For the currently selected room you can add or remove tiles.
    * Clicking on a tile will select it, turning it green. You can click again to toggle between keeping or removing the tile.
      Clicking on the tile of another room switches the selection to that room.
    * In the tool tab, you can also drag and drop a new mesh onto the Floor Object to replace the current one.


----------------------------------------------------------------------------------------------------------------------

#### Known issues:
- Room inserts. Do not insert new items into the room array, this will break things soon after.
If an item was inserted, please go out of the Room Generator mode and return. The room array will be restored.
- Room rotation. Either directly or using the gizmo to rotate a room is not yet supported. Using the room gizmo afterwards will clear any rotations.
- Room movement. Moving a room without using the gizmo, while in the Room Generator mode is not supported.
- Undo/Redo is not yet supported. If done, try to restore the plugin's state by leaving and entering the Room Generator mode again.

#### Lazy workarounds:
- Room Duplication: Go out of the Room Generator mode, select the room root and any or all content you wish to duplicate.
Afterwards open the Room Generator mode back up and the room will be properly recognized.
Bonus version: You can even do this with the EditWalls (or EditTiles) mode active, duplicating the temporary objects and then switching out/into the Room Generator mode. If you then enable/disable the same edit mode, the duplicated objects will have their materials updated to match the other persistent objects.

#### Future considerations:
- Handle or prevent inserting into the array.
- Integrate restoring previous materials. Currently using the tool will assign the default material after switching out of the EditWalls or EditTiles modes.
- Adding a spawn marker for the place where a new room will be created.
- Save custom materials. For now changing the defaults 

----------------------------------------------------------------------------------------------------------------------

#### Extra:
- The example walls and tiles were made with Blender and their collisions were made using Xavier150's Blender For Unreal addon for Blender.
The current usage is very simply, but the tool has a lot of possibilities.
Link: https://github.com/xavier150/Blender-For-UnrealEngine-Addons

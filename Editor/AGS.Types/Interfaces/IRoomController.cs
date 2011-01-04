using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;

namespace AGS.Types
{
    public delegate void PreSaveRoomHandler(ILoadedRoom roomBeingSaved, CompileMessages errors);

	public interface IRoomController
	{
		/// <summary>
		/// Returns the currently loaded room, or null if none is loaded.
		/// RequiredAGSVersion: 3.0.1.35
		/// </summary>
		ILoadedRoom CurrentRoom { get; }
		/// <summary>
		/// Loads the specified room (from the Game.Rooms collection) into memory.
		/// If another room is currently loaded, the user will be prompted to
		/// save it. Returns true if the room was loaded.
		/// RequiredAGSVersion: 3.0.1.35
		/// </summary>
		bool LoadRoom(IRoom roomToLoad);
		/// <summary>
		/// Gets the area number on the specified room mask at (x,y)
		/// RequiredAGSVersion: 3.0.1.35
		/// </summary>
		int  GetAreaMaskPixel(RoomAreaMaskType maskType, int x, int y);
		/// <summary>
		/// Draws the room background to the specified graphics context.
		/// RequiredAGSVersion: 3.0.1.35
		/// </summary>
		void DrawRoomBackground(Graphics g, int x, int y, int backgroundNumber, int scaleFactor);
		/// <summary>
		/// Draws the room background to the specified graphics context,
		/// and overlays one of the room masks onto it.
		/// RequiredAGSVersion: 3.0.1.35
		/// </summary>
		void DrawRoomBackground(Graphics g, int x, int y, int backgroundNumber, int scaleFactor, RoomAreaMaskType maskType, int maskTransparency, int selectedArea);
		/// <summary>
		/// Sets whether or not to grey out non-selected masks when drawing the background.
		/// </summary>
		bool GreyOutNonSelectedMasks { set; }
        /// <summary>
        /// Occurs when a room is about to be saved to disk. You can add a new CompileError
        /// to the errors collection if you want to prevent the save going ahead.
        /// RequiredAGSVersion: 3.2.0.95
        /// </summary>
        event PreSaveRoomHandler PreSaveRoom;
	}
}

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
        /// Saves the loaded room to disk.
        /// </summary>
        void Save();
        /// <summary>
        /// Gets the loaded room backround as a <see cref="Bitmap"/> instance.
        /// </summary>
        /// <param name="background">The background index to get.</param>
        /// <returns>A <see cref="Bitmap"/> instance with the selected background. Returns null if background doesn't exist.</returns>
        Bitmap GetBackground(int background);
        /// <summary>
        /// Sets the loaded room's background frame.
        /// </summary>
        /// <param name="background">The background index to set.</param>
        /// <param name="bmp">The image to use for the frame.</param>
        void SetBackground(int background, Bitmap bmp);
        /// <summary>
        /// Deletes the loaded room's background frame.
        /// </summary>
        /// <param name="background">The background index to delete.</param>
        void DeleteBackground(int background);
        /// <summary>
        /// Gets the loaded room specified mask as a <see cref="Bitmap"/> instance.
        /// </summary>
        /// <param name="mask">The mask type to get.</param>
        /// <returns>A <see cref="Bitmap"/> instance with the selected mask type.Return null if none is selected.</returns>
        Bitmap GetMask(RoomAreaMaskType mask);
        /// <summary>
        /// Sets the loaded room specified mask.
        /// </summary>
        /// <param name="mask">The mask type to set.</param>
        /// <param name="bmp">The mask to set.</param>
        void SetMask(RoomAreaMaskType mask, Bitmap bmp);
        /// <summary>
        /// Gets the area number on the specified room mask at (x,y)
        /// RequiredAGSVersion: 3.0.1.35
        /// </summary>
        int  GetAreaMaskPixel(RoomAreaMaskType maskType, int x, int y);
        /// <summary>
        /// Draws the room background to the specified graphics context.
        /// RequiredAGSVersion: 3.0.1.35
        /// </summary>
        [Obsolete("The method is deprecated because it takes integer for scale which is inaccurate. Use overload with double for scale instead.")]
        void DrawRoomBackground(Graphics g, int x, int y, int backgroundNumber, int scaleFactor);
		/// <summary>
		/// Draws the room background to the specified graphics context,
		/// and overlays one of the room masks onto it.
		/// RequiredAGSVersion: 3.0.1.35
		/// </summary>
        [Obsolete("The method is deprecated because it takes integer for scale which is inaccurate. Use overload with double for scale instead.")]
		void DrawRoomBackground(Graphics g, int x, int y, int backgroundNumber, int scaleFactor, RoomAreaMaskType maskType, int maskTransparency, int selectedArea);
        /// <summary>
        /// Draws the room background to the specified graphics context,
        /// and overlays one of the room masks onto it.
        /// </summary>
        void DrawRoomBackground(Graphics g, int x, int y, int backgroundNumber, double scaleFactor, RoomAreaMaskType maskType, int maskTransparency, int selectedArea);
        /// <summary>
        /// Copies the walkable area into regions mask.
        /// </summary>
        void CopyWalkableAreaMaskToRegions();
        /// <summary>
        /// Scales all the room's masks according to the <see cref="Room.MaskResolution"/>,
        /// execept for the <see cref="RoomAreaMaskType.WalkBehinds"/> which retains the
        /// resolution of the background image. If the <see cref="Room.MaskResolution"/>
        /// has a resolution of 2 it will halve the masks in both dimensions.
        /// </summary>
        void AdjustMaskResolution();
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

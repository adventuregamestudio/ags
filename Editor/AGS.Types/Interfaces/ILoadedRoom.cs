using System;
using System.Collections.Generic;

namespace AGS.Types
{
	public interface ILoadedRoom
	{
		int BackgroundAnimationDelay { get; set; }
		int BackgroundCount { get; }
		int BottomEdgeY { get; set; }
		int ColorDepth { get; }
		int GameID { get; }
		int Height { get; }
		IList<RoomHotspot> Hotspots { get; }
		Interactions Interactions { get; }
		int LeftEdgeX { get; set; }
		IList<RoomMessage> Messages { get; }
		bool Modified { get; set; }
		RoomVolumeAdjustment MusicVolumeAdjustment { get; set; }
        /// <summary>
        /// Gets the room number of this room.
        /// RequiredAGSVersion: 3.2.0.95
        /// </summary>
        int Number { get; }
		IList<RoomObject> Objects { get; }
		int PlayerCharacterView { get; set; }
		int PlayMusicOnRoomLoad { get; set; }
		CustomProperties Properties { get; }
		IList<RoomRegion> Regions { get; }
		RoomResolution Resolution { get; }
		int RightEdgeX { get; set; }
		event Room.RoomModifiedChangedHandler RoomModifiedChanged;
		bool SaveLoadEnabled { get; set; }
		bool ShowPlayerCharacter { get; set; }
		int TopEdgeY { get; set; }
		IList<RoomWalkableArea> WalkableAreas { get; }
		IList<RoomWalkBehind> WalkBehinds { get; }
		int Width { get; }
	}
}

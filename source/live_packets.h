//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#ifndef LIVE_PACKETS_H
#define LIVE_PACKETS_H

enum LivePacketType {
	PACKET_HELLO_FROM_CLIENT = 0x10,
	PACKET_READY_CLIENT = 0x11,

	PACKET_REQUEST_NODES = 0x20,
	PACKET_CHANGE_LIST = 0x21,
	PACKET_ADD_HOUSE = 0x23,
	PACKET_EDIT_HOUSE = 0x24,
	PACKET_REMOVE_HOUSE = 0x25,

	PACKET_CLIENT_TALK = 0x30,
	PACKET_CLIENT_UPDATE_CURSOR = 0x31,
	PACKET_CLIENT_COLOR_UPDATE = 0x32,

	PACKET_HELLO_FROM_SERVER = 0x80,
	PACKET_KICK = 0x81,
	PACKET_ACCEPTED_CLIENT = 0x82,
	PACKET_CHANGE_CLIENT_VERSION = 0x83,
	PACKET_SERVER_TALK = 0x84,
	PACKET_COLOR_UPDATE = 0x85,

	PACKET_NODE = 0x90,
	PACKET_CURSOR_UPDATE = 0x91,
	PACKET_START_OPERATION = 0x92,
	PACKET_UPDATE_OPERATION = 0x93,
	PACKET_CHAT_MESSAGE = 0x94,

	// New packet types for conflict resolution
	PACKET_SECTOR_LOCK_REQUEST = 0x95,     // Client requests exclusive edit access to a sector
	PACKET_SECTOR_LOCK_RESPONSE = 0x96,    // Server grants or denies lock request
	PACKET_SECTOR_LOCK_RELEASE = 0x97,     // Client releases lock on a sector
	PACKET_SECTOR_CONFLICT = 0x98,         // Server notifies of a conflict
	PACKET_SECTOR_CONFLICT_RESOLVE = 0x99, // Client decides how to resolve conflict
	PACKET_SECTOR_SNAPSHOT = 0x9A,         // Full sector data for recovery or initialization
	PACKET_SECTOR_VERSION = 0x9B,          // Sector version update
};

// Define sector size - note that this is different from QTreeNode's 4x4 grid
// We're using a larger sector size to group multiple QTreeNodes together
// Since QTreeNode is a 4x4 grid, we'll use a sector size that's a multiple of 4
#define SECTOR_SIZE 64

#endif

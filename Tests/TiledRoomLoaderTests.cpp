#include "TiledRoom.h"

#include <cassert>
#include <string>

int main()
{
	const TiledRoomData firstRoom = TiledRoomLoader::Load("Resources/maps/rooms/room_01.tmj");
	assert(firstRoom.width == 20);
	assert(firstRoom.height == 12);
	assert(firstRoom.solidTiles.size() == 240);
	assert(firstRoom.IsSolid(0, 0));
	assert(firstRoom.IsSolid(19, 11));
	assert(!firstRoom.IsSolid(1, 10));
	assert(firstRoom.FindSpawn("start") != nullptr);
	assert(firstRoom.FindSpawn("start")->position.x == 2.0f);
	assert(firstRoom.FindSpawn("start")->position.y == 1.0f);
	assert(firstRoom.FindSpawn("from_room_02") != nullptr);
	assert(firstRoom.exits.size() == 1);
	assert(firstRoom.exits.front().targetRoom == "room_02.tmj");
	assert(firstRoom.exits.front().targetSpawn == "from_room_01");
	assert(firstRoom.exits.front().bounds.min.x == 18.5f);
	assert(firstRoom.exits.front().bounds.max.x == 19.5f);
	assert(firstRoom.checkpoints.size() == 1);

	const TiledRoomData secondRoom = TiledRoomLoader::Load("Resources/maps/rooms/room_02.tmj");
	assert(secondRoom.width == 20);
	assert(secondRoom.height == 12);
	assert(secondRoom.FindSpawn("from_room_01") != nullptr);
	assert(secondRoom.exits.size() == 1);
	assert(secondRoom.exits.front().targetRoom == "room_01.tmj");
	return 0;
}

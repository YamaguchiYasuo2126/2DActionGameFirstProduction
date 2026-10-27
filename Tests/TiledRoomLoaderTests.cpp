#include "TiledRoom.h"
#include "SwitchLinkState.h"

#include <algorithm>
#include <cassert>
#include <array>
#include <string>

int main()
{
	const TiledRoomData firstRoom = TiledRoomLoader::Load("Resources/maps/rooms/room_01.tmj");
	assert(firstRoom.width > 0 && firstRoom.height > 0);
	assert(firstRoom.solidTiles.size() == static_cast<size_t>(firstRoom.width) * firstRoom.height);
	assert(firstRoom.FindSpawn("start") != nullptr);
	assert(firstRoom.gimmicks.size() == 5);
	const auto ring = std::find_if(firstRoom.gimmicks.begin(), firstRoom.gimmicks.end(),
		[](const TiledGimmickData& gimmick) { return gimmick.type == GimmickType::kAirRushRing; });
	assert(ring != firstRoom.gimmicks.end());
	assert(ring->position.x == 16.0f && ring->position.y == 6.0f);

	const TiledRoomData secondRoom = TiledRoomLoader::Load("Resources/maps/rooms/room_02.tmj");
	assert(secondRoom.width > 0 && secondRoom.height > 0);
	assert(secondRoom.FindSpawn("from_room_01") != nullptr);
	assert(secondRoom.gimmicks.size() == 7);
	const auto oreSwitch = std::find_if(secondRoom.gimmicks.begin(), secondRoom.gimmicks.end(),
		[](const TiledGimmickData& gimmick) { return gimmick.type == GimmickType::kOreSwitch; });
	const auto airSwitch = std::find_if(secondRoom.gimmicks.begin(), secondRoom.gimmicks.end(),
		[](const TiledGimmickData& gimmick) { return gimmick.type == GimmickType::kAirRushSwitch; });
	assert(oreSwitch != secondRoom.gimmicks.end() && oreSwitch->linkId == "ore_path");
	assert(airSwitch != secondRoom.gimmicks.end() && airSwitch->linkId == "air_path");
	for (const TiledGimmickData& gimmick : secondRoom.gimmicks)
	{
		if (gimmick.type == GimmickType::kStoneBridge)
		{
			assert(gimmick.linkId == (gimmick.position.x == 9.0f ? "ore_path" : "air_path"));
		}
	}
	const TiledRoomData thirdRoom = TiledRoomLoader::Load("Resources/maps/rooms/room_03.tmj");
	assert(thirdRoom.width > 0 && thirdRoom.height > 0);
	assert(thirdRoom.FindSpawn("from_room_02") != nullptr);
	assert(thirdRoom.gimmicks.size() == 3);
	std::array<bool, 13> placedTypes{};
	for (const TiledRoomData* room : {&firstRoom, &secondRoom, &thirdRoom})
	{
		for (const TiledGimmickData& gimmick : room->gimmicks)
		{
			const size_t index = static_cast<size_t>(gimmick.type);
			assert(index < placedTypes.size());
			placedTypes[index] = true;
		}
	}
	for (const bool placed : placedTypes)
	{
		assert(placed);
	}

	const TiledRoomData gimmickRoom = TiledRoomLoader::Load("Tests/fixtures/gimmick_room.tmj");
	assert(gimmickRoom.width == 4 && gimmickRoom.height == 4);
	assert(gimmickRoom.IsSolid(0, 3));
	assert(!gimmickRoom.IsSolid(0, 2));
	assert(gimmickRoom.FindSpawn("start") != nullptr);
	assert(gimmickRoom.FindSpawn("start")->position.x == 1.0f);
	assert(gimmickRoom.FindSpawn("start")->position.y == 1.0f);
	assert(gimmickRoom.exits.size() == 1);
	assert(gimmickRoom.exits.front().targetRoom == "other_room.tmj");
	assert(gimmickRoom.exits.front().targetSpawn == "from_fixture");
	assert(gimmickRoom.checkpoints.size() == 1);
	assert(gimmickRoom.triangles.size() == 1);
	assert(gimmickRoom.triangles.front().vertices[0].x == 0.0f);
	assert(gimmickRoom.triangles.front().vertices[2].y == 2.0f);

	const std::array<std::string, 13> gimmickTypeNames = {
		"FountainJump", "MovingPlatform", "WaterWheelLift", "Updraft", "BounceLeaf",
		"FallingRock", "Vine", "OreSwitch", "StoneBridge", "VanishingCloud", "ThunderCloud",
		"AirRushRing", "AirRushSwitch"};
	for (size_t index = 0; index < gimmickTypeNames.size(); ++index)
	{
		const auto type = ParseGimmickType(gimmickTypeNames[index]);
		assert(type.has_value());
		assert(static_cast<size_t>(*type) == index);
	}
	assert(!ParseGimmickType("UnknownGimmick").has_value());

	assert(gimmickRoom.gimmicks.size() == 5);
	assert(gimmickRoom.gimmicks[0].id == "spring_1");
	assert(gimmickRoom.gimmicks[0].type == GimmickType::kFountainJump);
	assert(gimmickRoom.gimmicks[0].position.x == 1.0f);
	assert(gimmickRoom.gimmicks[0].position.y == 1.0f);
	assert(gimmickRoom.gimmicks[1].type == GimmickType::kOreSwitch);
	assert(gimmickRoom.gimmicks[1].linkId == "fixture_path");
	assert(gimmickRoom.gimmicks[2].type == GimmickType::kStoneBridge);
	assert(gimmickRoom.gimmicks[2].linkId == "fixture_path");
	assert(gimmickRoom.gimmicks[3].id == "tile_0_1");
	assert(gimmickRoom.gimmicks[3].type == GimmickType::kMovingPlatform);
	assert(gimmickRoom.gimmicks[3].position.x == 0.0f);
	assert(gimmickRoom.gimmicks[3].position.y == 2.0f);
	assert(gimmickRoom.gimmicks[4].id == "tile_3_1");
	assert(gimmickRoom.gimmicks[4].type == GimmickType::kThunderCloud);
	assert(gimmickRoom.gimmicks[4].position.x == 3.0f);
	assert(gimmickRoom.gimmicks[4].position.y == 2.0f);

	SwitchLinkState links;
	links.Activate("ore_path", 300.0f);
	assert(links.IsActive("ore_path"));
	assert(!links.IsActive("air_path"));
	links.Activate("air_path", 120.0f);
	links.Update(120.0f);
	assert(links.IsActive("ore_path"));
	assert(!links.IsActive("air_path"));
	links.Update(180.0f);
	assert(!links.IsActive("ore_path"));
	return 0;
}

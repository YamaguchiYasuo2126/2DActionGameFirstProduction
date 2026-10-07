#pragma once

#include "AABB.h"
#include "KamataEngine.h"

#include <cstdint>
#include <string>
#include <vector>

struct TiledSpawnData
{
	std::string className;
	std::string id;
	std::string enemyType;
	KamataEngine::Vector3 position{};
};

struct TiledRoomExitData
{
	std::string id;
	std::string targetRoom;
	std::string targetSpawn;
	AABB bounds{};
};

struct TiledCheckpointData
{
	std::string id;
	KamataEngine::Vector3 position{};
};

struct TiledRoomData
{
	std::string roomId;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t tileWidth = 0;
	uint32_t tileHeight = 0;
	std::vector<uint8_t> solidTiles;
	std::vector<TiledSpawnData> spawns;
	std::vector<TiledRoomExitData> exits;
	std::vector<TiledCheckpointData> checkpoints;

	bool IsSolid(uint32_t x, uint32_t y) const;
	const TiledSpawnData* FindSpawn(const std::string& spawnId) const;
};

class TiledRoomLoader
{
public:
	static TiledRoomData Load(const std::string& filePath);
};

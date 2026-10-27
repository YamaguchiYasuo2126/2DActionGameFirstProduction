#pragma once

#include "AABB.h"
#include "KamataEngine.h"
#include "StageGimmick.h"

#include <cstdint>
#include <array>
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

struct TiledGimmickData
{
	std::string id;
	GimmickType type = GimmickType::kFountainJump;
	KamataEngine::Vector3 position{};
	std::string linkId;
};

struct TiledTriangleData
{
	std::string id;
	std::array<KamataEngine::Vector3, 3> vertices{};
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
	std::vector<TiledGimmickData> gimmicks;
	std::vector<TiledTriangleData> triangles;

	bool IsSolid(uint32_t x, uint32_t y) const;
	const TiledSpawnData* FindSpawn(const std::string& spawnId) const;
};

class TiledRoomLoader
{
public:
	static TiledRoomData Load(const std::string& filePath);
};

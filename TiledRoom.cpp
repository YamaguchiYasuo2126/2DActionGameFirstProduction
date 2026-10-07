#include "TiledRoom.h"

#include "ThirdParty/nlohmann/json.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>

using json = nlohmann::json;
using namespace KamataEngine;

namespace
{

constexpr uint32_t kTiledFlipFlags = 0xE0000000u;

std::string GetObjectClass(const json& object)
{
	// Tiled 1.10以降のObjectのClassはJSON上ではtypeとして保存される。
	const std::string objectType = object.value("type", "");
	return !objectType.empty() ? objectType : object.value("class", "");
}

template <class T>
T GetProperty(const json& owner, const std::string& name, const T& defaultValue)
{
	if (!owner.contains("properties") || !owner["properties"].is_array())
	{
		return defaultValue;
	}

	for (const json& property : owner["properties"])
	{
		if (property.value("name", "") == name && property.contains("value"))
		{
			return property["value"].get<T>();
		}
	}

	return defaultValue;
}

Vector3 PixelPointToWorld(const TiledRoomData& room, double pixelX, double pixelY)
{
	const float tileX = static_cast<float>(pixelX / static_cast<double>(room.tileWidth));
	const float tileY = static_cast<float>(pixelY / static_cast<double>(room.tileHeight));
	return {tileX - 0.5f, static_cast<float>(room.height) - tileY - 0.5f, 0.0f};
}

AABB PixelRectangleToWorld(const TiledRoomData& room, const json& object)
{
	const double x = object.value("x", 0.0);
	const double y = object.value("y", 0.0);
	const double width = object.value("width", 0.0);
	const double height = object.value("height", 0.0);
	const Vector3 topLeft = PixelPointToWorld(room, x, y);
	const Vector3 bottomRight = PixelPointToWorld(room, x + width, y + height);
	return {{topLeft.x, bottomRight.y, -1.0f}, {bottomRight.x, topLeft.y, 1.0f}};
}

void ReadTileData(TiledRoomData& room, const json& layer)
{
	if (layer.contains("data") && layer["data"].is_array())
	{
		const json& data = layer["data"];
		const size_t count = (std::min)(data.size(), room.solidTiles.size());
		for (size_t index = 0; index < count; ++index)
		{
			const uint32_t gid = data[index].get<uint32_t>() & ~kTiledFlipFlags;
			room.solidTiles[index] = gid == 0 ? 0u : 1u;
		}
		return;
	}

	// 宣言済みのマップ範囲内であれば、Tiledのchunk形式にも対応する。
	if (!layer.contains("chunks") || !layer["chunks"].is_array())
	{
		return;
	}

	for (const json& chunk : layer["chunks"])
	{
		const int32_t startX = chunk.value("x", 0);
		const int32_t startY = chunk.value("y", 0);
		const int32_t chunkWidth = chunk.value("width", 0);
		const int32_t chunkHeight = chunk.value("height", 0);
		const json& data = chunk.at("data");

		for (int32_t localY = 0; localY < chunkHeight; ++localY)
		{
			for (int32_t localX = 0; localX < chunkWidth; ++localX)
			{
				const int32_t x = startX + localX;
				const int32_t y = startY + localY;
				if (x < 0 || y < 0 || x >= static_cast<int32_t>(room.width) || y >= static_cast<int32_t>(room.height))
				{
					continue;
				}

				const size_t sourceIndex = static_cast<size_t>(localY * chunkWidth + localX);
				const uint32_t gid = data.at(sourceIndex).get<uint32_t>() & ~kTiledFlipFlags;
				room.solidTiles[static_cast<size_t>(y) * room.width + static_cast<size_t>(x)] = gid == 0 ? 0u : 1u;
			}
		}
	}
}

void ReadObjects(TiledRoomData& room, const json& layer)
{
	if (!layer.contains("objects") || !layer["objects"].is_array())
	{
		return;
	}

	for (const json& object : layer["objects"])
	{
		const std::string className = GetObjectClass(object);
		const std::string id = object.value("name", "");

		if (className == "PlayerSpawn" || className == "EnemySpawn")
		{
			TiledSpawnData spawn;
			spawn.className = className;
			spawn.id = id;
			spawn.enemyType = GetProperty<std::string>(object, "enemyType", "WalkEnemy");
			spawn.position = PixelPointToWorld(room, object.value("x", 0.0), object.value("y", 0.0));
			room.spawns.push_back(spawn);
		}
		else if (className == "RoomExit")
		{
			TiledRoomExitData exit;
			exit.id = id;
			exit.targetRoom = GetProperty<std::string>(object, "targetRoom", "");
			exit.targetSpawn = GetProperty<std::string>(object, "targetSpawn", "");
			exit.bounds = PixelRectangleToWorld(room, object);
			room.exits.push_back(exit);
		}
		else if (className == "Checkpoint")
		{
			TiledCheckpointData checkpoint;
			checkpoint.id = id;
			checkpoint.position = PixelPointToWorld(room, object.value("x", 0.0), object.value("y", 0.0));
			room.checkpoints.push_back(checkpoint);
		}
	}
}

void ReadLayers(TiledRoomData& room, const json& layers)
{
	for (const json& layer : layers)
	{
		const std::string layerType = layer.value("type", "");
		if (layerType == "group" && layer.contains("layers"))
		{
			ReadLayers(room, layer["layers"]);
		}
		else if (layerType == "tilelayer" && layer.value("name", "") == "CollisionSolid")
		{
			ReadTileData(room, layer);
		}
		else if (layerType == "objectgroup")
		{
			ReadObjects(room, layer);
		}
	}
}

} // namespace

bool TiledRoomData::IsSolid(uint32_t x, uint32_t y) const
{
	if (x >= width || y >= height)
	{
		return false;
	}
	return solidTiles[static_cast<size_t>(y) * width + x] != 0;
}

const TiledSpawnData* TiledRoomData::FindSpawn(const std::string& spawnId) const
{
	const auto iterator = std::find_if(spawns.begin(), spawns.end(), [&spawnId](const TiledSpawnData& spawn) {
		return spawn.className == "PlayerSpawn" && spawn.id == spawnId;
	});
	return iterator == spawns.end() ? nullptr : &*iterator;
}

TiledRoomData TiledRoomLoader::Load(const std::string& filePath)
{
	std::ifstream file(filePath);
	if (!file.is_open())
	{
		throw std::runtime_error("Tiled room could not be opened: " + filePath);
	}

	json map;
	file >> map;

	if (map.value("orientation", "") != "orthogonal")
	{
		throw std::runtime_error("Only orthogonal Tiled maps are supported: " + filePath);
	}

	TiledRoomData room;
	room.roomId = GetProperty<std::string>(map, "roomId", filePath);
	room.width = map.at("width").get<uint32_t>();
	room.height = map.at("height").get<uint32_t>();
	room.tileWidth = map.at("tilewidth").get<uint32_t>();
	room.tileHeight = map.at("tileheight").get<uint32_t>();

	if (room.width == 0 || room.height == 0 || room.tileWidth == 0 || room.tileHeight == 0)
	{
		throw std::runtime_error("Tiled room has an invalid size: " + filePath);
	}

	room.solidTiles.assign(static_cast<size_t>(room.width) * room.height, 0u);
	ReadLayers(room, map.at("layers"));
	return room;
}

#include "TiledRoom.h"

#include "ThirdParty/nlohmann/json.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <unordered_map>

using json = nlohmann::json;
using namespace KamataEngine;

namespace
{

constexpr uint32_t kTiledFlipFlags = 0xE0000000u;
using GimmickTileTypes = std::unordered_map<uint32_t, GimmickType>;

struct SwitchLinkRegion
{
	std::string name;
	std::string linkId;
	AABB bounds{};
};

bool IsLinkedGimmick(GimmickType type)
{
	return type == GimmickType::kOreSwitch ||
		type == GimmickType::kAirRushSwitch ||
		type == GimmickType::kStoneBridge;
}

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

GimmickTileTypes ReadGimmickTileTypes(const json& tilesets, const std::filesystem::path& mapPath)
{
	GimmickTileTypes types;
	for (const json& reference : tilesets)
	{
		const uint32_t firstGid = reference.at("firstgid").get<uint32_t>();
		json tileset = reference;
		if (reference.contains("source"))
		{
			const std::filesystem::path tilesetPath =
				(mapPath.parent_path() / reference.at("source").get<std::string>()).lexically_normal();
			std::ifstream file(tilesetPath);
			if (!file.is_open())
			{
				throw std::runtime_error("Tiled tileset could not be opened: " + tilesetPath.string());
			}
			file >> tileset;
		}
		if (!tileset.contains("tiles") || !tileset["tiles"].is_array())
		{
			continue;
		}
		for (const json& tile : tileset["tiles"])
		{
			const std::string name = GetProperty<std::string>(tile, "gimmickType", "");
			if (name.empty())
			{
				continue;
			}
			const std::optional<GimmickType> type = ParseGimmickType(name);
			if (!type)
			{
				throw std::runtime_error("Unknown gimmickType in Tiled tileset: " + name);
			}
			types.emplace(firstGid + tile.at("id").get<uint32_t>(), *type);
		}
	}
	return types;
}

void AddGimmickTile(TiledRoomData& room, const GimmickTileTypes& types,
	int32_t x, int32_t y, uint32_t rawGid)
{
	if (x < 0 || y < 0 || x >= static_cast<int32_t>(room.width) ||
		y >= static_cast<int32_t>(room.height))
	{
		return;
	}
	const uint32_t gid = rawGid & ~kTiledFlipFlags;
	if (gid == 0)
	{
		return;
	}
	const auto it = types.find(gid);
	if (it == types.end())
	{
		throw std::runtime_error("Gimmicks layer contains a tile without gimmickType (gid " +
			std::to_string(gid) + ")");
	}
	TiledGimmickData gimmick;
	gimmick.id = "tile_" + std::to_string(x) + "_" + std::to_string(y);
	gimmick.type = it->second;
	gimmick.position = {static_cast<float>(x),
		static_cast<float>(room.height - 1 - static_cast<uint32_t>(y)), 0.0f};
	room.gimmicks.push_back(gimmick);
}

void ReadGimmickTileData(TiledRoomData& room, const json& layer, const GimmickTileTypes& types)
{
	if (layer.contains("data") && layer["data"].is_array())
	{
		const json& data = layer["data"];
		const size_t count = (std::min)(data.size(), static_cast<size_t>(room.width) * room.height);
		for (size_t index = 0; index < count; ++index)
		{
			AddGimmickTile(room, types, static_cast<int32_t>(index % room.width),
				static_cast<int32_t>(index / room.width), data[index].get<uint32_t>());
		}
		return;
	}
	if (layer.contains("chunks") && layer["chunks"].is_array())
	{
		for (const json& chunk : layer["chunks"])
		{
			const int32_t startX = chunk.value("x", 0);
			const int32_t startY = chunk.value("y", 0);
			const int32_t width = chunk.at("width").get<int32_t>();
			const int32_t height = chunk.at("height").get<int32_t>();
			const json& data = chunk.at("data");
			for (int32_t y = 0; y < height; ++y)
			{
				for (int32_t x = 0; x < width; ++x)
				{
					AddGimmickTile(room, types, startX + x, startY + y,
						data.at(static_cast<size_t>(y) * width + x).get<uint32_t>());
				}
			}
		}
		return;
	}
	throw std::runtime_error("Gimmicks tile layer requires array or chunk data");
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
		else if (className == "Goal")
		{
			if (room.goal)
			{
				throw std::runtime_error("Tiled room can contain only one Goal");
			}
			if (object.value("point", false) || object.value("width", 0.0) <= 0.0 ||
				object.value("height", 0.0) <= 0.0 || object.value("rotation", 0.0) != 0.0)
			{
				throw std::runtime_error("Goal '" + id + "' needs a non-rotated rectangle");
			}
			room.goal = TiledGoalData{id, PixelRectangleToWorld(room, object)};
		}
		else if (className == "GimmickSpawn")
		{
			const std::string typeName = GetProperty<std::string>(object, "gimmickType", "");
			const std::optional<GimmickType> type = ParseGimmickType(typeName);
			if (!object.value("point", false) || !type)
			{
				throw std::runtime_error("GimmickSpawn '" + id +
					"' requires a Point Object and valid gimmickType (got '" + typeName + "')");
			}
			TiledGimmickData gimmick;
			gimmick.id = id;
			gimmick.type = *type;
			gimmick.position = PixelPointToWorld(room, object.value("x", 0.0), object.value("y", 0.0));
			gimmick.linkId = GetProperty<std::string>(object, "linkId", "");
			room.gimmicks.push_back(gimmick);
		}
		else if (className == "TriangleTrace")
		{
			if (id.empty() || !object.contains("polygon") || !object["polygon"].is_array() ||
				object["polygon"].size() != 3)
			{
				throw std::runtime_error("TriangleTrace requires a name and exactly three polygon points");
			}

			TiledTriangleData triangle;
			triangle.id = id;
			for (size_t i = 0; i < triangle.vertices.size(); ++i)
			{
				const json& point = object["polygon"][i];
				triangle.vertices[i] = PixelPointToWorld(room,
					object.value("x", 0.0) + point.at("x").get<double>(),
					object.value("y", 0.0) + point.at("y").get<double>());
			}
			room.triangles.push_back(triangle);
		}
	}
}

void ReadSwitchLinks(const TiledRoomData& room, const json& layer,
	std::vector<SwitchLinkRegion>& regions)
{
	if (!layer.contains("objects") || !layer["objects"].is_array())
	{
		throw std::runtime_error("SwitchLinks requires rectangle objects");
	}
	for (const json& object : layer["objects"])
	{
		const std::string name = object.value("name", "");
		const std::string linkId = GetProperty<std::string>(object, "linkId", "");
		if (linkId.empty() || object.value("point", false) ||
			object.value("width", 0.0) <= 0.0 || object.value("height", 0.0) <= 0.0 ||
			object.value("rotation", 0.0) != 0.0)
		{
			throw std::runtime_error("SwitchLinks object '" + name +
				"' needs a non-rotated rectangle and a linkId string property");
		}
		regions.push_back({name, linkId, PixelRectangleToWorld(room, object)});
	}
}

void AssignSwitchLinks(TiledRoomData& room, const std::vector<SwitchLinkRegion>& regions)
{
	std::vector<size_t> matched(regions.size(), 0);
	std::unordered_map<std::string, std::pair<bool, bool>> groups;
	for (TiledGimmickData& gimmick : room.gimmicks)
	{
		if (!IsLinkedGimmick(gimmick.type))
		{
			continue;
		}
		for (size_t i = 0; i < regions.size(); ++i)
		{
			const AABB& bounds = regions[i].bounds;
			const Vector3& point = gimmick.position;
			if (point.x < bounds.min.x || point.x > bounds.max.x ||
				point.y < bounds.min.y || point.y > bounds.max.y)
			{
				continue;
			}
			if (!gimmick.linkId.empty() && gimmick.linkId != regions[i].linkId)
			{
				throw std::runtime_error("Gimmick '" + gimmick.id +
					"' is covered by different SwitchLinks IDs");
			}
			gimmick.linkId = regions[i].linkId;
			++matched[i];
		}
		if (gimmick.linkId.empty())
		{
			throw std::runtime_error("Switch or StoneBridge '" + gimmick.id +
				"' needs a SwitchLinks rectangle with linkId");
		}
		auto& group = groups[gimmick.linkId];
		if (gimmick.type == GimmickType::kStoneBridge)
		{
			group.second = true;
		}
		else
		{
			group.first = true;
		}
	}
	for (size_t i = 0; i < regions.size(); ++i)
	{
		if (matched[i] == 0)
		{
			throw std::runtime_error("SwitchLinks rectangle '" + regions[i].name +
				"' does not cover a switch or StoneBridge tile");
		}
	}
	for (const auto& [linkId, group] : groups)
	{
		if (!group.first || !group.second)
		{
			throw std::runtime_error("SwitchLinks linkId '" + linkId +
				"' needs both a switch and a StoneBridge");
		}
	}
}

void ReadLayers(TiledRoomData& room, const json& layers, const GimmickTileTypes& gimmickTileTypes,
	std::vector<SwitchLinkRegion>& regions)
{
	for (const json& layer : layers)
	{
		const std::string layerType = layer.value("type", "");
		if (layerType == "group" && layer.contains("layers"))
		{
			ReadLayers(room, layer["layers"], gimmickTileTypes, regions);
		}
		else if (layerType == "tilelayer" && layer.value("name", "") == "CollisionSolid")
		{
			ReadTileData(room, layer);
		}
		else if (layerType == "tilelayer" && layer.value("name", "") == "Gimmicks")
		{
			ReadGimmickTileData(room, layer, gimmickTileTypes);
		}
		else if (layerType == "objectgroup" && layer.value("name", "") == "SwitchLinks")
		{
			ReadSwitchLinks(room, layer, regions);
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
	const GimmickTileTypes gimmickTileTypes = ReadGimmickTileTypes(map.at("tilesets"), filePath);
	std::vector<SwitchLinkRegion> switchLinks;
	ReadLayers(room, map.at("layers"), gimmickTileTypes, switchLinks);
	AssignSwitchLinks(room, switchLinks);
	return room;
}

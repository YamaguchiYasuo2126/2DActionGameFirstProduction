#include "TriangleTraceGimmick.h"

#include "MyMathUtility.h"
#include "Player.h"

#include <algorithm>
#include <cmath>

using namespace KamataEngine;

namespace
{
constexpr float kStartRadius = 0.8f;
constexpr float kSnapRadius = 1.1f;
constexpr float kLineTolerance = 0.7f;
constexpr float kVertexRadius = 0.55f;
constexpr float kMinDirectionDot = 0.96f;
constexpr float kBounceVelocity = 0.38f;

float Dot2D(const Vector3& a, const Vector3& b) { return a.x * b.x + a.y * b.y; }
float Length2D(const Vector3& a) { return std::sqrt(Dot2D(a, a)); }
float Distance2D(const Vector3& a, const Vector3& b)
{
	return std::hypot(a.x - b.x, a.y - b.y);
}
float DistanceToSegment2D(const Vector3& point, const Vector3& start, const Vector3& end)
{
	const Vector3 segment{end.x - start.x, end.y - start.y, 0.0f};
	const float lengthSquared = Dot2D(segment, segment);
	if (lengthSquared < 0.0001f)
	{
		return Distance2D(point, start);
	}
	const Vector3 fromStart{point.x - start.x, point.y - start.y, 0.0f};
	const float t = std::clamp(Dot2D(fromStart, segment) / lengthSquared, 0.0f, 1.0f);
	return std::hypot(point.x - (start.x + segment.x * t), point.y - (start.y + segment.y * t));
}
} // namespace

bool TriangleTraceGimmick::Initialize(Model* edgeModel, Model* fillModel, uint32_t whiteTexture,
	Camera* camera, const TiledTriangleData& data, WorldProgress* progress, const std::string& progressKey)
{
	vertices_ = data.vertices;
	edgeModel_ = edgeModel;
	fillModel_ = fillModel;
	whiteTexture_ = whiteTexture;
	camera_ = camera;
	progress_ = progress;
	progressKey_ = progressKey;
	completed_ = progress_ && progress_->HasFlag(progressKey_);
	if (!edgeModel_ || !fillModel_ || !camera_)
	{
		return false;
	}

	// 最初のギミックは水平な底辺を持つ二等辺三角形に限定する。
	apexIndex_ = 0;
	for (int i = 1; i < 3; ++i)
	{
		if (vertices_[i].y > vertices_[apexIndex_].y)
		{
			apexIndex_ = i;
		}
	}
	leftBaseIndex_ = (apexIndex_ + 1) % 3;
	rightBaseIndex_ = (apexIndex_ + 2) % 3;
	if (vertices_[leftBaseIndex_].x > vertices_[rightBaseIndex_].x)
	{
		std::swap(leftBaseIndex_, rightBaseIndex_);
	}
	const Vector3& left = vertices_[leftBaseIndex_];
	const Vector3& right = vertices_[rightBaseIndex_];
	const Vector3& apex = vertices_[apexIndex_];
	const float width = right.x - left.x;
	const float height = apex.y - left.y;
	if (width < 1.0f || height < 0.5f || std::abs(left.y - right.y) > 0.1f ||
		std::abs(width * 0.5f - height) > 0.1f ||
		std::abs(apex.x - (left.x + right.x) * 0.5f) > 0.1f)
	{
		return false;
	}

	for (int i = 0; i < 3; ++i)
	{
		const Vector3& start = vertices_[i];
		const Vector3& end = vertices_[(i + 1) % 3];
		const float length = Distance2D(start, end);
		edgeTransforms_[i].Initialize();
		edgeTransforms_[i].translation_ = {(start.x + end.x) * 0.5f, (start.y + end.y) * 0.5f, -0.22f};
		edgeTransforms_[i].rotation_.z = std::atan2(end.y - start.y, end.x - start.x);
		// movingPlatform.objの一辺（約0.983）に合わせ、従来の表示サイズを保つ。
		constexpr float kModelSpan = 0.982598f;
		edgeTransforms_[i].scale_ = {length / kModelSpan, 0.11f / kModelSpan, 0.09f / kModelSpan};
		edgeColors_[i].Initialize();
	}

	fillTransform_.Initialize();
	fillTransform_.translation_ = {(left.x + right.x) * 0.5f, (left.y + apex.y) * 0.5f, -0.08f};
	fillTransform_.scale_ = {width, height, 1.0f};
	fillColor_.Initialize();
	fillColor_.SetColor({0.95f, 0.55f, 0.12f, 0.75f});
	return true;
}

bool TriangleTraceGimmick::FindSnapCandidate(const Vector3& playerPosition, SnapCandidate& candidate) const
{
	if (completed_)
	{
		return false;
	}

	bool found = false;
	float nearestDistanceSquared = kSnapRadius * kSnapRadius;
	for (int vertexIndex = 0; vertexIndex < 3; ++vertexIndex)
	{
		const Vector3& vertex = vertices_[vertexIndex];
		const float dx = playerPosition.x - vertex.x;
		const float dy = playerPosition.y - vertex.y;
		const float distanceSquared = dx * dx + dy * dy;
		if (distanceSquared > nearestDistanceSquared)
		{
			continue;
		}

		SnapCandidate next{};
		next.vertex = vertex;
		next.distanceSquared = distanceSquared;
		for (int edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
		{
			if (traced_[edgeIndex] || (edgeIndex != vertexIndex && (edgeIndex + 1) % 3 != vertexIndex))
			{
				continue;
			}
			const int oppositeIndex = edgeIndex == vertexIndex ? (edgeIndex + 1) % 3 : edgeIndex;
			const Vector3& opposite = vertices_[oppositeIndex];
			const Vector3 edge{opposite.x - vertex.x, opposite.y - vertex.y, 0.0f};
			const float length = Length2D(edge);
			if (length > 0.01f)
			{
				next.directions[next.directionCount++] = {edge.x / length, edge.y / length, 0.0f};
			}
		}
		if (next.directionCount > 0)
		{
			candidate = next;
			nearestDistanceSquared = distanceSquared;
			found = true;
		}
	}
	return found;
}

void TriangleTraceGimmick::ObserveRush(Player* player)
{
	completedThisFrame_ = false;
	if (completed_ || !player || !player->IsJumpRushing())
	{
		activeEdge_ = -1;
		return;
	}

	if (observedRushSequence_ != player->GetRushSequence())
	{
		observedRushSequence_ = player->GetRushSequence();
		activeEdge_ = -1;
		const Vector3& rushStart = player->GetRushStartPosition();
		const Vector3& rushDirection = player->GetRushDirection();
		const float rushDirectionLength = Length2D(rushDirection);
		if (rushDirectionLength < 0.01f)
		{
			return;
		}
		for (int i = 0; i < 3 && activeEdge_ < 0; ++i)
		{
			if (traced_[i])
			{
				continue;
			}
			for (int orientation = 0; orientation < 2; ++orientation)
			{
				const int startIndex = orientation == 0 ? i : (i + 1) % 3;
				const int endIndex = orientation == 0 ? (i + 1) % 3 : i;
				const Vector3& start = vertices_[startIndex];
				const Vector3& end = vertices_[endIndex];
				const Vector3 edge{end.x - start.x, end.y - start.y, 0.0f};
				const float length = Length2D(edge);
				if (length > 0.01f && Distance2D(rushStart, start) <= kStartRadius &&
					Dot2D(rushDirection, edge) / (rushDirectionLength * length) >= kMinDirectionDot)
				{
					activeEdge_ = i;
					edgeEndVertex_ = endIndex;
					break;
				}
			}
		}
	}

	if (activeEdge_ < 0)
	{
		return;
	}

	const int startIndex = edgeEndVertex_ == activeEdge_ ? (activeEdge_ + 1) % 3 : activeEdge_;
	const Vector3& start = vertices_[startIndex];
	const Vector3& end = vertices_[edgeEndVertex_];
	const Vector3 current = player->GetWorldPosition();
	const Vector3 previous = player->GetPreviousPosition();
	const Vector3 edge{end.x - start.x, end.y - start.y, 0.0f};
	const float length = Length2D(edge);
	const float cross = std::abs((current.x - start.x) * edge.y - (current.y - start.y) * edge.x) / length;
	if (cross > kLineTolerance)
	{
		activeEdge_ = -1;
		return;
	}

	const Vector3 travel{current.x - start.x, current.y - start.y, 0.0f};
	if (Dot2D(travel, edge) / length >= length - kVertexRadius &&
		DistanceToSegment2D(end, previous, current) <= kVertexRadius)
	{
		traced_[activeEdge_] = true;
		activeEdge_ = -1;
		completed_ = traced_[0] && traced_[1] && traced_[2];
		SnapCandidate next{};
		if (!completed_ && FindSnapCandidate(end, next))
		{
			player->FinishRushAtVertex(end, next.directions, next.directionCount);
		}
		else
		{
			player->FinishRushAtVertex(end, {}, 0);
		}
		if (completed_)
		{
			completedThisFrame_ = true;
			if (progress_)
			{
				progress_->SetFlag(progressKey_);
			}
		}
	}
	else if (player->IsJumpRushEnding())
	{
		activeEdge_ = -1;
	}
}

void TriangleTraceGimmick::Update() {}

void TriangleTraceGimmick::Draw()
{
	if (completed_)
	{
		fillTransform_.matWorld_ = MyMathUtility::MakeAffineMatrix(
			fillTransform_.scale_, fillTransform_.rotation_, fillTransform_.translation_);
		fillTransform_.TransferMatrix();
		fillModel_->Draw(fillTransform_, *camera_, whiteTexture_, &fillColor_);
	}

	for (int i = 0; i < 3; ++i)
	{
		const Vector4 color = completed_ ? Vector4{1.0f, 0.83f, 0.2f, 1.0f}
			: traced_[i] ? Vector4{0.2f, 1.0f, 0.9f, 1.0f}
			: Vector4{0.55f, 0.85f, 1.0f, 0.45f};
		edgeColors_[i].SetColor(color);
		edgeTransforms_[i].matWorld_ = MyMathUtility::MakeAffineMatrix(
			edgeTransforms_[i].scale_, edgeTransforms_[i].rotation_, edgeTransforms_[i].translation_);
		edgeTransforms_[i].TransferMatrix();
		edgeModel_->Draw(edgeTransforms_[i], *camera_, whiteTexture_, &edgeColors_[i]);
	}
}

AABB TriangleTraceGimmick::GetAABB() const
{
	AABB bounds{{vertices_[0].x, vertices_[0].y, -0.5f}, {vertices_[0].x, vertices_[0].y, 0.5f}};
	for (const Vector3& vertex : vertices_)
	{
		bounds.min.x = (std::min)(bounds.min.x, vertex.x);
		bounds.min.y = (std::min)(bounds.min.y, vertex.y);
		bounds.max.x = (std::max)(bounds.max.x, vertex.x);
		bounds.max.y = (std::max)(bounds.max.y, vertex.y);
	}
	return bounds;
}

void TriangleTraceGimmick::OnPlayerCollision(Player* player)
{
	if (!completed_ || completedThisFrame_ || !player || player->GetVelocity().y >= -0.01f)
	{
		return;
	}

	const Vector3& left = vertices_[leftBaseIndex_];
	const Vector3& right = vertices_[rightBaseIndex_];
	const Vector3& apex = vertices_[apexIndex_];
	const float x = player->GetWorldPosition().x;
	if (x <= left.x + 0.1f || x >= right.x - 0.1f)
	{
		return;
	}

	const float slope = x <= apex.x ? (x - left.x) / (apex.x - left.x)
		: (right.x - x) / (right.x - apex.x);
	const float surfaceY = left.y + (apex.y - left.y) * slope;
	const float previousFootY = player->GetPreviousAABB().min.y;
	const float currentFootY = player->GetAABB().min.y;
	if (previousFootY >= surfaceY - 0.08f && currentFootY <= surfaceY + 0.08f)
	{
		player->BounceOnGimmick(surfaceY, kBounceVelocity);
	}
}

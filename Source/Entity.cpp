#include "stdafx.h"
#include "Entity.h"
#include "Node.h"
#include "ToolKit.h"
#include "Skeleton.h"
#include "MathUtil.h"
#include "DebugNew.h"

namespace ToolKit
{

	EntityId Entity::m_lastId = 1; // 0 is null entity id.

	Entity::Entity()
	{
		m_node = new Node();
		m_id = m_lastId++;
	}

	Entity::~Entity()
	{
		SafeDel(m_node);
	}

	bool Entity::IsDrawable() const
	{
		return false;
	}

	EntityType Entity::GetType() const
	{
		return EntityType::Entity_Base;
	}

	void Entity::SetPose(Animation* anim)
	{
		anim->GetCurrentPose(m_node);
	}

	struct BoundingBox Entity::GetAABB(bool inWorld) const
	{
		BoundingBox aabb;

		// Unit aabb.
		aabb.min = Vec3(0.5f, 0.5f, 0.5f);
		aabb.max = Vec3(-0.5f, -0.5f, -0.5f);

		if (inWorld)
		{
			TransformAABB(aabb, m_node->GetTransform(TransformationSpace::TS_WORLD));
		}

		return aabb;
	}

}

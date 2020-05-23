#pragma once

#include "ToolKit.h"

namespace ToolKit
{
	namespace Editor
	{
		class Scene
		{
		public:
			~Scene();

			// Scene queries.
			struct PickData
			{
				Vec3 pickPos;
				Entity* entity = nullptr;
			};

			PickData PickObject(Ray ray, const EntityIdArray& ignoreList = EntityIdArray());
			void PickObject(const Frustum& frustum, std::vector<PickData>& pickedObjects, const EntityIdArray& ignoreList = EntityIdArray(), bool pickPartiallyInside = true);

			// Selection operations.
			bool IsSelected(EntityId id);
			void RemoveFromSelection(EntityId id);
			void AddToSelection(const EntityIdArray& entities, bool additive);
			void AddToSelection(const EntityRawPtrArray& entities, bool additive);
			void AddToSelection(EntityId id);
			void ClearSelection();
			bool IsCurrentSelection(EntityId id);
			void MakeCurrentSelection(EntityId id, bool ifExist);
			uint GetSelectedEntityCount();
			Entity* GetCurrentSelection();

			// Entity operations.
			Entity* GetEntity(EntityId id);
			void AddEntity(Entity* entity);
			Entity* RemoveEntity(EntityId id);
			const EntityRawPtrArray& GetEntities();
			void GetSelectedEntities(EntityRawPtrArray& entities);

		private:
			EntityRawPtrArray m_entitites;
			EntityIdArray m_selectedEntities;
		};
	}
}
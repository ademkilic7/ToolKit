#pragma once

#include "ToolKit.h"
#include "Scene.h"

namespace ToolKit
{
  namespace Editor
  {
    class EditorScene : public Scene
    {
    public:
      EditorScene();
      virtual ~EditorScene();

      // Selection operations.
      bool IsSelected(EntityId id) const;
      void RemoveFromSelection(EntityId id);
      void AddToSelection(const EntityIdArray& entities, bool additive);
      void AddToSelection(const EntityRawPtrArray& entities, bool additive);
      void AddToSelection(EntityId id, bool additive);
      void ClearSelection();
      bool IsCurrentSelection(EntityId id) const;
      void MakeCurrentSelection(EntityId id, bool ifExist); // Makes the entity current selection. ifExist true, only works if the entity exist in the selection. Otherwise adds entity to selection list and selects it.
      uint GetSelectedEntityCount() const;
      Entity* GetCurrentSelection() const;

      // Entity operations.
      virtual Entity* RemoveEntity(EntityId id) override;
      virtual void Destroy() override;
      void GetSelectedEntities(EntityRawPtrArray& entities) const;
      void GetSelectedEntities(EntityIdArray& entities) const;
      void SelectByTag(const String& tag);

    public:
      bool m_newScene; // Indicates if this is created via new scene. That is not saved on the disk.

    private:
      EntityIdArray m_selectedEntities;
    };
  }
}
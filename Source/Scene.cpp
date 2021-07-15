#include "stdafx.h"
#include "Scene.h"
#include "Util.h"
#include "DebugNew.h"

namespace ToolKit
{

  Scene::Scene()
  {
    m_name = "NewScene";
  }

  Scene::~Scene()
  {
    Destroy();
  }

  Scene::PickData Scene::PickObject(Ray ray, const EntityIdArray& ignoreList) const
  {
    PickData pd;
    pd.pickPos = ray.position + ray.direction * 5.0f;

    float closestPickedDistance = FLT_MAX;
    for (Entity* e : m_entitites)
    {
      if (!e->IsDrawable())
      {
        continue;
      }

      if (std::find(ignoreList.begin(), ignoreList.end(), e->m_id) != ignoreList.end())
      {
        continue;
      }

      Ray rayInObjectSpace = ray;
      Mat4 ts = e->m_node->GetTransform(TransformationSpace::TS_WORLD);
      Mat4 its = glm::inverse(ts);
      rayInObjectSpace.position = its * Vec4(ray.position, 1.0f);
      rayInObjectSpace.direction = its * Vec4(ray.direction, 0.0f);

      float dist = 0;
      Drawable* dw = static_cast<Drawable*>(e);
      if (RayBoxIntersection(rayInObjectSpace, dw->GetAABB(), dist))
      {
        bool hit = true;
        if (dw->m_mesh->m_clientSideVertices.size() == dw->m_mesh->m_vertexCount)
        {
          // Per polygon check if data exist.
          float meshDist = 0.0f;
          hit = RayMeshIntersection(dw->m_mesh.get(), rayInObjectSpace, meshDist);
          if (hit)
          {
            dist = meshDist;
          }
        }
        if (hit)
        {
          if (dist < closestPickedDistance && dist > 0.0f)
          {
            pd.entity = e;
            pd.pickPos = ray.position + ray.direction * dist;
            closestPickedDistance = dist;
          }
        }
      }
    }

    return pd;
  }

  void Scene::PickObject(const Frustum& frustum, std::vector<PickData>& pickedObjects, const EntityIdArray& ignoreList, bool pickPartiallyInside) const
  {
    for (Entity* e : m_entitites)
    {
      if (!e->IsDrawable())
      {
        continue;
      }

      if (std::find(ignoreList.begin(), ignoreList.end(), e->m_id) != ignoreList.end())
      {
        continue;
      }

      BoundingBox bb = e->GetAABB(true);
      IntersectResult res = FrustumBoxIntersection(frustum, bb);
      if (res != IntersectResult::Outside)
      {
        PickData pd;
        pd.pickPos = (bb.max + bb.min) * 0.5f;
        pd.entity = e;

        if (res == IntersectResult::Inside)
        {
          pickedObjects.push_back(pd);
        }
        else if (pickPartiallyInside)
        {
          pickedObjects.push_back(pd);
        }
      }
    }
  }

  Entity* Scene::GetEntity(EntityId id) const
  {
    for (Entity* e : m_entitites)
    {
      if (e->m_id == id)
      {
        return e;
      }
    }

    return nullptr;
  }

  void Scene::AddEntity(Entity* entity)
  {
    assert(GetEntity(entity->m_id) == nullptr && "Entity is already in the scene.");
    m_entitites.push_back(entity);
  }

  Entity* Scene::RemoveEntity(EntityId id)
  {
    Entity* removed = nullptr;
    for (int i = (int)m_entitites.size() - 1; i >= 0; i--)
    {
      if (m_entitites[i]->m_id == id)
      {
        removed = m_entitites[i];
        m_entitites.erase(m_entitites.begin() + i);
        break;
      }
    }

    return removed;
  }

  const EntityRawPtrArray& Scene::GetEntities() const
  {
    return m_entitites;
  }

  EntityRawPtrArray Scene::GetByTag(const String& tag)
  {
    EntityRawPtrArray arrayByTag;
    for (Entity* e : m_entitites)
    {
      if (e->m_tag == tag)
      {
        arrayByTag.push_back(e);
      }
    }

    return arrayByTag;
  }

  void Scene::Destroy()
  {
    for (Entity* ntt : m_entitites)
    {
      SafeDel(ntt);
    }
    m_entitites.clear();
  }

  void Scene::Serialize(XmlDocument* doc, XmlNode* parent) const
  {
    std::ofstream file;
    String fileName = ScenePath(m_name + ".scene");

    file.open(fileName.c_str(), std::ios::out);
    if (file.is_open())
    {
      XmlNode* scene = doc->allocate_node(rapidxml::node_element, XmlSceneElement.c_str());
      WriteAttr(scene, doc, "name", m_name.c_str());

      if (parent != nullptr)
      {
        parent->append_node(scene);
      }
      else
      {
        doc->append_node(scene);
      }

      for (Entity* ntt : m_entitites)
      {
        ntt->Serialize(doc, scene);
      }

      std::string xml;
      rapidxml::print(std::back_inserter(xml), *doc, 0);

      file << xml;
      file.close();
      doc->clear();
    }
  }

  void Scene::DeSerialize(XmlDocument* doc, XmlNode* parent)
  {
    XmlNode* root = nullptr;
    if (parent != nullptr)
    {
      root = parent->first_node(XmlSceneElement.c_str());
    }
    else
    {
      root = doc->first_node(XmlSceneElement.c_str());
    }

    ReadAttr(root, "name", m_name);

    XmlNode* node = nullptr;
    for (node = root->first_node(XmlEntityElement.c_str()); node; node = node->next_sibling(XmlEntityElement.c_str()))
    {
      XmlAttribute* typeAttr = node->first_attribute(XmlEntityTypeAttr.c_str());
      EntityType et = (EntityType)std::atoi(typeAttr->value());
      Entity* ntt = nullptr;
      switch (et)
      {
      case EntityType::Entity_Base:
        ntt = new Entity(); // Empty entities are used for transform hierarchy.
        break;
      case EntityType::Entity_AudioSource:
        continue;
      case EntityType::Entity_Billboard:
        break;
      case EntityType::Entity_Cube:
        ntt = new Cube(false);
        break;
      case EntityType::Entity_Quad:
        ntt = new Quad(false);
        break;
      case EntityType::Entity_Sphere:
        ntt = new Sphere(false);
        break;
      case EntityType::Etity_Arrow:
        ntt = new Arrow2d(false);
        break;
      case EntityType::Entity_LineBatch:
        break;
      case EntityType::Entity_Cone:
        ntt = new Cone(false);
        break;
      case EntityType::Entity_Drawable:
        ntt = new Drawable();
        break;
      case EntityType::Entity_SpriteAnim:
      case EntityType::Entity_Surface:
        continue;
      case EntityType::Entity_Light:
        break;
      case EntityType::Entity_Camera:
        break;
      case EntityType::Entity_Directional:
        continue;
      default:
        assert(false);
        continue;
      }

      ntt->DeSerialize(doc, node);
      m_entitites.push_back(ntt);
    }

    // Update parent - child relation for entities.
    for (Entity* e : m_entitites)
    {
      if (e->_parentId != 0)
      {
        Entity* parent = GetEntity(e->_parentId);
        if (parent)
        {
          parent->m_node->AddChild(e->m_node);
        }
      }
    }
  }

}
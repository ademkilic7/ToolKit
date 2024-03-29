#include "stdafx.h"
#include "ToolKit.h"
#include "Scene.h"
#include "Util.h"
#include "DebugNew.h"

namespace ToolKit
{

  Scene::Scene()
  {
    m_name = "New Scene";
    m_type = ResourceType::Scene;
  }

  Scene::Scene(String file)
    : Scene()
  {
    m_file = file;
  }

  Scene::~Scene()
  {
    Destroy(false);
  }

  void Scene::Load()
  {
    if (m_loaded)
    {
      return;
    }

    XmlFile sceneFile(m_file.c_str());
    XmlDocument sceneDoc;
    sceneDoc.parse<0>(sceneFile.data());

    DeSerialize(&sceneDoc, nullptr);

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

    m_loaded = true;
  }

  void Scene::Save(bool onlyIfDirty)
  {
    String fullPath = m_file;
    if (m_file.empty())
    {
      fullPath = ScenePath(m_name + SCENE);
    }

    std::ofstream file;
    file.open(fullPath.c_str(), std::ios::out);
    if (file.is_open())
    {
      XmlDocument doc;
      Serialize(&doc, nullptr);
      
      std::string xml;
      rapidxml::print(std::back_inserter(xml), doc, 0);

      file << xml;
      file.close();
      doc.clear();
    }
  }

  void Scene::Init(bool flushClientSideArray)
  {
    if (m_initiated)
    {
      return;
    }

    const EntityRawPtrArray& ntties = GetEntities();
    for (Entity* ntt : ntties)
    {
      if (ntt->IsDrawable())
      {
        static_cast<Drawable*> (ntt)->m_mesh->Init(flushClientSideArray);
      }
    }

    m_initiated = true;
  }

  void Scene::UnInit()
  {
    Destroy(false);
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

  EntityRawPtrArray Scene::Filter(std::function<bool(Entity*)> filter)
  {
    EntityRawPtrArray filtered;
    std::copy_if(m_entitites.begin(), m_entitites.end(), std::back_inserter(filtered), filter);
    return filtered;
  }

  void Scene::Destroy(bool removeResources)
  {
    for (Entity* ntt : m_entitites)
    {
      if (removeResources)
      {
        ntt->RemoveResources();
      }

      SafeDel(ntt);
    }
    m_entitites.clear();

    m_loaded = false;
    m_initiated = false;
  }

  void Scene::CopyTo(Resource* other)
  {
    Resource::CopyTo(other);
    Scene* cpy = static_cast<Scene*> (other);
    cpy->m_name = m_name + "_cpy";

    cpy->m_entitites.reserve(m_entitites.size());
    for (Entity* ntt : m_entitites)
    {
      Entity* cpyNtt = ntt->GetCopy();
      cpy->m_entitites.push_back(cpyNtt);
    }
  }

  void Scene::Serialize(XmlDocument* doc, XmlNode* parent) const
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
      EntityType t = (EntityType)std::atoi(typeAttr->value());
      Entity* ntt = Entity::CreateByType(t);

      ntt->DeSerialize(doc, node);
      m_entitites.push_back(ntt);
    }
  }

  SceneManager::SceneManager()
  {
    m_type = ResourceType::Scene;
  }

  SceneManager::~SceneManager()
  {
  }

  void SceneManager::Init()
  {
    m_currentScene = nullptr;
    ResourceManager::Init();
  }

  void SceneManager::Uninit()
  {
    m_currentScene = nullptr;
    ResourceManager::Uninit();
  }

}

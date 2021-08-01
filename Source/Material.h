#pragma once

#include "RenderState.h"
#include "Resource.h"
#include "ResourceManager.h"

namespace ToolKit
{

  class Material : public Resource
  {
  public:
    Material();
    Material(String file);
    ~Material();

    virtual void Load() override;
    virtual void Save(bool onlyIfDirty) override;
    virtual void Init(bool flushClientSideArray = true) override;
    virtual void UnInit() override;
    virtual Material* GetCopy() override;
    RenderState* GetRenderState();

    virtual void Serialize(XmlDocument* doc, XmlNode* parent) const;
    virtual void DeSerialize(XmlDocument* doc, XmlNode* parent);

  public:
    CubeMapPtr m_cubeMap;
    TexturePtr m_diffuseTexture;
    ShaderPtr m_vertexShader;
    ShaderPtr m_fragmetShader;
    Vec3 m_color;

  private:
    RenderState m_renderState;
  };

  class MaterialManager : public ResourceManager
  {
  public:
    MaterialManager();
    virtual ~MaterialManager();
    virtual void Init() override;

    MaterialPtr GetCopyOfUnlitMaterial();
    MaterialPtr GetCopyOfUnlitColorMaterial();
    MaterialPtr GetCopyOfSolidMaterial();
    MaterialPtr GetCopyOfDefaultMaterial();
  };

}
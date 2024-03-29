#include "stdafx.h"
#include "Surface.h"
#include "Mesh.h"
#include "Texture.h"
#include "Material.h"
#include "Node.h"
#include "DebugNew.h"

namespace ToolKit
{

  Surface::Surface(TexturePtr texture, const Vec2& pivotOffset)
  {
    m_mesh->m_material->m_diffuseTexture = texture;
    m_pivotOffset = pivotOffset;
    CreateQuat();
    AssignTexture();
  }

  Surface::Surface(TexturePtr texture, const SpriteEntry& entry)
  {
    m_mesh->m_material->m_diffuseTexture = texture;
    CreateQuat(entry);
    AssignTexture();
  }

  Surface::Surface(const String& textureFile, const Vec2& pivotOffset)
  {
    m_mesh->m_material->m_diffuseTexture = GetTextureManager()->Create<Texture>(textureFile);
    m_pivotOffset = pivotOffset;
    CreateQuat();
    AssignTexture();
  }

  Surface::~Surface()
  {
  }

  EntityType Surface::GetType() const
  {
    return EntityType::Entity_Surface;
  }

  void Surface::AssignTexture()
  {
    m_mesh->m_material->GetRenderState()->blendFunction = BlendFunction::SRC_ALPHA_ONE_MINUS_SRC_ALPHA;
    m_mesh->m_material->GetRenderState()->depthTestEnabled = false;
  }

  void Surface::CreateQuat()
  {
    float width = (float)m_mesh->m_material->m_diffuseTexture->m_width;
    float height = (float)m_mesh->m_material->m_diffuseTexture->m_height;
    float depth = 0;
    Vec2 absOffset = Vec2(m_pivotOffset.x * width, m_pivotOffset.y * height);

    VertexArray vertices;
    vertices.resize(6);
    vertices[0].pos = Vec3(-absOffset.x, -absOffset.y, depth);
    vertices[0].tex = Vec2(0.0f, 1.0f);
    vertices[1].pos = Vec3(width - absOffset.x, -absOffset.y, depth);
    vertices[1].tex = Vec2(1.0f, 1.0f);
    vertices[2].pos = Vec3(-absOffset.x, height - absOffset.y, depth);
    vertices[2].tex = Vec2(0.0f, 0.0f);

    vertices[3].pos = Vec3(width - absOffset.x, -absOffset.y, depth);
    vertices[3].tex = Vec2(1.0f, 1.0f);
    vertices[4].pos = Vec3(width - absOffset.x, height - absOffset.y, depth);
    vertices[4].tex = Vec2(1.0f, 0.0f);
    vertices[5].pos = Vec3(-absOffset.x, height - absOffset.y, depth);
    vertices[5].tex = Vec2(0.0f, 0.0f);

    m_mesh->m_clientSideVertices = vertices;
  }

  void Surface::CreateQuat(const SpriteEntry& val)
  {
    float imageWidth = (float)m_mesh->m_material->m_diffuseTexture->m_width;
    float imageHeight = (float)m_mesh->m_material->m_diffuseTexture->m_height;

    Rect<float> textureRect;
    textureRect.x = (float)val.rectangle.x / (float)imageWidth;
    textureRect.height = ((float)val.rectangle.height / (float)imageHeight);
    textureRect.y = 1.0f - ((float)val.rectangle.y / (float)imageHeight) - textureRect.height;
    textureRect.width = (float)val.rectangle.width / (float)imageWidth;

    float depth = 0.0f;
    float width = (float)val.rectangle.width;
    float height = (float)val.rectangle.height;
    Vec2 absOffset = Vec2(val.offset.x * val.rectangle.width, val.offset.y * val.rectangle.height);

    VertexArray vertices;
    vertices.resize(6);
    vertices[0].pos = Vec3(-absOffset.x, -absOffset.y, depth);
    vertices[0].tex = Vec2(textureRect.x, 1.0f - textureRect.y);
    vertices[1].pos = Vec3(width - absOffset.x, -absOffset.y, depth);
    vertices[1].tex = Vec2(textureRect.x + textureRect.width, 1.0f - textureRect.y);
    vertices[2].pos = Vec3(-absOffset.x, height - absOffset.y, depth);
    vertices[2].tex = Vec2(textureRect.x, 1.0f - (textureRect.y + textureRect.height));

    vertices[3].pos = Vec3(width - absOffset.x, -absOffset.y, depth);
    vertices[3].tex = Vec2(textureRect.x + textureRect.width, 1.0f - textureRect.y);
    vertices[4].pos = Vec3(width - absOffset.x, height - absOffset.y, depth);
    vertices[4].tex = Vec2(textureRect.x + textureRect.width, 1.0f - (textureRect.y + textureRect.height));
    vertices[5].pos = Vec3(-absOffset.x, height - absOffset.y, depth);
    vertices[5].tex = Vec2(textureRect.x, 1.0f - (textureRect.y + textureRect.height));

    m_mesh->m_clientSideVertices = vertices;
  }

}

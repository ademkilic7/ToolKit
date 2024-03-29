#include "stdafx.h"

#include "ConsoleWindow.h"
#include "FolderWindow.h"
#include "GlobalDef.h"
#include "Gizmo.h"
#include "PropInspector.h"
#include "Util.h"
#include "DebugNew.h"

#include <filesystem>

namespace ToolKit
{
  namespace Editor
  {
    DirectoryEntry::DirectoryEntry()
    {
    }

    DirectoryEntry::DirectoryEntry(const String& fullPath)
    {
      DecomposePath(fullPath, &m_rootPath, &m_fileName, &m_ext);
    }

    String DirectoryEntry::GetFullPath() const
    {
      return ConcatPaths({ m_rootPath, m_fileName + m_ext });
    }

    ResourceManager* DirectoryEntry::GetManager() const
    {
      if (m_ext == ANIM)
      {
        return GetAnimationManager();
      }
      else if (m_ext == AUDIO)
      {
        return GetAudioManager();
      }
      else if (m_ext == MATERIAL)
      {
        return GetMaterialManager();
      }
      else if (m_ext == MESH)
      {
        return GetMeshManager();
      }
      else if (m_ext == SHADER)
      {
        return GetShaderManager();
      }
      else if (SupportedImageFormat(m_ext))
      {
        return GetTextureManager();
      }

      return nullptr;
    }

    void DirectoryEntry::GenerateThumbnail() const
    {
      const Vec2& thumbSize = g_app->m_thumbnailSize;
      auto renderThumbFn = [this, &thumbSize](Camera* cam, Drawable* dw) -> void
      {
        RenderTarget* thumb = nullptr;
        RenderTargetPtr thumbPtr = nullptr;
        String fullPath = GetFullPath();
        
        if (g_app->m_thumbnailCache.find(fullPath) != g_app->m_thumbnailCache.end())
        {
          thumbPtr = g_app->m_thumbnailCache[fullPath];
          if 
          (
            thumbPtr->m_width - (int)thumbSize.x == 0 &&
            thumbPtr->m_height - (int)thumbSize.y == 0
          )
          {
            thumb = thumbPtr.get();
          }
        }

        if (thumb == nullptr)
        {
          thumbPtr = std::make_shared<RenderTarget>((uint)thumbSize.x, (uint)thumbSize.y);
          thumb = thumbPtr.get();
          thumb->Init();
        }
        
        g_app->m_renderer->SwapRenderTarget(&thumb);
        
        Light light;
        light.m_node->SetTranslation({ 5.0f, 5.0f, 5.0f });
        light.LookAt(Vec3());

        LightRawPtrArray lights = { &light };

        g_app->m_renderer->Render(dw, cam, lights);
        g_app->m_renderer->SwapRenderTarget(&thumb, false);
        g_app->m_thumbnailCache[GetFullPath()] = thumbPtr;
      };

      if (m_ext == MESH)
      {
        Drawable dw;
        String fullpath = ConcatPaths({ m_rootPath, m_fileName + m_ext });
        dw.m_mesh = GetMeshManager()->Create<Mesh>(fullpath);
        dw.m_mesh->Init(false);

        // Tight fit a frustum to a bounding sphere
        // https://stackoverflow.com/questions/2866350/move-camera-to-fit-3d-scene
        BoundingBox bb = dw.GetAABB();
        Vec3 geoCenter = (bb.max + bb.min) * 0.5f;
        float r = glm::distance(geoCenter, bb.max) * 1.1f; // 10% safezone.
        float a = glm::radians(45.0f);
        float d = r / glm::tan(a / 2.0f);

        Vec3 eye = geoCenter + glm::normalize(Vec3(1.0f)) * d;

        Camera cam;
        cam.SetLens(a, thumbSize.x, thumbSize.y);
        cam.m_node->SetTranslation(eye);
        cam.LookAt(geoCenter);

        renderThumbFn(&cam, &dw);
      }
      else if (m_ext == MATERIAL)
      {
        Sphere ball;
        String fullpath = GetFullPath();
        ball.m_mesh->m_material = GetMaterialManager()->Create<Material>(fullpath);
        ball.m_mesh->Init(false);

        Camera cam;
        cam.SetLens(glm::half_pi<float>(), thumbSize.x, thumbSize.y);
        cam.m_node->SetTranslation(Vec3(0.0f, 0.0f, 1.5f));

        renderThumbFn(&cam, &ball);
      }
      else if (SupportedImageFormat(m_ext))
      {
        Quad frame;
        String fullpath = m_rootPath + GetPathSeparator() + m_fileName + m_ext;
        frame.m_mesh->m_material = GetMaterialManager()->GetCopyOfUnlitMaterial();
        frame.m_mesh->m_material->m_diffuseTexture = GetTextureManager()->Create<Texture>(fullpath);
        frame.m_mesh->m_material->m_diffuseTexture->Init(false);

        Camera cam;
        cam.SetLens(glm::half_pi<float>(), thumbSize.x, thumbSize.y);
        cam.m_node->SetTranslation(Vec3(0.0f, 0.0f, 0.5f));

        renderThumbFn(&cam, &frame);
      }
    }

    RenderTargetPtr DirectoryEntry::GetThumbnail() const
    {
      String fullPath = GetFullPath();
      if (g_app->m_thumbnailCache.find(fullPath) != g_app->m_thumbnailCache.end())
      {
        return g_app->m_thumbnailCache[fullPath];
      }

      return nullptr;
    }

    FolderView::FolderView()
    {
    }

    FolderView::FolderView(class FolderWindow* parent)
    {
      m_parent = parent;
    }

    void FolderView::Show()
    {
      bool* visCheck = nullptr;
      if (!m_currRoot)
      {
        visCheck = &m_visible;
      }

      if (ImGui::BeginTabItem(m_folder.c_str(), visCheck))
      {
        if (m_dirty)
        {
          Iterate();
          m_dirty = false;
        }

        // Handle Item Icon size.
        ImGuiIO io = ImGui::GetIO();
        static float wheel = io.MouseWheel;
        float delta = io.MouseWheel - wheel;

        const float icMin = 50.0f;
        const float icMax = 300.0f;
        if (io.KeyCtrl && ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows))
        {
          m_iconSize += Vec2(delta) * 15.0f;
          if (m_iconSize.x < icMin)
          {
            m_iconSize = Vec2(icMin);
          }

          if (m_iconSize.x > icMax)
          {
            m_iconSize = Vec2(icMax);
          }
        }

        // Show Tab path.
        if (ImGui::IsItemHovered())
        {
          ImGui::SetTooltip("%s", m_path.c_str());
        }

        // Start drawing folder items.
        const float footerHeightReserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("##Content", ImVec2(0, -footerHeightReserve), true);

        if (m_entiries.empty())
        {
          // Handle context menu based on path / content type of the folder.
          ShowContextMenu();
        }
        else
        {
          // Draw folder items.
          for (int i = 0; i < (int)m_entiries.size(); i++)
          {
            // Prepare Item Icon.
            ImGuiStyle& style = ImGui::GetStyle();
            float visX2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

            DirectoryEntry& dirEnt = m_entiries[i];
            if (!m_filter.PassFilter(dirEnt.m_fileName.c_str()))
            {
              continue;
            }

            bool flipRenderTarget = false;
            uint iconId = UI::m_fileIcon->m_textureId;

            auto genThumbFn = [&flipRenderTarget, &iconId, &dirEnt]() -> void
            {
              if (dirEnt.GetThumbnail() == nullptr)
              {
                dirEnt.GenerateThumbnail();
              }
              iconId = dirEnt.GetThumbnail()->m_textureId;
              flipRenderTarget = true;
            };

            if (dirEnt.m_isDirectory)
            {
              iconId = UI::m_folderIcon->m_textureId;
            }
            else if (dirEnt.m_ext == SCENE)
            {
              iconId = UI::m_worldIcon->m_textureId;
            }
            else if (dirEnt.m_ext == MESH)
            {
              genThumbFn();
            }
            else if (dirEnt.m_ext == ANIM)
            {
              iconId = UI::m_clipIcon->m_textureId;
            }
            else if (dirEnt.m_ext == SKINMESH)
            {
              iconId = UI::m_armatureIcon->m_textureId;
            }
            else if (dirEnt.m_ext == AUDIO)
            {
              iconId = UI::m_audioIcon->m_textureId;
            }
            else if (dirEnt.m_ext == SHADER)
            {
              iconId = UI::m_codeIcon->m_textureId;
            }
            else if (dirEnt.m_ext == SKELETON)
            {
              iconId = UI::m_boneIcon->m_textureId;
            }
            else if (dirEnt.m_ext == MATERIAL)
            {
              genThumbFn();
            }
            else if (SupportedImageFormat(dirEnt.m_ext))
            {
              genThumbFn();
            }
            else
            {
              if (m_onlyNativeTypes)
              {
                continue;
              }
            }

            ImGui::PushID(i);
            ImGui::BeginGroup();
            ImVec2 texCoords = flipRenderTarget ? ImVec2(1.0f, -1.0f) : ImVec2(1.0f, 1.0f);

            // Draw Item Icon.
            if (ImGui::ImageButton((void*)(intptr_t)iconId, m_iconSize, ImVec2(0.0f, 0.0f), texCoords))
            {
              ResourceManager* rm = dirEnt.GetManager();
              if (rm && rm->m_type == ResourceType::Material)
              {
                MaterialInspector* mi = g_app->GetMaterialInspector();
                mi->m_material = rm->Create<Material>(dirEnt.GetFullPath());
              }
            }

            // Handle context menu.
            ShowContextMenu(&dirEnt);

            // Handle if item is directory.
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
              if (ImGui::IsItemHovered())
              {
                if (dirEnt.m_isDirectory)
                {
                  if (m_parent != nullptr)
                  {
                    String path = ConcatPaths({ dirEnt.m_rootPath, dirEnt.m_fileName });
                    int indx = m_parent->Exist(path);
                    if (indx == -1)
                    {
                      FolderView view(m_parent);
                      view.SetPath(path);
                      view.Iterate();
                      m_parent->AddEntry(view);
                    }
                    else
                    {
                      m_parent->GetView(indx).m_visible = true;
                    }
                  }
                }
              }
            }

            // Handle mouse hover tips.
            String fullName = dirEnt.m_fileName + dirEnt.m_ext;
            UI::HelpMarker(LOC + fullName, fullName.c_str());

            // Handle drag - drop to scene / inspector.
            if (!dirEnt.m_isDirectory)
            {
              if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
              {
                ImGui::SetDragDropPayload("BrowserDragZone", &dirEnt, sizeof(DirectoryEntry));
                if (io.KeyShift)
                {
                  ImGui::SetTooltip("Copy %s", fullName.c_str());
                }
                else
                {
                  ImGui::SetTooltip("Instantiate %s", fullName.c_str());
                }
                ImGui::EndDragDropSource();
              }
            }

            // Handle Item sub text.
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + m_iconSize.x);
            size_t charLim = (size_t)(m_iconSize.x * 0.1f);
            if (dirEnt.m_fileName.size() > charLim)
            {
              String shorten = dirEnt.m_fileName.substr(0, charLim) + "...";
              ImGui::TextWrapped("%s", shorten.c_str());
            }
            else
            {
              ImGui::TextWrapped("%s", dirEnt.m_fileName.c_str());
            }
            ImGui::PopTextWrapPos();
            ImGui::EndGroup();
            ImGui::PopID();

            // Handle next column / row.
            float lastBtnX2 = ImGui::GetItemRectMax().x;
            float nextBtnX2 = lastBtnX2 + style.ItemSpacing.x + m_iconSize.x;
            if (nextBtnX2 < visX2)
            {
              ImGui::SameLine();
            }
          }
        } // Tab item handling ends.
        ImGui::EndChild();

        // Handle searchbar.
        ImGui::Separator();
        ImGui::Text("Filter: ");
        ImGui::SameLine();

        float width = ImGui::GetWindowContentRegionWidth() * 0.25f;
        m_filter.Draw("##Filter", width);

        ImGui::EndTabItem();
      }
    }

    void FolderView::SetPath(const String& path)
    {
      m_path = path;
      StringArray splits;
      Split(path, GetPathSeparatorAsStr(), splits);
      m_folder = splits.back();
    }

    const String& FolderView::GetPath() const
    {
      return m_path;
    }

    void FolderView::Iterate()
    {
      using namespace std::filesystem;

      m_entiries.clear();
      for (const directory_entry& e : directory_iterator(m_path))
      {
        DirectoryEntry de;
        de.m_isDirectory = e.is_directory();
        de.m_rootPath = e.path().parent_path().u8string();
        de.m_fileName = e.path().stem().u8string();
        de.m_ext = e.path().filename().extension().u8string();

        m_entiries.push_back(de);
      }
    }

    int FolderView::Exist(const String& file)
    {
      for (int i = 0; i < (int)m_entiries.size(); i++)
      {
        if (m_entiries[i].m_fileName == file)
        {
          return i;
        }
      }

      return -1;
    }

    void FolderView::ShowContextMenu(DirectoryEntry* entry)
    {
      String path = m_path + GetPathSeparatorAsStr();
      if (path.find(MaterialPath("")) != String::npos)
      {
        ShowContextForMaterial(entry);
      }
      else if (path.find(MeshPath("")) != String::npos)
      {
        ShowContextForMesh(entry);
      }
      else
      {
        ShowGenericContext();
      }
    }

    void FolderView::ShowContextForMaterial(DirectoryEntry* entry)
    {
      auto menuItemsFn = [entry, this](std::vector<bool> show) -> void
      {
        if (show[0] && ImGui::Button("Crate", m_contextBtnSize))
        {
          StringInputWindow* inputWnd = new StringInputWindow("Material Name##NwMat", true);
          inputWnd->m_inputVal = "New Material";
          inputWnd->m_inputLabel = "Name";
          inputWnd->m_hint = "New material name";
          inputWnd->m_taskFn = [this](const String& val)
          {
            String file = ConcatPaths({ m_path, val + MATERIAL });
            if (CheckFile(file))
            {
              g_app->GetConsole()->AddLog("Can't create. A material with the same name exist", ConsoleWindow::LogType::Error);
            }
            else
            {
              MaterialManager* man = GetMaterialManager();
              MaterialPtr mat = man->GetCopyOfSolidMaterial();
              mat->m_name = val;
              mat->m_file = file;
              m_dirty = true;
              mat->Save(true);
              man->Manage(mat);
            }
          };
          ImGui::CloseCurrentPopup();
        }

        if (entry)
        {
          if (show[1] && ImGui::Button("Copy", m_contextBtnSize))
          {
            if (ResourceManager* rm = entry->GetManager())
            {
              String path = entry->GetFullPath();
              MaterialPtr resource = rm->Create<Material>(path)->Copy<Material>();
              resource->Save(true);
              m_dirty = true;
            }

            ImGui::CloseCurrentPopup();
          }

          if (show[2] && ImGui::Button("Delete", m_contextBtnSize))
          {
            if (ResourceManager* rm = entry->GetManager())
            {
              if (MaterialPtr mat = rm->Create<Material>(entry->GetFullPath()))
              {
                if (g_app->m_scene->IsMaterialInUse(mat))
                {
                  g_app->GetConsole()->AddLog("Can't delete. Material is in use", ConsoleWindow::LogType::Error);
                }
                else
                {
                  std::filesystem::remove(entry->GetFullPath());
                  m_dirty = true;
                }
              }
            }

            ImGui::CloseCurrentPopup();
          }

          if (show[3] && ImGui::Button("Reload", m_contextBtnSize))
          {
            if (ResourceManager* rm = entry->GetManager())
            {
              if (MaterialPtr mat = rm->Create<Material>(entry->GetFullPath()))
              {
                mat->Reload();
                entry->GenerateThumbnail();
              }
            }

            ImGui::CloseCurrentPopup();
          }
        }

        if (show[4] && ImGui::Button("Refresh", m_contextBtnSize))
        {
          m_dirty = true;
          ImGui::CloseCurrentPopup();
        }
      };

      if (ImGui::BeginPopupContextItem())
      {
        menuItemsFn({ true, true, true, true, true });
        ImGui::EndPopup();
      }
      
      if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
      {
        menuItemsFn({ true, false, false, false, true });
        ImGui::EndPopup();
      }
    }

    void FolderView::ShowContextForMesh(DirectoryEntry* entry)
    {
      auto menuItemsFn = [entry, this](std::vector<bool> show) -> void
      {
        if (entry)
        {
          if (show[0] && ImGui::Button("Copy", m_contextBtnSize))
          {
            if (ResourceManager* rm = entry->GetManager())
            {
              String path = entry->GetFullPath();
              MeshPtr resource = rm->Create<Mesh>(path)->Copy<Mesh>();
              resource->Save(true);
              m_dirty = true;
            }

            ImGui::CloseCurrentPopup();
          }

          if (show[1] && ImGui::Button("Delete", m_contextBtnSize))
          {
            if (ResourceManager* rm = entry->GetManager())
            {
              if (MeshPtr res = rm->Create<Mesh>(entry->GetFullPath()))
              {
                if (g_app->m_scene->IsMeshInUse(res))
                {
                  g_app->GetConsole()->AddLog("Can't delete. Resource is in use", ConsoleWindow::LogType::Error);
                }
                else
                {
                  std::filesystem::remove(entry->GetFullPath());
                  m_dirty = true;
                }
              }
            }

            ImGui::CloseCurrentPopup();
          }
        }

        if (show[2] && ImGui::Button("Refresh", m_contextBtnSize))
        {
          m_dirty = true;
          ImGui::CloseCurrentPopup();
        }
      };

      if (ImGui::BeginPopupContextItem())
      {
        menuItemsFn({ true, true, true });
        ImGui::EndPopup();
      }

      if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
      {
        menuItemsFn({ false, false, true });
        ImGui::EndPopup();
      }
    }

    void FolderView::ShowGenericContext()
    {
      if (ImGui::BeginPopupContextWindow())
      {
        if (ImGui::Button("Refresh", m_contextBtnSize))
        {
          m_dirty = true;
          ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
      }
    }

    FolderWindow::FolderWindow(XmlNode* node)
    {
      DeSerialize(nullptr, node);
      Iterate(ResourcePath(), true);
    }

    FolderWindow::FolderWindow()
    {
    }

    FolderWindow::~FolderWindow()
    {
    }

    void FolderWindow::Show()
    {
      ImGui::SetNextWindowSize(ImVec2(300, 150), ImGuiCond_Once);
      if (ImGui::Begin(m_name.c_str(), &m_visible))
      {
        HandleStates();

        auto IsRootFn = [](const String& path)
        {
          size_t lastSep = path.find_last_of(GetPathSeparator());
          if (lastSep != String::npos)
          {
            String root = path.substr(0, lastSep);
            return root == ResourcePath();
          }

          return false;
        };

        if
        (
          !g_app->m_workspace.GetActiveWorkspace().empty() &&
          g_app->m_workspace.GetActiveProject().name.empty()
        )
        {
          ImGui::Text("Load a project.");
          ImGui::End();
          return;
        }

        if (m_showStructure)
        {
          // Show Resource folder structure.
          ImGui::PushID("##FolderStructure");
          ImGui::BeginGroup();

          ImGui::TextUnformatted("Resources");

          ImGui::SameLine();
          if (ImGui::Button("<<"))
          {
            m_showStructure = !m_showStructure;
          }

          ImGui::BeginChild("##Folders", ImVec2(130, 0), true);
          for (int i = 0; i < (int)m_entiries.size(); i++)
          {
            if (!IsRootFn(m_entiries[i].GetPath()))
            {
              continue;
            }

            bool currSel = false;
            if (m_activeFolder == i)
            {
              currSel = true;
            }

            currSel = UI::ToggleButton
            (
              m_entiries[i].m_folder,
              ImVec2(100, 25),
              currSel
            );

            // Selection switch.
            if (currSel)
            {
              if (i != m_activeFolder)
              {
                for (FolderView& view : m_entiries)
                {
                  view.m_visible = false;
                }
              }

              m_activeFolder = i;
            }
          }
          ImGui::EndChild();
          
          ImGui::EndGroup();
          ImGui::PopID();
        }
        else
        {
          if (ImGui::Button(">>"))
          {
            m_showStructure = !m_showStructure;
          }
        }

        ImGui::SameLine();

        ImGui::PushID("##FolderContent");
        ImGui::BeginGroup();
        if (ImGui::BeginTabBar("Folders", ImGuiTabBarFlags_NoTooltip | ImGuiTabBarFlags_AutoSelectNewTabs))
        {
          String currRootPath; 
          auto IsDescendentFn = [&currRootPath](String candidate) -> bool
          {
            return !currRootPath.empty() && candidate.find(currRootPath) != std::string::npos;
          };

          for (int i = 0; i < (int)m_entiries.size(); i++)
          {
            FolderView& view = m_entiries[i];
            String candidate = view.GetPath();
            if ((view.m_currRoot = (i == m_activeFolder)))
            {
              currRootPath = candidate;
            }
            
            // Show only current root folder or descendents.
            if (view.m_currRoot || IsDescendentFn(candidate))
            {
              view.Show();
            }
          }
          ImGui::EndTabBar();
        }

        ImGui::EndGroup();
        ImGui::PopID();
      }

      ImGui::End();
    }

    Window::Type FolderWindow::GetType() const
    {
      return Window::Type::Browser;
    }

    void FolderWindow::Iterate(const String& path, bool clear)
    {
      using namespace std::filesystem;

      if (clear)
      {
        m_entiries.clear();
      }
      
      for (const directory_entry& e : directory_iterator(path))
      {
        if (e.is_directory())
        {
          FolderView view(this);
          String path = e.path().u8string();
          view.SetPath(path);
          if (m_viewSettings.find(path) != m_viewSettings.end())
          {
            view.m_iconSize = m_viewSettings[path];
          }

          view.Iterate();
          m_entiries.push_back(view);
          Iterate(view.GetPath(), false);
        }
      }
    }

    void FolderWindow::UpdateContent()
    {
      for (FolderView& view : m_entiries)
      {
        view.Iterate();
      }
    }

    void FolderWindow::AddEntry(const FolderView& view)
    {
      if (Exist(view.GetPath()) == -1)
      {
        m_entiries.push_back(view);
      }
    }

    FolderView& FolderWindow::GetView(int indx)
    {
      return m_entiries[indx];
    }

    int FolderWindow::Exist(const String& folder)
    {
      for (size_t i = 0; i < m_entiries.size(); i++)
      {
        if (m_entiries[i].GetPath() == folder)
        {
          return (int)i;
        }
      }

      return -1;
    }

    bool FolderWindow::GetFileEntry(const String& fullPath, DirectoryEntry& entry)
    {
      String path, name, ext;
      DecomposePath(fullPath, &path, &name, &ext);
      int viewIndx = Exist(path);
      if (viewIndx != -1)
      {
        int dirEntry = m_entiries[viewIndx].Exist(name);
        if (dirEntry != -1)
        {
          entry = m_entiries[viewIndx].m_entiries[dirEntry];
          return true;
        }
      }

      return false;
    }

    void FolderWindow::Serialize(XmlDocument* doc, XmlNode* parent) const
    {
      Window::Serialize(doc, parent);
      XmlNode* node = parent->last_node();

      XmlNode* folder = doc->allocate_node(rapidxml::node_element, "FolderWindow");
      node->append_node(folder);
      WriteAttr(folder, doc, "activeFolder", std::to_string(m_activeFolder));
      WriteAttr(folder, doc, "showStructure", std::to_string(m_showStructure));

      for (const FolderView& view : m_entiries)
      {
        XmlNode* viewNode = doc->allocate_node(rapidxml::node_element, "FolderView");
        WriteAttr(viewNode, doc, "path", view.GetPath());
        folder->append_node(viewNode);
        XmlNode* setting = doc->allocate_node(rapidxml::node_element, "IconSize");
        WriteVec(setting, doc, view.m_iconSize);
        viewNode->append_node(setting);
      }
    }

    void FolderWindow::DeSerialize(XmlDocument* doc, XmlNode* parent)
    {
      Window::DeSerialize(doc, parent);
      if (XmlNode* node = parent->first_node("FolderWindow"))
      {
        ReadAttr(node, "activeFolder", m_activeFolder);
        ReadAttr(node, "showStructure", m_showStructure);

        if (XmlNode* view = node->first_node("FolderView"))
        {
          do
          {
            String path;
            ReadAttr(view, "path", path);
            Vec2 val(50.0f);
            if (XmlNode* setting = view->first_node("IconSize"))
            {
              ReadVec(setting, val);
            }
            m_viewSettings[path] = val;
          } while (view = view->next_sibling("FolderView"));
        }
      }
    }

  }
}

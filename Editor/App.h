#pragma once

#include "ToolKit.h"
#include "EditorScene.h"
#include "Workspace.h"

namespace ToolKit
{
  class Renderer;
  class Light;
  class Cube;
  class Sphere;
  class Camera;
  class Animation;

  namespace Editor
  {
    class Viewport;
    class Grid;
    class Axis3d;
    class Cursor;
    class ConsoleWindow;
    class FolderWindow;
    class OutlinerWindow;
    class PropInspector;
    class MaterialInspector;
    class Window;
    class Gizmo;

    class App : Serializable
    {
    public:
      App(int windowWidth, int windowHeight);
      virtual ~App();

      void Init();
      void Destroy();
      void Frame(float deltaTime);
      void OnResize(uint width, uint height);
      void OnNewScene(const String& name);
      void OnSaveScene();
      void OnQuit();
      void OnNewProject(const String& name);

      // UI
      void ResetUI();
      void DeleteWindows();
      void CreateWindows(XmlNode* parent);

      // Import facilities.
      int Import(const String& fullPath, const String& subDir, bool overwrite);
      bool CanImport(const String& fullPath);

      // Workspace
      void OpenScene(const String& fullPath);
      void MergeScene(const String& fullPath);
      void ApplyProjectSettings(bool setDefaults);
      void OpenProject(const Project& project);

      Viewport* GetActiveViewport(); // Returns open and active viewport or nullptr.
      Viewport* GetViewport(const String& name);
      ConsoleWindow* GetConsole();
      FolderWindow* GetAssetBrowser();
      OutlinerWindow* GetOutliner();
      PropInspector* GetPropInspector();
      MaterialInspector* GetMaterialInspector();

      template<typename T>
      T* GetWindow(const String& name);

      // Quick selected render implementation.
      void RenderSelected(Viewport* vp);

      virtual void Serialize(XmlDocument* doc, XmlNode* parent) const override;
      virtual void DeSerialize(XmlDocument* doc, XmlNode* parent) override;

    public:
      EditorScenePtr m_scene;

      // UI elements.
      std::vector<Window*> m_windows;

      // Editor variables.
      float m_camSpeed = 8.0; // Meters per sec.
      float m_mouseSensitivity = 0.5f;
      MaterialPtr m_highLightMaterial;
      MaterialPtr m_highLightSecondaryMaterial;
      Vec2 m_thumbnailSize = Vec2(300.0f, 300.0f);
      std::unordered_map<String, RenderTargetPtr> m_thumbnailCache;

      // Editor objects.
      Grid* m_grid;
      Axis3d* m_origin;
      Cursor* m_cursor;
      Gizmo* m_gizmo = nullptr;
      std::vector<Drawable*> m_perFrameDebugObjects;

      // 3 point lighting system.
      Node* m_lightMaster;
      LightRawPtrArray m_sceneLights; // { 0:key 1:fill, 2:back }

      // Editor states.
      int m_fps = 0;
      bool m_showPickingDebug = false;
      bool m_showStateTransitionsDebug = false;
      bool m_showOverlayUI = true;
      bool m_showOverlayUIAlways = true;
      bool m_importSlient = false;
      bool m_showSelectionBoundary = false;
      bool m_windowMaximized = false;
      Byte m_showGraphicsApiErrors = 0;
      TransformationSpace m_transformSpace = TransformationSpace::TS_WORLD;
      Workspace m_workspace;

      // Snap settings.
      bool m_snapsEnabled = false; // Delta transforms.
      bool m_snapToGrid = false; // Jump to grid junctions.
      float m_moveDelta = 0.25f;
      float m_rotateDelta = 15.0f;
      float m_scaleDelta = 0.5f;

      Renderer* m_renderer;

    private:
      bool m_onNewScene = false;
    };

  }
}

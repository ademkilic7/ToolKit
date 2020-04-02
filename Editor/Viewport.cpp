#include "stdafx.h"

#include "Viewport.h"
#include "Directional.h"
#include "Renderer.h"
#include "App.h"
#include "GlobalDef.h"
#include "SDL.h"
#include "DebugNew.h"
#include "OverlayMenu.h"
#include "Node.h"
#include "Primative.h"
#include "Grid.h"

using namespace ToolKit;

uint Editor::Viewport::m_nextId = 1;
Editor::OverlayNav* Editor::Viewport::m_overlayNav = nullptr;

Editor::Viewport::Viewport(float width, float height)
	: m_width(width), m_height(height), m_name(g_viewportStr)
{
	m_camera = new Camera();
	m_camera->SetLens(glm::quarter_pi<float>(), width, height);
	m_viewportImage = new RenderTarget((uint)width, (uint)height);
	m_viewportImage->Init();
	m_name += " " + std::to_string(m_nextId++);

	if (m_overlayNav == nullptr)
	{
		m_overlayNav = new OverlayNav(this);
	}
}

Editor::Viewport::~Viewport()
{
	SafeDel(m_camera);
	SafeDel(m_viewportImage);
}

void Editor::Viewport::Show()
{
	ImGui::SetNextWindowSize(ImVec2(m_width, m_height), ImGuiCond_Once);
	ImGui::Begin(m_name.c_str(), &m_open);
	{
		HandleStates();

		// Content area size
		ImVec2 vMin = ImGui::GetWindowContentRegionMin();
		ImVec2 vMax = ImGui::GetWindowContentRegionMax();

		vMin.x += ImGui::GetWindowPos().x;
		vMin.y += ImGui::GetWindowPos().y;
		vMax.x += ImGui::GetWindowPos().x;
		vMax.y += ImGui::GetWindowPos().y;

		m_wndPos.x = vMin.x;
		m_wndPos.y = vMin.y;

		m_wndContentAreaSize = *(glm::vec2*)&ImVec2(glm::abs(vMax.x - vMin.x), glm::abs(vMax.y - vMin.y));
		ImGuiIO& io = ImGui::GetIO();
		
		m_mouseOverContentArea = false;
		ImVec2 absMousePos = io.MousePos;
		if (vMin.x < absMousePos.x && vMax.x > absMousePos.x)
		{
			if (vMin.y < absMousePos.y && vMax.y > absMousePos.y)
			{
				m_mouseOverContentArea = true;
			}
		}

		m_lastMousePosRelContentArea.x = (int)(absMousePos.x - vMin.x);
		m_lastMousePosRelContentArea.y = (int)(absMousePos.y - vMin.y);

		if (!ImGui::IsWindowCollapsed())
		{
			if (m_wndContentAreaSize.x > 0 && m_wndContentAreaSize.y > 0)
			{
				ImGui::Image((void*)(intptr_t)m_viewportImage->m_textureId, ImVec2(m_width, m_height), ImVec2(0.0f, 0.0f), ImVec2(1.0f, -1.0f));

				ImVec2 currSize = ImGui::GetContentRegionMax();
				if (m_wndContentAreaSize.x != m_width || m_wndContentAreaSize.y != m_height)
				{
					OnResize(m_wndContentAreaSize.x, m_wndContentAreaSize.y);
				}

				if (IsActive())
				{
					ImGui::GetWindowDrawList()->AddRect(vMin, vMax, IM_COL32(255, 255, 0, 255));
				}
				else
				{
					ImGui::GetWindowDrawList()->AddRect(vMin, vMax, IM_COL32(128, 128, 128, 255));
				}
			}
		}
	}

	if (g_app->m_showOverlayUI)
	{
		if (IsActive() && m_overlayNav != nullptr)
		{
			m_overlayNav->m_owner = this;
			m_overlayNav->Show();
		}
	}

	m_mouseHover = ImGui::IsWindowHovered();

	// Process draw commands.
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	for (auto command : m_drawCommands)
	{
		command(drawList);
	}
	m_drawCommands.clear();

	ImGui::End();
}

void Editor::Viewport::Update(float deltaTime)
{
	if (!IsActive())
	{
		return;
	}

	// Update viewport mods.
	FpsNavigationMode(deltaTime);
}

void Editor::Viewport::OnResize(float width, float height)
{
	m_width = width;
	m_height = height;
	m_camera->SetLens(m_camera->GetData().fov, width, height);

	m_viewportImage->UnInit();
	m_viewportImage->m_width = (uint)width;
	m_viewportImage->m_height = (uint)height;
	m_viewportImage->Init();
}

ToolKit::Editor::Window::Type ToolKit::Editor::Viewport::GetType()
{
	return Type::Viewport;
}

bool ToolKit::Editor::Viewport::IsViewportQueriable()
{
	return m_mouseOverContentArea && m_mouseHover && m_active && m_open && m_relMouseModBegin;
}

Ray ToolKit::Editor::Viewport::RayFromMousePosition()
{
	Ray ray;
	ray.position = GetLastMousePosWorldSpace();
	ray.direction = glm::normalize(ray.position - m_camera->m_node->m_translation);

	return ray;
}

glm::vec3 ToolKit::Editor::Viewport::GetLastMousePosWorldSpace()
{
	return TransformViewportToWorldSpace(GetLastMousePosViewportSpace());
}

glm::vec2 ToolKit::Editor::Viewport::GetLastMousePosViewportSpace()
{
	glm::vec2 screenPoint = m_lastMousePosRelContentArea;
	screenPoint.y = m_wndContentAreaSize.y - screenPoint.y; // Imgui Window origin Top - Left to OpenGL window origin Bottom - Left

	return screenPoint;
}

glm::vec2 ToolKit::Editor::Viewport::GetLastMousePosScreenSpace()
{
	glm::vec2 screenPoint = GetLastMousePosViewportSpace();
	screenPoint.y = m_wndContentAreaSize.y - screenPoint.y; // Bring it back from opengl (BottomLeft) to Imgui (TopLeft) system.

	return m_wndPos + screenPoint; // Move it from window space to screen space.
}

glm::vec3 ToolKit::Editor::Viewport::TransformViewportToWorldSpace(const glm::vec2& pnt)
{
	glm::vec3 screenPoint = glm::vec3(pnt, 0.0f);

	glm::mat4 view = m_camera->GetViewMatrix();
	glm::mat4 project = m_camera->GetData().projection;

	return glm::unProject(screenPoint, view, project, glm::vec4(0.0f, 0.0f, m_width, m_height));
}

glm::vec2 ToolKit::Editor::Viewport::TransformScreenToViewportSpace(const glm::vec2& pnt)
{
	glm::vec2 vp = pnt - m_wndPos; // In window space.
	vp.y = m_wndContentAreaSize.y - vp.y; // In viewport space.
	return vp;
}

void Editor::Viewport::FpsNavigationMode(float deltaTime)
{
	if (m_camera)
	{
		// Mouse is right clicked
		if (ImGui::IsMouseDown(1))
		{
			// Handle relative mouse hack.
			if (m_relMouseModBegin)
			{
				m_relMouseModBegin = false;
				SDL_GetGlobalMouseState(&m_mousePosBegin.x, &m_mousePosBegin.y);				
			}

			ImGui::SetMouseCursor(ImGuiMouseCursor_None);
			glm::ivec2 mp;
			SDL_GetGlobalMouseState(&mp.x, &mp.y);
			mp = mp - m_mousePosBegin;
			SDL_WarpMouseGlobal(m_mousePosBegin.x, m_mousePosBegin.y);
			// End of relative mouse hack.

			m_camera->Pitch(-glm::radians(mp.y * g_app->m_mouseSensitivity));
			m_camera->RotateOnUpVector(-glm::radians(mp.x * g_app->m_mouseSensitivity));

			glm::vec3 dir, up, right;
			dir = -ToolKit::Z_AXIS;
			up = ToolKit::Y_AXIS;
			right = ToolKit::X_AXIS;

			float speed = g_app->m_camSpeed;

			glm::vec3 move;
			ImGuiIO& io = ImGui::GetIO();
			if (io.KeysDown[SDL_SCANCODE_A])
			{
				move += -right;
			}

			if (io.KeysDown[SDL_SCANCODE_D])
			{
				move += right;
			}

			if (io.KeysDown[SDL_SCANCODE_W])
			{
				move += dir;
			}

			if (io.KeysDown[SDL_SCANCODE_S])
			{
				move += -dir;
			}

			if (io.KeysDown[SDL_SCANCODE_PAGEUP])
			{
				move += up;
			}

			if (io.KeysDown[SDL_SCANCODE_PAGEDOWN])
			{
				move += -up;
			}

			float displace = speed * MilisecToSec(deltaTime);
			if (length(move) > 0.0f)
			{
				move = normalize(move);
			}

			m_camera->Translate(move * displace);
		}
		else
		{
			if (!m_relMouseModBegin)
			{
				m_relMouseModBegin = true;
			}
		}
	} 
}

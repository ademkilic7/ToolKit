#include "stdafx.h"

#include "OverlayUI.h"
#include "Viewport.h"
#include "GlobalDef.h"
#include "Mod.h"
#include "ConsoleWindow.h"
#include "DebugNew.h"

namespace ToolKit
{
	namespace Editor
	{
		
		OverlayUI::OverlayUI(Viewport* owner)
		{
			m_owner = owner;
		}

		OverlayUI::~OverlayUI()
		{
		}

		// OverlayMods
		//////////////////////////////////////////////////////////////////////////

		OverlayMods::OverlayMods(Viewport* owner)
			: OverlayUI(owner)
		{
		}

		void OverlayMods::Show()
		{
			ImVec2 overlaySize(48, 258);
			const float padding = 5.0f;
			ImVec2 window_pos = ImVec2(m_owner->m_wndPos.x + padding, m_owner->m_wndPos.y + padding);
			ImGui::SetNextWindowPos(window_pos);
			ImGui::SetNextWindowBgAlpha(0.65f);
			if (ImGui::BeginChildFrame(ImGui::GetID("Navigation"), overlaySize, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
			{
				// Select button.
				static float hoverTimeSelectBtn = 0.0f;
				bool isCurrentMod = ModManager::GetInstance()->m_modStack.back()->m_id == ModId::Select;
				ModManager::GetInstance()->SetMod
				(
					UI::ToggleButton((void*)(intptr_t)UI::m_selectIcn->m_textureId, ImVec2(32, 32), isCurrentMod) && !isCurrentMod,
					ModId::Select
				);
				UI::HelpMarker("Select Box\nSelect items using box selection.", &hoverTimeSelectBtn);

				// Cursor button.
				static float hoverTimeCursorBtn = 0.0f;
				isCurrentMod = ModManager::GetInstance()->m_modStack.back()->m_id == ModId::Cursor;
				ModManager::GetInstance()->SetMod
				(
					UI::ToggleButton((void*)(intptr_t)UI::m_cursorIcn->m_textureId, ImVec2(32, 32), isCurrentMod) && !isCurrentMod,
					ModId::Cursor
				);
				UI::HelpMarker("Cursor\nSet the cursor location.", &hoverTimeCursorBtn);
				ImGui::Separator();

				// Move button.
				static float hoverTimeMoveBtn = 0.0f;
				isCurrentMod = ModManager::GetInstance()->m_modStack.back()->m_id == ModId::Move;
				ModManager::GetInstance()->SetMod
				(
					UI::ToggleButton((void*)(intptr_t)UI::m_moveIcn->m_textureId, ImVec2(32, 32), isCurrentMod) && !isCurrentMod,
					ModId::Move
				);
				UI::HelpMarker("Move\nMove selected items.", &hoverTimeMoveBtn);

				// Rotate button.
				static float hoverTimeRotateBtn = 0.0f;
				isCurrentMod = ModManager::GetInstance()->m_modStack.back()->m_id == ModId::Rotate;
				ModManager::GetInstance()->SetMod
				(
					UI::ToggleButton((void*)(intptr_t)UI::m_rotateIcn->m_textureId, ImVec2(32, 32), isCurrentMod) && !isCurrentMod,
					ModId::Rotate
				);
				UI::HelpMarker("Rotate\nRotate selected items.", &hoverTimeRotateBtn);

				// Scale button.
				static float hoverTimeScaleBtn = 0.0f;
				isCurrentMod = ModManager::GetInstance()->m_modStack.back()->m_id == ModId::Scale;
				ModManager::GetInstance()->SetMod
				(
					UI::ToggleButton((void*)(intptr_t)UI::m_scaleIcn->m_textureId, ImVec2(32, 32), isCurrentMod) && !isCurrentMod,
					ModId::Scale
				);
				UI::HelpMarker("Scale\nScale (resize) selected items.", &hoverTimeScaleBtn);
				ImGui::Separator();

				const char* items[] = { "1", "2", "4", "8", "16" };
				static int current_item = 3; // Also the default.
				ImGui::PushItemWidth(40);
				if (ImGui::BeginCombo("##CS", items[current_item], ImGuiComboFlags_None))
				{
					for (int n = 0; n < IM_ARRAYSIZE(items); n++)
					{
						bool is_selected = (current_item == n);
						if (ImGui::Selectable(items[n], is_selected))
						{
							current_item = n;
						}

						if (is_selected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
				ImGui::PopItemWidth();

				switch (current_item)
				{
				case 0:
					g_app->m_camSpeed = 0.5f;
					break;
				case 1:
					g_app->m_camSpeed = 1.0f;
					break;
				case 2:
					g_app->m_camSpeed = 2.0f;
					break;
				case 3:
					g_app->m_camSpeed = 4.0f;
					break;
				case 4:
					g_app->m_camSpeed = 16.0f;
					break;
				default:
					g_app->m_camSpeed = 8;
					break;
				}

				ImGuiStyle& style = ImGui::GetStyle();
				float spacing = style.ItemInnerSpacing.x;

				static float hoverTimeCS = 0.0f;
				ImGui::SameLine(0, spacing); UI::HelpMarker("Camera speed m/s\n", &hoverTimeCS);
			}
			ImGui::EndChildFrame();
		}

		// OverlayViewportOptions
		//////////////////////////////////////////////////////////////////////////

		OverlayViewportOptions::OverlayViewportOptions(Viewport* owner)
			: OverlayUI(owner)
		{
		}

		void OverlayViewportOptions::Show()
		{
			ImVec2 overlaySize(154, 24);
			const float padding = 5.0f;
			ImVec2 window_pos = ImVec2(m_owner->m_wndPos.x + padding + 50, m_owner->m_wndPos.y + padding);
			ImGui::SetNextWindowPos(window_pos);
			ImGui::SetNextWindowBgAlpha(0.65f);
			if (ImGui::BeginChildFrame(ImGui::GetID("ViewportOptions"), overlaySize, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
			{
				const char* itemsCam[] = { "Persp.", "Top", "Front", "Left" };
				static int currentItemCam = 0;
				bool change = false;

				ImGui::PushItemWidth(72);
				if (ImGui::BeginCombo("##VC", itemsCam[currentItemCam], ImGuiComboFlags_None))
				{
					for (int n = 0; n < IM_ARRAYSIZE(itemsCam); n++)
					{
						bool is_selected = (currentItemCam == n);
						if (ImGui::Selectable(itemsCam[n], is_selected))
						{
							if (currentItemCam != n)
							{
								change = true;
							}
							currentItemCam = n;
						}

						if (is_selected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
				ImGui::PopItemWidth();

				if (change)
				{
					String view;
					switch (currentItemCam)
					{
					case 1:
						view = "top";
						break;
					case 2:
						view = "front";
						break;
					case 3:
						view = "left";
						break;
					case 0:
					default:
						view = "persp.";
						break;
					}

					String cmd = "SetCameraTransform --v \"" + m_owner->m_name + "\" " + view;
					g_app->GetConsole()->ExecCommand(cmd);
				}


				ImGuiStyle& style = ImGui::GetStyle();
				float spacing = style.ItemInnerSpacing.x;

				static float hoverTimeCA = 0.0f;
				ImGui::SameLine(0, spacing); UI::HelpMarker("Camera alignment\n", &hoverTimeCA);

				const char* items[] = { "World", "Parent", "Local" };
				static int current_item = 0;

				change = false;
				ImGui::PushItemWidth(72);
				if (ImGui::BeginCombo("##TRS", items[current_item], ImGuiComboFlags_None))
				{
					for (int n = 0; n < IM_ARRAYSIZE(items); n++)
					{
						bool is_selected = (current_item == n);
						if (ImGui::Selectable(items[n], is_selected))
						{
							if (current_item != n)
							{
								change = true;
							}
							current_item = n;
						}

						if (is_selected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
				ImGui::PopItemWidth();
				
				if (change)
				{
					String ts;
					switch (current_item)
					{
					case 1:
						ts = "parent";
						break;
					case 2:
						ts = "local";
						break;
					case 0:
					default:
						ts = "world";
						break;
					}

					String cmd = "SetTransformOrientation " + ts;
					g_app->GetConsole()->ExecCommand(cmd);
				}

				static float hoverTimeTO = 0.0f;
				ImGui::SameLine(0, spacing); UI::HelpMarker("Transform orientations\n", &hoverTimeTO);
			}
			ImGui::EndChildFrame();
		}

	}
}

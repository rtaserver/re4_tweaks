#include "dllmain.h"
#include "UI_Utility.h"
#include "input.hpp"
#include <stack>
#include "ConsoleWnd.h"
#include "Settings.h"
#include <imgui/imgui_internal.h>

MenuTab Tab = MenuTab::Display;
TrainerTab CurTrainerTab = TrainerTab::Patches;
ImGuiTextFilterCustom OptionsFilter;

void ImGui_ItemSeparator()
{
	// ImGui::Separator() doesn't work inside tables?

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImVec2 p0 = ImGui::GetCursorScreenPos();

	drawList->AddLine(ImVec2(p0.x, p0.y), ImVec2(p0.x + ImGui::GetContentRegionAvail().x, p0.y), ImGui::GetColorU32(ImGuiCol_Separator));
}

void ImGui_ItemSeparator2()
{
	// ImGui::Separator() doesn't work inside tables?

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImVec2 p0 = ImGui::GetCursorScreenPos();

	drawList->AddRectFilled(ImVec2(p0.x, p0.y), ImVec2(p0.x + ImGui::GetContentRegionAvail().x, p0.y + 3), ImColor(150, 10, 40));
}

// Customized version that puts the label inside the inputbox
bool ImGuiTextFilterCustom::Draw2(const char* label, float width) {
	if (width != 0.0f)
		ImGui::SetNextItemWidth(width);

	std::string id = std::string("##Input_") += label;
	bool value_changed = ImGui::InputTextWithHint(id.c_str(), label, InputBuf, IM_ARRAYSIZE(InputBuf));
	if (value_changed)
		Build();
	return value_changed;
}

ImColor itmbgColor = ImColor(25, 20, 20, 166);
int itemIdx = -1;

std::vector<float> columnLastY;
void ImGui_ColumnInit()
{
	itemIdx = -1;

	columnLastY.clear();
	columnLastY.resize(ImGui::TableGetColumnCount());

	ImGui::TableNextColumn();
}

std::stack<ImDrawListSplitter> bgSplitters;

// ImGui_BeginBackground & ImGui_EndBackground:
// Draw a background behind/below an item (or an ImGui Group) by splitting the drawList.
// Supports nested background inside of each other.
void ImGui_BeginBackground()
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	bgSplitters.emplace();
	bgSplitters.top().Split(drawList, 2);
	bgSplitters.top().SetCurrentChannel(drawList, 1);
}

void ImGui_EndBackground(float extraMargin, float rounding, bool fillX, ImColor bgCol)
{
	if (bgSplitters.empty())
		return;

	ImVec2 groupMin = ImGui::GetItemRectMin();
	ImVec2 groupMax = ImGui::GetItemRectMax();

	ImDrawList* drawList = ImGui::GetWindowDrawList();

	if (fillX)
		groupMax.x = groupMin.x + ImGui::GetContentRegionAvail().x;

	bgSplitters.top().SetCurrentChannel(drawList, 0);

	drawList->AddRectFilled(ImVec2(groupMin.x - extraMargin, groupMin.y - extraMargin), ImVec2(groupMax.x + extraMargin, groupMax.y + extraMargin), bgCol, rounding, ImDrawCornerFlags_All);

	bgSplitters.top().Merge(drawList);
	bgSplitters.top().Clear();
	bgSplitters.pop();
}

void ImGui_ColumnSwitch()
{
	if (itemIdx >= 0)
	{
		ImGui::EndGroup();
		ImGui_EndBackground();

		ImGui::Dummy(ImVec2(10, 25));
		columnLastY[ImGui::TableGetColumnIndex()] = ImGui::GetCursorPos().y;
	}

	itemIdx++;

	// Insert new item into smallest column
	auto smallestColumn = std::min_element(columnLastY.begin(), columnLastY.end());
	int smallestColumnIdx = std::distance(columnLastY.begin(), smallestColumn);

	ImGui::TableSetColumnIndex(smallestColumnIdx);

	// Restore columnLastY for every item but the first on each column
	if (itemIdx > (ImGui::TableGetColumnCount() - 1))
		ImGui::SetCursorPosY(columnLastY[ImGui::TableGetColumnIndex()]);

	ImGui_BeginBackground();
	ImGui::BeginGroup();
}

void ImGui_ColumnFinish()
{
	if (itemIdx <= -1)
		return;

	ImGui::EndGroup();
	ImGui_EndBackground();
	ImGui::Dummy(ImVec2(10, 25));
}

void ImGui_SetHotkey(std::string* cfgHotkey, bool supportsCombo)
{
	unsigned int HotkeyVK1 = 0;
	unsigned int HotkeyVK2 = 0;

	std::string OrigHotkeyCombo = *cfgHotkey;
	std::string FinalHotkeyCombo;

	// Hacky way to change ImGui's current button label
	*cfgHotkey = "Press any key...";

	bool waitingforkey = true;

	while (waitingforkey) {
		for (unsigned int Key1 = 0; Key1 < 256; Key1++)
		{
			while (pInput->is_key_down(Key1)) {
				HotkeyVK1 = Key1;

				if (supportsCombo)
				{
					for (unsigned int Key2 = 0; Key2 < 256; Key2++)
					{
						if (pInput->is_key_down(Key2) && (Key2 != Key1))
						{
							HotkeyVK2 = Key2;
						}
					}
				}
				waitingforkey = false;
			}
		}
	}

	// Clear the second hotkey if they're the same.
	// I've had that happen before due to deadkey weirdness.
	if (HotkeyVK1 == HotkeyVK2)
		HotkeyVK2 = 0;

	#ifdef VERBOSE
	con.log("HotkeyVK1 = %d", HotkeyVK1);
	con.log("HotkeyVK2 = %d", HotkeyVK2);
	#endif

	// Check if our KeyMap function is able to identify the key names, restore the original combo if not.
	// As of now, there's no popup informing the user that the key is unsupported. TODO: Add popup?
	if (HotkeyVK2 > 0)
	{
		if (pInput->key_name_from_vk(HotkeyVK1).empty() || pInput->key_name_from_vk(HotkeyVK2).empty())
		{
			*cfgHotkey = OrigHotkeyCombo;
			return;
		}
		else
			FinalHotkeyCombo = pInput->key_name_from_vk(HotkeyVK1) + "+" + pInput->key_name_from_vk(HotkeyVK2);
	}
	else
	{
		if (pInput->key_name_from_vk(HotkeyVK1).empty())
		{
			*cfgHotkey = OrigHotkeyCombo;
			return;
		}
		else
			FinalHotkeyCombo = pInput->key_name_from_vk(HotkeyVK1);
	}

	#ifdef VERBOSE
	con.log("FinalHotkeyCombo = %s", FinalHotkeyCombo.c_str());
	#endif

	*cfgHotkey = FinalHotkeyCombo;
}

// If we don't run these in another thread, we end up locking the rendering
void ImGui_SetHotkeyComboThread(std::string* cfgHotkey)
{
	bWaitingForHotkey = true;

	ImGui_SetHotkey(cfgHotkey, true);

	bWaitingForHotkey = false;
	return;
}

void ImGui_SetHotkeyThread(std::string* cfgHotkey)
{
	bWaitingForHotkey = true;

	ImGui_SetHotkey(cfgHotkey, false);

	bWaitingForHotkey = false;
	return;
}

bool ImGui_ButtonSameLine(const char* label, bool samelinecheck, float offset,
	const ImVec2& size)
{
	bool ret = ImGui::Button(label, size);

	if (samelinecheck)
	{
		ImGuiStyle& style = ImGui::GetStyle();

		float window_visible_x2 = ImGui::GetCursorScreenPos().x + offset + ImGui::GetContentRegionAvail().x;
		float last_button_x2;
		float next_button_x2;

		last_button_x2 = ImGui::GetItemRectMax().x;
		next_button_x2 = last_button_x2 + style.ItemSpacing.x + size.x; // Expected position if next button was on same line
		if (next_button_x2 < window_visible_x2)
			ImGui::SameLine();
	}

	return ret;
}

bool ImGui_TabButton(const char* btnID, const char* text, const ImVec4& activeCol,
	const ImVec4& inactiveCol, MenuTab tabID, const char* icon, const ImColor& iconColor,
	const ImColor& textColor, const ImVec2& size)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	ImGui::PushStyleColor(ImGuiCol_Button, Tab == tabID ? activeCol : inactiveCol);
	bool ret = ImGui::Button(btnID, size);
	ImGui::PopStyleColor();

	auto p0 = ImGui::GetItemRectMin();
	auto p1 = ImGui::GetItemRectMax();

	float text_pos_x = (p1.x - p0.x) / 3;
	float icon_pos_x = text_pos_x / 2;

	float text_pos_y = (p1.y - p0.y) / 4;
	float icon_pos_y = (p1.y - p0.y) / 4;

	drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(p0.x + icon_pos_x, p0.y + icon_pos_y), iconColor, icon, NULL, 0.0f, &ImVec4(p0.x, p0.y, p1.x, p1.y));
	drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(p0.x + text_pos_x, p0.y + text_pos_y), textColor, text, NULL, 0.0f, &ImVec4(p0.x, p0.y, p1.x, p1.y));

	if (ret)
	{
		Tab = tabID;
		OptionsFilter.Clear();
	}

	return ret;
}

bool ImGui_TrainerTabButton(const char* btnID, const char* text, const ImVec4& activeCol,
	const ImVec4& inactiveCol, TrainerTab tabID, const char* icon, const ImColor& iconColor,
	const ImColor& textColor, const ImVec2& size, const bool samelinecheck)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	ImGui::PushStyleColor(ImGuiCol_Button, CurTrainerTab == tabID ? activeCol : inactiveCol);
	bool ret = ImGui_ButtonSameLine(btnID, samelinecheck, 0.0f, size);
	ImGui::PopStyleColor();

	auto p0 = ImGui::GetItemRectMin();
	auto p1 = ImGui::GetItemRectMax();

	float textWidth = ImGui::CalcTextSize(text).x;
	float textHeight = ImGui::CalcTextSize(text).y;
	float iconWidth = ImGui::CalcTextSize(icon).x;

	float x_text_offset = (((p1.x - p0.x) - textWidth + iconWidth + 7.0f) * 0.5f);
	float y_text_offset = ((p1.y - p0.y) - textHeight) * 0.5f;

	float x_icon_offset = x_text_offset - iconWidth - 7.0f;
	float y_icon_offset = y_text_offset * 1.0f;

	drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(p0.x + x_icon_offset, p0.y + y_icon_offset), iconColor, icon, NULL, 0.0f, &ImVec4(p0.x, p0.y, p1.x, p1.y));
	drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(p0.x + x_text_offset, p0.y + y_text_offset), textColor, text, NULL, 0.0f, &ImVec4(p0.x, p0.y, p1.x, p1.y));

	if (ret && tabID != TrainerTab::NumTabs)
		CurTrainerTab = tabID;

	return ret;
}

bool ImGui_BufferingBar(const char* label, float value, const ImVec2& size_arg, const ImU32& bg_col, const ImU32& fg_col) {
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);

	ImVec2 pos = window->DC.CursorPos;
	ImVec2 size = size_arg;
	size.x -= style.FramePadding.x * 2;

	const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
	ImGui::ItemSize(bb, style.FramePadding.y);
	if (!ImGui::ItemAdd(bb, id))
		return false;

	// Render
	const float circleStart = size.x * 0.7f;
	const float circleEnd = size.x;
	const float circleWidth = circleEnd - circleStart;

	window->DrawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart, bb.Max.y), bg_col);
	window->DrawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart * value, bb.Max.y), fg_col);

	const float t = (float)g.Time;
	const float r = size.y / 2;
	const float speed = 1.5f;

	const float a = speed * 0;
	const float b = speed * 0.333f;
	const float c = speed * 0.666f;

	const float o1 = (circleWidth + r) * (t + a - speed * (int)((t + a) / speed)) / speed;
	const float o2 = (circleWidth + r) * (t + b - speed * (int)((t + b) / speed)) / speed;
	const float o3 = (circleWidth + r) * (t + c - speed * (int)((t + c) / speed)) / speed;

	window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o1, bb.Min.y + r), r, bg_col);
	window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o2, bb.Min.y + r), r, bg_col);
	window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o3, bb.Min.y + r), r, bg_col);

	return true;
}

bool ImGui_Spinner(const char* label, float radius, int thickness, const ImU32& color) {
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);

	ImVec2 pos = window->DC.CursorPos;
	ImVec2 size((radius) * 2, (radius + style.FramePadding.y) * 2);

	const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
	ImGui::ItemSize(bb, style.FramePadding.y);
	if (!ImGui::ItemAdd(bb, id))
		return false;

	// Render
	window->DrawList->PathClear();

	int num_segments = 30;
	int start = (int)abs(ImSin((float)g.Time * 1.8f) * (num_segments - 5));

	const float a_min = IM_PI * 2.0f * ((float)start) / (float)num_segments;
	const float a_max = IM_PI * 2.0f * ((float)num_segments - 3) / (float)num_segments;

	const ImVec2 centre = ImVec2(pos.x + radius, pos.y + radius + style.FramePadding.y);

	for (int i = 0; i < num_segments; i++) {
		const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
		window->DrawList->PathLineTo(ImVec2(centre.x + ImCos(a + (float)g.Time * 8) * radius,
			centre.y + ImSin(a + (float)g.Time * 8) * radius));
	}

	window->DrawList->PathStroke(color, false, (float)thickness);

	return true;
}

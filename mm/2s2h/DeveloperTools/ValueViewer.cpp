#include "ValueViewer.h"
#include "2s2h/BenGui/UIWidgets.hpp"
#include "2s2h/BenGui/BenGui.hpp"
#include "2s2h/GameInteractor/GameInteractor.h"
#include <spdlog/fmt/fmt.h>

extern "C" {
#include "z64.h"
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "gfxprint.h"
extern PlayState* gPlayState;
}

#define CVAR_NAME "gDeveloperTools.ValueViewer.EnablePrinting"
#define CVAR_DEFAULT 0
#define CVAR_VALUE CVarGetInteger(CVAR_NAME, CVAR_DEFAULT)

using namespace UIWidgets;

ImVec4 WHITE = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

// clang-format off
static std::vector<ValueTableElement> valueTable = {
    { "Time",               "gSaveContext.save.time",                 "TIME:",   TYPE_U32,   false, []() -> void* { return &gSaveContext.save.time; },                      WHITE },
    { "Age",                "gSaveContext.save.linkAge",              "AGE:",    TYPE_S32,   false, []() -> void* { return &gSaveContext.save.linkAge; },                   WHITE },
    { "Health",             "gSaveContext.save.saveInfo.playerData.health", "HP:", TYPE_S16, false, []() -> void* { return &gSaveContext.save.saveInfo.playerData.health; }, WHITE },
    { "Scene ID",           "gPlayState->sceneId",                    "SCENE:",  TYPE_S16,   true,  []() -> void* { return &gPlayState->sceneId; },                         WHITE },
    { "Entrance ID",        "gSaveContext.save.entrance",             "ENTR:",   TYPE_S32,   false, []() -> void* { return &gSaveContext.save.entrance; },                  WHITE },
    { "Link X",             "Player->actor.world.pos.x",              "X:",      TYPE_FLOAT, true,  []() -> void* { return &GET_PLAYER(gPlayState)->actor.world.pos.x; },  WHITE },
    { "Link Y",             "Player->actor.world.pos.y",              "Y:",      TYPE_FLOAT, true,  []() -> void* { return &GET_PLAYER(gPlayState)->actor.world.pos.y; },  WHITE },
    { "Link Z",             "Player->actor.world.pos.z",              "Z:",      TYPE_FLOAT, true,  []() -> void* { return &GET_PLAYER(gPlayState)->actor.world.pos.z; },  WHITE },
    { "Link Yaw",           "Player->actor.world.rot.y",              "ROT:",    TYPE_S16,   true,  []() -> void* { return &GET_PLAYER(gPlayState)->actor.world.rot.y; },  WHITE },
    { "Link Velocity",      "Player->speedXZ",                        "V:",      TYPE_FLOAT, true,  []() -> void* { return &GET_PLAYER(gPlayState)->speedXZ; },            WHITE },
    { "Link X Velocity",    "Player->actor.velocity.x",               "XV:",     TYPE_FLOAT, true,  []() -> void* { return &GET_PLAYER(gPlayState)->actor.velocity.x; },   WHITE },
    { "Link Y Velocity",    "Player->actor.velocity.y",               "YV:",     TYPE_FLOAT, true,  []() -> void* { return &GET_PLAYER(gPlayState)->actor.velocity.y; },   WHITE },
    { "Link Z Velocity",    "Player->actor.velocity.z",               "ZV:",     TYPE_FLOAT, true,  []() -> void* { return &GET_PLAYER(gPlayState)->actor.velocity.z; },   WHITE },
};
// clang-format on

void ValueViewer_DrawElement(ValueTableElement* element) {
    if (!element->isPrinted) {
        return;
    }

    if (element->requiresPlayState && gPlayState == nullptr) {
        return;
    }

    void* ptr = element->valueFn();
    if (ptr == nullptr) {
        return;
    }

    switch (element->type) {
        case TYPE_S8:
            element->prefix = fmt::format("{:d}", (int)*(s8*)ptr);
            break;
        case TYPE_U8:
            element->prefix = fmt::format("{:d}", (unsigned int)*(u8*)ptr);
            break;
        case TYPE_S16:
            element->prefix = fmt::format("{:d}", *(s16*)ptr);
            break;
        case TYPE_U16:
            element->prefix = fmt::format("{:d}", *(u16*)ptr);
            break;
        case TYPE_S32:
            element->prefix = fmt::format("{:d}", *(s32*)ptr);
            break;
        case TYPE_U32:
            element->prefix = fmt::format("{:d}", *(u32*)ptr);
            break;
        case TYPE_CHAR:
            element->prefix = fmt::format("{:c}", *(char*)ptr);
            break;
        case TYPE_STRING:
            element->prefix = fmt::format("{:s}", (char*)ptr);
            break;
        case TYPE_FLOAT:
            element->prefix = fmt::format("{:f}", *(float*)ptr);
            break;
    }
}

extern "C" void ValueViewer_Draw(GfxPrint* printer) {
    if (!CVAR_VALUE) {
        return;
    }

    for (auto& element : valueTable) {
        if (element.isPrinted) {
            ValueViewer_DrawElement(&element);
            GfxPrint_SetPos(printer, element.y, element.x);
            GfxPrint_SetColor(printer, element.color.x * 255, element.color.y * 255, element.color.z * 255,
                              element.color.w * 255);

            if (element.typeFormat) {
                void* ptr = element.valueFn();
                if (ptr == nullptr) {
                    continue;
                }

                switch (element.type) {
                    case TYPE_S8:
                        GfxPrint_Printf(printer, "%s%hhd", element.name, *(s8*)ptr);
                        break;
                    case TYPE_U8:
                        GfxPrint_Printf(printer, "%s%hhu", element.name, *(u8*)ptr);
                        break;
                    case TYPE_S16:
                        GfxPrint_Printf(printer, "%s%hd", element.name, *(s16*)ptr);
                        break;
                    case TYPE_U16:
                        GfxPrint_Printf(printer, "%s%hu", element.name, *(u16*)ptr);
                        break;
                    case TYPE_S32:
                        GfxPrint_Printf(printer, "%s%d", element.name, *(s32*)ptr);
                        break;
                    case TYPE_U32:
                        GfxPrint_Printf(printer, "%s%u", element.name, *(u32*)ptr);
                        break;
                    case TYPE_CHAR:
                        GfxPrint_Printf(printer, "%s%c", element.name, *(char*)ptr);
                        break;
                    case TYPE_STRING:
                        GfxPrint_Printf(printer, "%s%s", element.name, (char*)ptr);
                        break;
                    case TYPE_FLOAT:
                        GfxPrint_Printf(printer, "%s%f", element.name, *(float*)ptr);
                        break;
                }
            } else {
                GfxPrint_Printf(printer, "%s%s", element.name, element.prefix.c_str());
            }
        }
    }
}

extern "C" void ValueViewer_SetupDraw() {
    if (!CVAR_VALUE || gPlayState == nullptr) {
        return;
    }

    OPEN_DISPS(gPlayState->state.gfxCtx);

    OPEN_PRINTER(OVERLAY_DISP);
    ValueViewer_Draw(&printer);
    CLOSE_PRINTER(printer, OVERLAY_DISP);

    CLOSE_DISPS(gPlayState->state.gfxCtx);
}

void RegisterValueViewerHooks() {
    COND_HOOK(OnGameStateUpdate, CVAR_VALUE, []() { ValueViewer_SetupDraw(); });
}

RegisterShipInitFunc initFunc(RegisterValueViewerHooks, { CVAR_NAME });

void ValueViewerWindow::DrawElement() {
    ImGui::BeginDisabled(CVarGetInteger("gSettings.DisableChanges", 0));
    UIWidgets::CVarCheckbox("Enable Printing", CVAR_NAME, UIWidgets::CheckboxOptions().Color(THEME_COLOR));

    ImGui::SeparatorText("Value List");

    static int selectedIndex = -1;
    static ValueTableElement editElement;
    static bool isEditing = false;

    if (ImGui::BeginTable("ValueTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Active");
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Value");
        ImGui::TableSetupColumn("Position");
        ImGui::TableSetupColumn("Actions");
        ImGui::TableSetupColumn("Delete");
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < valueTable.size(); i++) {
            ImGui::TableNextRow();
            ValueTableElement& element = valueTable[i];

            ImGui::TableSetColumnIndex(0);
            ImGui::PushID(i);
            ImGui::Checkbox("##Active", &element.isPrinted);
            ImGui::PopID();

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", element.name);

            ImGui::TableSetColumnIndex(2);
            if (element.requiresPlayState && gPlayState == nullptr) {
                ImGui::TextDisabled("(Requires PlayState)");
            } else if (element.valueFn() == nullptr) {
                ImGui::TextDisabled("(Invalid)");
            } else {
                ValueViewer_DrawElement(&element);
                ImGui::Text("%s", element.prefix.c_str());
            }

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("(%d, %d)", element.x, element.y);

            ImGui::TableSetColumnIndex(4);
            ImGui::PushID(i);
            if (ImGui::Button("Edit")) {
                selectedIndex = i;
                editElement = element;
                isEditing = true;
            }
            ImGui::PopID();

            ImGui::TableSetColumnIndex(5);
            ImGui::PushID(i);
            if (ImGui::Button("X")) {
                valueTable.erase(valueTable.begin() + i);
                if (selectedIndex == i) {
                    selectedIndex = -1;
                    isEditing = false;
                }
                i--;
            }
            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    ImGui::Separator();

    if (ImGui::Button("Add New Value")) {
        selectedIndex = -1;
        isEditing = true;
        editElement = {};
        editElement.name = "New Value";
        editElement.path = "";
        editElement.prefix = "";
        editElement.type = TYPE_S32;
        editElement.requiresPlayState = false;
        editElement.valueFn = nullptr;
        editElement.color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        editElement.isActive = true;
        editElement.isPrinted = true;
        editElement.typeFormat = false;
        editElement.x = 0;
        editElement.y = 0;
    }

    if (isEditing) {
        ImGui::Separator();
        ImGui::Text("%s", selectedIndex >= 0 ? "Edit Value" : "Add Value");

        static char nameBuf[256] = "";
        static char pathBuf[512] = "";
        static char prefixBuf[256] = "";

        if (ImGui::IsWindowAppearing()) {
            snprintf(nameBuf, sizeof(nameBuf), "%s", editElement.name);
            snprintf(pathBuf, sizeof(pathBuf), "%s", editElement.path);
            snprintf(prefixBuf, sizeof(prefixBuf), "%s", editElement.prefix.c_str());
        }

        ImGui::InputText("Name", nameBuf, sizeof(nameBuf));
        ImGui::InputText("Path (unused)", pathBuf, sizeof(pathBuf));
        ImGui::InputText("Prefix", prefixBuf, sizeof(prefixBuf));

        const char* typeNames[] = { "S8", "U8", "S16", "U16", "S32", "U32", "CHAR", "STRING", "FLOAT" };
        int typeIndex = (int)editElement.type;
        if (ImGui::Combo("Type", &typeIndex, typeNames, IM_ARRAYSIZE(typeNames))) {
            editElement.type = (ValueType)typeIndex;
        }

        ImGui::Checkbox("Requires PlayState", &editElement.requiresPlayState);
        ImGui::Checkbox("Type Format", &editElement.typeFormat);
        ImGui::ColorEdit4("Color", (float*)&editElement.color);
        ImGui::InputScalar("X Position", ImGuiDataType_U32, &editElement.x);
        ImGui::InputScalar("Y Position", ImGuiDataType_U32, &editElement.y);

        ImGui::Separator();
        ImGui::TextWrapped(
            "Note: This UI cannot set function pointers. Values must be registered programmatically to work.");

        if (ImGui::Button("Save")) {
            editElement.name = nameBuf;
            editElement.path = pathBuf;
            editElement.prefix = prefixBuf;

            if (selectedIndex >= 0 && selectedIndex < valueTable.size()) {
                // Preserve the function pointer
                editElement.valueFn = valueTable[selectedIndex].valueFn;
                valueTable[selectedIndex] = editElement;
            } else {
                valueTable.push_back(editElement);
            }
            isEditing = false;
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            isEditing = false;
        }
    }

    ImGui::EndDisabled();
}

void ValueViewerWindow::InitElement() {
    // Initialize any necessary state here
}

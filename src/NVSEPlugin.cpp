#include <NVSE/PluginAPI.h>
#include <reshade/reshade.hpp>

using namespace reshade::api;

HMODULE m_hModule = nullptr;

static effect_runtime* m_runtime = nullptr;
static command_list*   m_cmdlist = nullptr;
static resource_view   m_rtv;
static resource_view   m_rtv_srgb;

struct __declspec(uuid("7251932A-ADAF-4DFC-B5CB-9A4E8CD5D6EB")) device_data
{
	effect_runtime* main_runtime = nullptr;
};

bool                     validPass = false;
std::vector<std::string> backupTechniques;

void on_reshade_begin_effects(effect_runtime* runtime, command_list* cmd_list, resource_view rtv, resource_view rtv_srgb)
{
	m_runtime = runtime;
	m_cmdlist = cmd_list;
	m_rtv = rtv;
	m_rtv_srgb = rtv_srgb;
	backupTechniques.clear();
	if (!validPass) {
		m_runtime->enumerate_techniques(nullptr, [&](reshade::api::effect_runtime* runtime, reshade::api::effect_technique technique) {
			char buffer[256];
			runtime->get_technique_name(technique, buffer);
			std::string name = buffer;
			if (runtime->get_technique_state(technique)) {
				backupTechniques.push_back(name);
				runtime->set_technique_state(technique, false);
			}
		});
	}
}

void on_reshade_finish_effects(effect_runtime* runtime, command_list* cmd_list, resource_view rtv, resource_view rtv_srgb)
{
	if (!validPass) {
		m_runtime->enumerate_techniques(nullptr, [&](reshade::api::effect_runtime* runtime, reshade::api::effect_technique technique) {
			char buffer[256];
			runtime->get_technique_name(technique, buffer);
			std::string name = buffer;
			runtime->set_technique_state(technique, std::find(backupTechniques.begin(), backupTechniques.end(), name) != backupTechniques.end());
		});
	} else {
		validPass = false;
	}
}

reshade::api::resource_view rtv;

void on_bind_render_targets_and_depth_stencil(reshade::api::command_list* cmd_list, uint32_t count, const reshade::api::resource_view* rtvs, reshade::api::resource_view dsv)
{
	rtv = rtvs[0];
}


void register_addon_events()
{
	reshade::register_event<reshade::addon_event::reshade_begin_effects>(on_reshade_begin_effects);
	reshade::register_event<reshade::addon_event::reshade_finish_effects>(on_reshade_finish_effects);
	reshade::register_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(on_bind_render_targets_and_depth_stencil);
}



extern "C" __declspec(dllexport) const char* NAME = "FNV ReShade Helper";
extern "C" __declspec(dllexport) const char* DESCRIPTION = "Renders ReShade effects before the UI, by Wall_SoGB and doodlez";

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID)
{
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		m_hModule = hModule;
		break;
	}
	return TRUE;
}

UInt32 base_DrawWorld = 0x875FD0;
UInt32 func_ProcessImageSpaceShaders = 0x559450;

float GetWorldspaceID()
{
	if (auto player = PlayerCharacter::GetSingleton()) {
		if (player->parentCell && player->parentCell->worldSpace) {
			return player->parentCell->worldSpace->refID & 0x00FFFFFF;
		}
	}
	return 0;
}

float GetInteriorID()
{
	if (auto player = PlayerCharacter::GetSingleton()) {
		if (player->parentCell && player->parentCell->IsInterior()) {
			return player->parentCell->refID & 0x00FFFFFF;
		}
	}
	return 0;
}

void RenderEffects()
	{
	if (m_runtime) {
		m_runtime->enumerate_uniform_variables(nullptr, [&](reshade::api::effect_runtime* runtime, reshade::api::effect_uniform_variable variable) {
			char annotation_value[128];
			if (runtime->get_annotation_string_from_uniform_variable(variable, "source", annotation_value)) {
				std::string annotation = annotation_value;

				if (annotation == "ModLoaded") {
					runtime->set_uniform_value_bool(variable, true);
				}

				if (annotation == "Weather") {
					if (auto tes = TES::GetSingleton()) {
						if (auto sky = tes->sky) {
							runtime->set_uniform_value_float(variable,
								(float)(sky->currWeather->refID & 0x00FFFFFF),
								(float)(sky->transWeather->refID & 0x00FFFFFF),
								sky->weatherPercent,
								sky->gameHour);
						}
					}
				}

				if (auto player = PlayerCharacter::GetSingleton()) {
					if (annotation == "WorldspaceID") {
						runtime->set_uniform_value_int(variable, GetWorldspaceID());
					} else if (annotation == "InteriorID") {
						runtime->set_uniform_value_int(variable, GetInteriorID());
					}
				}
			}
		});

		validPass = true;
		m_runtime->render_effects(m_cmdlist, rtv);
	}
}

class BSRenderedTexture;

void hk_ProcessImageSpaceShaders(NiDX9Renderer* Renderer, BSRenderedTexture* RenderedTexture1, BSRenderedTexture* RenderedTexture2)
{
	StdCall(func_ProcessImageSpaceShaders, Renderer, RenderedTexture1, RenderedTexture2);
	if (!RenderedTexture2) {
		RenderEffects();
	}
}

void Load()
{
	if (reshade::register_addon(m_hModule)) {
		PrintLog("Registered addon");
		register_addon_events();
		// Pre-UI Hook for rendering effects
		WriteRelCall(base_DrawWorld + 0x173, (UInt32)&hk_ProcessImageSpaceShaders);
		PrintLog("Installed render hook");
	} else {
		PrintLog("ReShade not present, not installing hook");
	}
}

void Init(const NVSEInterface* nvse)
{
	if (!nvse->isEditor)
		Load();
}

extern "C" DLLEXPORT bool NVSEAPI NVSEPlugin_Load(const NVSEInterface* nvse)
{
#ifndef NDEBUG
	while (!IsDebuggerPresent()) {};
#endif
	s_log().Create(fmt::format("{}.log", Plugin::NAME).data());
	PrintLog("Loaded plugin");

	Init(nvse);

	return true;
}

extern "C" DLLEXPORT bool NVSEAPI NVSEPlugin_Query(const NVSEInterface* nvse, PluginInfo* info)
{
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = Plugin::NAME;
	return true;
}
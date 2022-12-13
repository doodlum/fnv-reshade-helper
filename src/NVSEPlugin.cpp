#include <NVSE/PluginAPI.h>
#include <reshade/reshade.hpp>

#include <detours.h>

using namespace reshade;

HMODULE m_hModule = nullptr;

api::effect_runtime*     m_runtime = nullptr;
api::command_list*       m_cmdlist = nullptr;
api::resource_view       m_rtv;
api::resource_view       m_rtv_srgb;
api::resource_view       true_rtv;
bool                     validPass = false;
std::vector<std::string> backupTechniques;

struct __declspec(uuid("7251932A-ADAF-4DFC-B5CB-9A4E8CD5D6EB")) device_data
{
	api::effect_runtime* main_runtime = nullptr;
};

void on_reshade_begin_effects(api::effect_runtime* runtime, api::command_list* cmd_list, api::resource_view rtv, api::resource_view rtv_srgb)
{
	m_runtime = runtime;
	m_cmdlist = cmd_list;
	m_rtv = rtv;
	m_rtv_srgb = rtv_srgb;
	backupTechniques.clear();
	if (!validPass) {
		m_runtime->enumerate_techniques(nullptr, [&](api::effect_runtime* runtime, api::effect_technique technique) {
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

void on_reshade_finish_effects(api::effect_runtime* runtime, api::command_list* cmd_list, api::resource_view rtv, api::resource_view rtv_srgb)
{
	if (!validPass) {
		m_runtime->enumerate_techniques(nullptr, [&](api::effect_runtime* runtime, api::effect_technique technique) {
			char buffer[256];
			runtime->get_technique_name(technique, buffer);
			std::string name = buffer;
			runtime->set_technique_state(technique, std::find(backupTechniques.begin(), backupTechniques.end(), name) != backupTechniques.end());
		});
	} else {
		validPass = false;
	}
}

void on_bind_render_targets_and_depth_stencil(api::command_list* cmd_list, uint32_t count, const api::resource_view* rtvs, api::resource_view dsv)
{
	true_rtv = rtvs[0];
}

void register_addon_events()
{
	register_event<addon_event::reshade_begin_effects>(on_reshade_begin_effects);
	register_event<addon_event::reshade_finish_effects>(on_reshade_finish_effects);
	register_event<addon_event::bind_render_targets_and_depth_stencil>(on_bind_render_targets_and_depth_stencil);
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
		m_runtime->enumerate_uniform_variables(nullptr, [&](api::effect_runtime* runtime, api::effect_uniform_variable variable) {
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
		m_runtime->render_effects(m_cmdlist, true_rtv);
	}
}

class BSRenderedTexture;

void(__cdecl* ProcessImageSpaceShaders)(NiDX9Renderer*, BSRenderedTexture*, BSRenderedTexture*) = (void(__cdecl*)(NiDX9Renderer*, BSRenderedTexture*, BSRenderedTexture*))0x00B55AC0;
void __cdecl ProcessImageSpaceShadersHook(NiDX9Renderer* Renderer, BSRenderedTexture* RenderedTexture1, BSRenderedTexture* RenderedTexture2)
{
	ProcessImageSpaceShaders(Renderer, RenderedTexture1, RenderedTexture2);
	if (!RenderedTexture2) {
		RenderEffects();
	}
}

void Load()
{
	if (register_addon(m_hModule)) {
		PrintLog("Registered addon");
		register_addon_events();
		// Pre-UI Hook for rendering effects
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(&(PVOID&)ProcessImageSpaceShaders, &ProcessImageSpaceShadersHook);
		DetourTransactionCommit();
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

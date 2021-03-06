#include "Async SaveWorld.h"
#include "Async SaveWorldConfig.h"
#include "Async SaveWorldHooks.h"
#pragma comment(lib, "ArkApi.lib")

void Init()
{
	Log::Get().Init("Async SaveWorld");
	InitConfig();
	InitHooks();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Init();
		break;
	case DLL_PROCESS_DETACH:
		RemoveHooks();
		break;
	}
	return TRUE;
}
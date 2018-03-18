#include "EventManager.h"
#pragma comment(lib, "ArkApi.lib")
DECLARE_HOOK(APrimalStructure_IsAllowedToBuild, int, APrimalStructure*, APlayerController*, FVector, FRotator, FPlacementData*, bool, FRotator, bool);
DECLARE_HOOK(APrimalCharacter_TakeDamage, float, APrimalCharacter*, float, FDamageEvent*, AController*, AActor*);
DECLARE_HOOK(APrimalStructure_TakeDamage, float, APrimalStructure*, float, FDamageEvent*, AController*, AActor*);
DECLARE_HOOK(AShooterCharacter_Die, bool, AShooterCharacter*, float, FDamageEvent*, AController*, AActor*);
DECLARE_HOOK(AShooterGameMode_Logout, void, AShooterGameMode*, AController*);

void EventManagerUpdate()
{
	if (!ArkApi::GetApiUtils().GetShooterGameMode()) return;
	EventManager::Get().Update();
}

int _cdecl Hook_APrimalStructure_IsAllowedToBuild(APrimalStructure* _this, APlayerController* PC, FVector AtLocation, FRotator AtRotation
	, FPlacementData * OutPlacementData, bool bDontAdjustForMaxRange, FRotator PlayerViewRotation, bool bFinalPlacement)
{
	return APrimalStructure_IsAllowedToBuild_original(_this, PC, AtLocation, AtRotation, OutPlacementData, bDontAdjustForMaxRange
		, PlayerViewRotation, bFinalPlacement);
}

const long long GetPlayerID(APrimalCharacter* _this)
{
	AShooterCharacter* shooterCharacter = static_cast<AShooterCharacter*>(_this);
	return (shooterCharacter && shooterCharacter->GetPlayerData()) ? shooterCharacter->GetPlayerData()->MyDataField()()->PlayerDataIDField()() : -1;
}

const long long GetPlayerID(AController* _this)
{
	AShooterPlayerController* Player = static_cast<AShooterPlayerController*>(_this);
	return Player ? Player->LinkedPlayerIDField()() : 0;
}

float _cdecl Hook_APrimalCharacter_TakeDamage(APrimalCharacter* _this, float Damage, FDamageEvent* DamageEvent, AController* EventInstigator
	, AActor* DamageCauser)
{
	return (EventManager::Get().IsEventRunning() && EventInstigator && _this && !EventInstigator->IsLocalController()
		&& _this->IsA(AShooterCharacter::GetPrivateStaticClass()) ? (EventManager::Get().CanTakeDamage(GetPlayerID(EventInstigator), GetPlayerID(_this))
			? APrimalCharacter_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser) : 0)
		: APrimalCharacter_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser));
}

float _cdecl Hook_APrimalStructure_TakeDamage(APrimalStructure* _this, float Damage, FDamageEvent* DamageEvent, AController* EventInstigator
	, AActor* DamageCauser)
{
	return EventManager::Get().IsEventProtectedStructure(_this->RootComponentField()() ? _this->RootComponentField()()->RelativeLocationField()()
		: FVector(0, 0, 0)) ? 0 : APrimalStructure_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);
}

bool _cdecl Hook_AShooterCharacter_Die(AShooterCharacter* _this, float KillingDamage, FDamageEvent* DamageEvent, AController* Killer, AActor* DamageCauser)
{
	return (EventManager::Get().IsEventRunning() && Killer && _this && !Killer->IsLocalController() && Killer->IsA(AShooterCharacter::GetPrivateStaticClass())
		&& EventManager::Get().OnPlayerDied(GetPlayerID(Killer), GetPlayerID(_this))) ? 0 : AShooterCharacter_Die_original(_this, KillingDamage, DamageEvent, Killer, DamageCauser);
}

void _cdecl Hook_AShooterGameMode_Logout(AShooterGameMode* _this, AController* Exiting)
{
	if (EventManager::Get().IsEventRunning() && Exiting && Exiting->IsA(AShooterPlayerController::StaticClass()))
	{
		AShooterPlayerController* Player = static_cast<AShooterPlayerController*>(Exiting);
		if (Player)	EventManager::Get().OnPlayerLogg(Player);
	}
	AShooterGameMode_Logout_original(_this, Exiting);
}


void JoinEvent(AShooterPlayerController* player, FString* message, int mode)
{
	if (!player || !player->PlayerStateField()() || !player->GetPlayerCharacter()) return;
	if (EventManager::Get().IsEventRunning() && !EventManager::Get().IsEventOverrideJoinAndLeave() && EventManager::Get().GetEventState() == EventState::WaitingForPlayers)
	{
		if (EventManager::Get().AddPlayer(player))
		{
			if (EventManager::Get().GetEventQueueNotifications()) ArkApi::GetApiUtils().SendChatMessageToAll(EventManager::Get().GetServerName(), L"{} has joined {} event queue!", *ArkApi::GetApiUtils().GetCharacterName(player), *EventManager::Get().GetCurrentEventName());
			ArkApi::GetApiUtils().SendChatMessage(player, EventManager::Get().GetServerName(), L"To leave the queue type /leave");
		} else ArkApi::GetApiUtils().SendChatMessage(player, EventManager::Get().GetServerName(), L"you are already in the {} event queue!", *EventManager::Get().GetCurrentEventName());
	}
}


void LeaveEvent(AShooterPlayerController* player, FString* message, int mode)
{
	if (!player || !player->PlayerStateField()() || !player->GetPlayerCharacter()) return;
	if (EventManager::Get().IsEventRunning() && !EventManager::Get().IsEventOverrideJoinAndLeave() && EventManager::Get().GetEventState() == EventState::WaitingForPlayers)
	{
		if (EventManager::Get().RemovePlayer(player))
		{
			if (EventManager::Get().GetEventQueueNotifications()) ArkApi::GetApiUtils().SendChatMessageToAll(EventManager::Get().GetServerName(), L"{} has left {} event queue!", *ArkApi::GetApiUtils().GetCharacterName(player), *EventManager::Get().GetCurrentEventName());
			ArkApi::GetApiUtils().SendChatMessage(player, EventManager::Get().GetServerName(), L"You left {} event queue!", *EventManager::Get().GetCurrentEventName());
		} else ArkApi::GetApiUtils().SendChatMessage(player, EventManager::Get().GetServerName(), L"you are not in the {} event queue!", *EventManager::Get().GetCurrentEventName());
	}
}

void InitEventManager()
{
	Log::Get().Init("Event Manager");
	ArkApi::GetHooks().SetHook("APrimalStructure.IsAllowedToBuild", &Hook_APrimalStructure_IsAllowedToBuild, &APrimalStructure_IsAllowedToBuild_original);
	ArkApi::GetHooks().SetHook("APrimalCharacter.TakeDamage", &Hook_APrimalCharacter_TakeDamage, &APrimalCharacter_TakeDamage_original);
	ArkApi::GetHooks().SetHook("APrimalStructure.TakeDamage", &Hook_APrimalStructure_TakeDamage, &APrimalStructure_TakeDamage_original);
	ArkApi::GetHooks().SetHook("AShooterCharacter.Die", &Hook_AShooterCharacter_Die, &AShooterCharacter_Die_original);
	ArkApi::GetHooks().SetHook("AShooterGameMode.Logout", &Hook_AShooterGameMode_Logout, &AShooterGameMode_Logout_original);
	ArkApi::GetCommands().AddOnTimerCallback("EventManagerUpdate", &EventManagerUpdate);
	ArkApi::GetCommands().AddChatCommand("/join", &JoinEvent);
	ArkApi::GetCommands().AddChatCommand("/leave", &LeaveEvent);
}

void DestroyEventManager()
{
	ArkApi::GetHooks().DisableHook("APrimalStructure.IsAllowedToBuild", &Hook_APrimalStructure_IsAllowedToBuild);
	ArkApi::GetHooks().DisableHook("APrimalCharacter.TakeDamage", &Hook_APrimalCharacter_TakeDamage);
	ArkApi::GetHooks().DisableHook("APrimalStructure.TakeDamage", &Hook_APrimalStructure_TakeDamage);
	ArkApi::GetHooks().DisableHook("AShooterCharacter.Die", &Hook_AShooterCharacter_Die);
	ArkApi::GetHooks().DisableHook("AShooterGameMode.Logout", &Hook_AShooterGameMode_Logout);
	ArkApi::GetCommands().RemoveOnTimerCallback("EventManagerUpdate");
	ArkApi::GetCommands().RemoveChatCommand("/join");
	ArkApi::GetCommands().RemoveChatCommand("/leave");
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		InitEventManager();
		break;
	case DLL_PROCESS_DETACH:
		DestroyEventManager();
		break;
	}
	return TRUE;
}
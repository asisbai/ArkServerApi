#pragma once
#include "DeathmatchEvent.h"
#include "../../EventManager/Public/Event.h"
#include "../../EventManager/Public/IEventManager.h"
#pragma comment(lib, "AZEventManager.lib")
#pragma comment(lib, "ArkApi.lib")
#include <fstream>
#include "json.hpp"

class DeathMatch : public Event
{
private:
	bool Notifications;
	int ArkShopPointsRewardMin, ArkShopPointsRewardMax, JoinMessages, JoinMessageDelaySeconds, PlayersNeededToStart, WaitForDelay, WaitCounter, LastEquipmentIndex = -1;
	FString JoinEventCommand, ServerName, Messages[11];

	struct Reward
	{
		const FString BP;
		const int QuantityMin, QuantityMax, QualityMin, QualityMax, MinIsBP, MaxIsBP;
		const float DMG, Armour, Dura, AmmoClip;
		Reward(const FString BP, const int QuantityMin, const int QuantityMax, const int QualityMin, const int QualityMax, const int MinIsBP, const int MaxIsBP, const float DMG, const float Armour, const float Dura, const float AmmoClip) : BP(BP), QuantityMin(QuantityMin)
			, QuantityMax(QuantityMax), QualityMin(QualityMin), QualityMax(QualityMax), MinIsBP(MinIsBP), MaxIsBP(MaxIsBP), DMG(DMG), Armour(Armour), Dura(Dura), AmmoClip(AmmoClip) {}
	};

	TArray<Reward> Rewards;
	TArray<EventManager::EventEquipment> Equipments;
public:
	virtual void InitConfig(const FString& JoinEventCommand, const FString& ServerName, const FString& Map)
	{
		this->JoinEventCommand = JoinEventCommand;
		this->ServerName = ServerName;
		try
		{
			if (!HasConfigLoaded())
			{
				Equipments.Empty();
				Rewards.Empty();
				ClearSpawns();
				const std::string config_path = ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/DeathMatchEvent/" + Map.ToString() + ".json";
				std::ifstream file{ config_path };
				if (!file.is_open())
				{
					Log::GetLog()->error("Can't find {}.json", Map.ToString().c_str());
					return;
				}
				nlohmann::json config;
				file >> config;
				const std::string eName = config["Deathmatch"]["EventName"];
				const FString& EventName = ArkApi::Tools::Utf8Decode(eName).c_str();

				PlayersNeededToStart = config["Deathmatch"]["PlayersNeededToStart"];

				JoinMessages = config["Deathmatch"]["JoinMessages"];
				JoinMessageDelaySeconds = config["Deathmatch"]["JoinMessageDelaySeconds"];


				const bool StructureProtection = config["Deathmatch"]["StructureProtection"];
				const auto StructureProtectionPosition = config["Deathmatch"]["StructureProtectionPosition"];
				const int StructureProtectionDistacne = config["Deathmatch"]["StructureProtectionDistance"];

				Notifications = config["Deathmatch"]["TopNotifications"];
				const float MovementSpeedAddon = config["Deathmatch"]["MovementSpeedAddon"];
				const int ArkShopPointsEntryFee = config["Deathmatch"]["ArkShopPointsEntryFee"];

				ArkShopPointsRewardMin = config["Deathmatch"]["ArkShopPointsRewardMin"];
				ArkShopPointsRewardMax = config["Deathmatch"]["ArkShopPointsRewardMax"];
				if (ArkShopPointsRewardMin > ArkShopPointsRewardMax) ArkShopPointsRewardMax = ArkShopPointsRewardMin;
				if (ArkShopPointsRewardMin < 0) ArkShopPointsRewardMin = 0;

				const bool KillOnLogg = config["Deathmatch"]["KillOnLoggout"];
				std::string Data;

				InitDefaults(EventName, false, true, KillOnLogg, StructureProtection
					, FVector(StructureProtectionPosition[0], StructureProtectionPosition[1], StructureProtectionPosition[2]), StructureProtectionDistacne, MovementSpeedAddon, ArkShopPointsEntryFee, PlayersNeededToStart);

				const auto& Spawns = config["Deathmatch"]["Spawns"];
				for (const auto& Spawn : Spawns)
				{
					const auto& SpawnPos = Spawn["Position"];
					AddSpawn(FVector(SpawnPos[0], SpawnPos[1], SpawnPos[2]));
				}

				const auto& RewardsConfig = config["Deathmatch"]["Rewards"];
				for (const auto& szitem : RewardsConfig)
				{
					Data = szitem["Blueprint"];
					Rewards.Add(Reward(FString(Data.c_str()), (int)szitem["QuantityMin"], (int)szitem["QuantityMax"], (int)szitem["QualityMin"], (int)szitem["QualityMax"], (int)szitem["MinIsBP"], (int)szitem["MaxIsBP"],
						szitem.value("ExtraDmgPercent", 0.f), szitem.value("ExtraArmourPercent", 0.f), szitem.value("ExtraDurabilityPercent", 0.f), szitem.value("ExtraAmmoPercent", 0.f)));
				}

				const auto& EquipmentConfig = config["Deathmatch"]["Equipment"];
				for (const auto& Equipment : EquipmentConfig)
				{
					TArray<EventManager::EventItem> Items;
					EventManager::EventItem Armour[EventManager::EventArmourType::Max];
					
					const float ExtraArmour = Equipment.value("ExtraArmourPercent", 0.f), ExtraDura =  Equipment.value("ExtraArmourDurabilityPercent", 0.f);

					Data = Equipment["HeadBP"];
					Armour[EventManager::EventArmourType::Head] = EventManager::EventItem(Data.c_str(), 1, 0, 0, 
						ExtraArmour, ExtraDura, 0);

					Data = Equipment["TorsoBP"];
					Armour[EventManager::EventArmourType::Torso] = EventManager::EventItem(Data.c_str(), 1, 0, 0,
						ExtraArmour, ExtraDura, 0);

					Data = Equipment["GlovesBP"];
					Armour[EventManager::EventArmourType::Gloves] = EventManager::EventItem(Data.c_str(), 1, 0, 0,
						ExtraArmour, ExtraDura, 0);

					Data = Equipment["OffhandBP"];
					Armour[EventManager::EventArmourType::Offhand] = EventManager::EventItem(Data.c_str(), 1, 0, 0,
						ExtraArmour, ExtraDura, 0);

					Data = Equipment["LegsBP"];
					Armour[EventManager::EventArmourType::Legs] = EventManager::EventItem(Data.c_str(), 1, 0, 0,
						ExtraArmour, ExtraDura, 0);

					Data = Equipment["FeetBP"];
					Armour[EventManager::EventArmourType::Feet] = EventManager::EventItem(Data.c_str(), 1, 0, 0,
						ExtraArmour, ExtraDura, 0);

					const auto& ItemConfig = Equipment["Items"];
					for (const auto& Item : ItemConfig)
					{
						Data = Item["BP"];
						Items.Add(EventManager::EventItem(Data.c_str(), Item["Quantity"], Item["Quality"], Equipment.value("ExtraDmgPercent", 0.f), 0.f,
							Equipment.value("ExtraDurabilityPercent", 0.f), Equipment.value("ExtraAmmoPercent", 0.f)));
					}
					Equipments.Add(EventManager::EventEquipment(Items, Armour));
				}


				int j = 0;
				const auto& Msgs = config["Deathmatch"]["Messages"];
				for (const auto& Msg : Msgs)
				{
					Data = Msg;
					Messages[j++] = ArkApi::Tools::Utf8Decode(Data).c_str();
				}
				file.close();
			}
			Init(JoinMessageDelaySeconds + 1);
			WaitForDelay = JoinMessageDelaySeconds;
			WaitCounter = JoinMessages + 1;
		}
		catch (...)
		{
			Log::GetLog()->error("Config Error!!!");
		}
	}
	
	virtual void Update()
	{
		switch (GetState())
		{
		case EventState::WaitingForPlayers:
			if (WaitForTimer(WaitForDelay))
			{
				if (WaitForCounter(WaitCounter))
				{
					if (EventManager::Get().GetEventPlayersCount() < PlayersNeededToStart)
					{
						ArkApi::GetApiUtils().SendChatMessageToAll(ServerName, *Messages[4], *GetName(), PlayersNeededToStart);
						SetState(EventState::Finished);
					}
					else SetState(EventState::TeleportingPlayers);
				}
				else
				{
					const int Seconds = GetFinalWarning() ? WaitForDelay : ((WaitCounter - GetCounter()) * JoinMessageDelaySeconds);
					ArkApi::GetApiUtils().SendChatMessageToAll(ServerName, *Messages[0], *GetName()
						, (Seconds >= 60 ? (Seconds / 60) : Seconds), (Seconds >= 60 ? (Seconds >= 120 ? *Messages[1] : *Messages[2]) : *Messages[3]), *JoinEventCommand);

					if (!GetFinalWarning() && GetCounter() == (WaitCounter - 1))
					{
						WaitForDelay = (int)(JoinMessageDelaySeconds / 2);
						SetFinalWarning(true);
					}
				}
				ResetTimer();
			}
			break;
		case EventState::TeleportingPlayers:
			if(!EventManager::Get().TeleportEventPlayers(true, true, true, true, false, true, GetSpawns()))
			{
				ArkApi::GetApiUtils().SendChatMessageToAll(ServerName, *Messages[4], *GetName(), PlayersNeededToStart);
				SetState(EventState::Finished);
			}
			else
			{
				EventManager::Get().SendChatMessageToAllEventPlayers(ServerName, *Messages[10]);
				SetState(EventState::GiveEquipment);
			}
			break;
		case EventState::GiveEquipment:
			if (WaitForTimer(10))
			{
				ResetTimer();
				EventManager::Get().SendChatMessageToAllEventPlayers(ServerName, *Messages[5], *GetName());
				if (Equipments.Num() > 0)
					EventManager::Get().GiveEventPlayersEquipment(Equipments[EventManager::Get().GetRandomIndexNonRecurr(Equipments.Num() - 1)]);
				SetState(EventState::WaitForFight);
			}
			break;
		case EventState::WaitForFight:
			if(Notifications) EventManager::Get().SendNotificationToAllEventPlayers(FLinearColor(0, 1, 1), 1.f, 1, nullptr, *Messages[8]);
			if (WaitForTimer(30))
			{
				EventManager::Get().EnableInputs();
				EventManager::Get().SendChatMessageToAllEventPlayers(ServerName, *Messages[6], *GetName());
				SetState(EventState::Fighting);
			}
			break;
		case EventState::Fighting:
			{
				const int Players = EventManager::Get().GetEventPlayersCount();
				if (Notifications) EventManager::Get().SendNotificationToAllEventPlayers(FLinearColor(0, 1, 0), 1.5f, 1.f, nullptr, *Messages[9], *GetName(), Players);
				if (Players <= 1) SetState(EventState::Rewarding);
			}
			break;
		case EventState::Rewarding:
			if (EventManager::Get().GetEventPlayersCount() > 0)
			{
				EventManager::Get().TeleportWinningEventPlayersToStart(true);
				if (EventManager::Get().CanRewardWinner())
				{
					AShooterPlayerController* RewardPlayer = EventManager::Get().GetEventPlayers()[0].ASPC;
					if (RewardPlayer && RewardPlayer->GetPlayerCharacter())
					{
						if (Rewards.Num() != 0)
						{
							const Reward& reward = Rewards[FMath::RandRange(0, Rewards.Num() - 1)];
							const int RandomQuantity = (reward.QuantityMin == reward.QuantityMax ? reward.QuantityMin : FMath::RandRange(reward.QuantityMin, reward.QuantityMax))
								, RandomQuality = (reward.QualityMin == reward.QualityMax ? reward.QualityMin : FMath::RandRange(reward.QualityMin, reward.QualityMax));
							const bool IsBP = (reward.MinIsBP == reward.MaxIsBP && reward.MinIsBP == 0 ? false : (reward.MinIsBP == reward.MaxIsBP ? true
								: (FMath::RandRange(reward.MinIsBP, reward.MaxIsBP) == reward.MinIsBP)));
							FString BP = reward.BP;
							TArray<UPrimalItem*> SpawnedItems;
							RewardPlayer->GiveItem(&SpawnedItems, &BP, RandomQuantity, (float)RandomQuality, false, IsBP, 0);
							
							if ((reward.Dura > 0 || reward.AmmoClip > 0 || reward.DMG > 0 || reward.Dura > 0 || reward.Armour > 0) && SpawnedItems.Num() > 0)
							{
								EventManager::Get().SetItemStatValue(SpawnedItems[0], EPrimalItemStat::MaxDurability, reward.Dura);
								EventManager::Get().SetItemStatValue(SpawnedItems[0], EPrimalItemStat::WeaponClipAmmo, reward.AmmoClip);
								EventManager::Get().SetItemStatValue(SpawnedItems[0], EPrimalItemStat::WeaponDamagePercent, reward.DMG);
								EventManager::Get().SetItemStatValue(SpawnedItems[0], EPrimalItemStat::Armor, reward.Armour);
								EventManager::Get().SetItemStatValue(SpawnedItems[0], EPrimalItemStat::MaxDurability, reward.Dura);
								SpawnedItems[0]->UpdatedItem(false);
							}
						}

						if (ArkShopPointsRewardMax > 0)
						{
							int Amount = FMath::RandRange(ArkShopPointsRewardMin, ArkShopPointsRewardMax);
							Log::GetLog()->info("adding {} Points to winner!", Amount);
							EventManager::Get().ArkShopAddPoints(Amount, (int)RewardPlayer->LinkedPlayerIDField());
						}

						Log::GetLog()->info("{} won {} event!", ArkApi::GetApiUtils().GetCharacterName(RewardPlayer).ToString(), GetName().ToString());

						ArkApi::GetApiUtils().SendChatMessageToAll(ServerName, *Messages[7], *ArkApi::GetApiUtils().GetCharacterName(RewardPlayer), *GetName());
					}
				}
			}
			SetState(EventState::Finished);
			break;
		}
	}
}; 

DeathMatch* DmEvent;
void DMReload(APlayerController* player_controller, FString* message, bool LogToFile)
{
	AShooterPlayerController* player = static_cast<AShooterPlayerController*>(player_controller);
	if (!player || !player->PlayerStateField() || !player->GetPlayerCharacter()) return;
	DmEvent->ResetConfigLoaded();
	ArkApi::GetApiUtils().SendChatMessage(player, EventManager::Get().GetServerName(), "Config Reloaded!");
}

void InitEvent()
{
	Log::Get().Init("Deathmatch Event");
	DmEvent = new DeathMatch();
	EventManager::Get().AddEvent((Event*)DmEvent);
	ArkApi::GetCommands().AddConsoleCommand("dmreload", &DMReload);
}

void RemoveEvent()
{
	EventManager::Get().RemoveEvent((Event*)DmEvent);
	ArkApi::GetCommands().RemoveConsoleCommand("dmreload");
	if (DmEvent != nullptr) delete[] DmEvent;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		InitEvent();
		break;
	case DLL_PROCESS_DETACH:
		RemoveEvent();
		break;
	}
	return TRUE;
}
#include "Cheat.h"
#include <fstream>

cCheat* Cheat = new cCheat();

std::string cCheat::getNameFromIDmem(int ID) {
	try {
		DWORD_PTR fNamePtr = Mem->Read<uintptr_t>(GNames + int(ID / 0x4000) * 0x8);
		DWORD_PTR fName = Mem->Read<uintptr_t>(fNamePtr + 0x8 * int(ID % 0x4000));
		return Mem->Read<text>(fName + 0x10).word;
	}
	catch (int e)
	{
		return std::string("");
	}
}

std::string cCheat::getNameFromIDmap(int ID) 
{
	auto it = Names.find(ID);


	if (it == Names.end())
		return "";
	else
		return it->second;
}

Vector2 RotatePoint(Vector2 pointToRotate, Vector2 centerPoint, float angle, bool angleInRadians = false)
{
	if (!angleInRadians)
		angle = static_cast<float>(angle * (PI / 180.f));
	float cosTheta = static_cast<float>(cos(angle));
	float sinTheta = static_cast<float>(sin(angle));
	Vector2 returnVec = Vector2(cosTheta * (pointToRotate.x - centerPoint.x) - sinTheta * (pointToRotate.y - centerPoint.y), sinTheta * (pointToRotate.x - centerPoint.x) + cosTheta * (pointToRotate.y - centerPoint.y)
	);
	returnVec += centerPoint;
	return returnVec;
}

void cCheat::readData()
{
	if (!baseModule)
	{
		baseModule = Mem->Module("SoTGame.exe");
		baseSize = Mem->ModuleSize("SoTGame.exe");
	}

	uintptr_t address = 0;

	if (!UWorld)
	{
		address = Mem->FindSignature(baseModule, baseSize,
			(BYTE*)("\x48\x8B\x05\x00\x00\x00\x00\x48\x8B\x88\x00\x00\x00\x00\x48\x85\xC9\x74\x06\x48\x8B\x49\x70"),
			(char*)"xxx????xxx????xxxxxxxxx");

		auto uworldoffset = Mem->Read<int32_t>(address + 3);
		UWorld = address + uworldoffset + 7;
	}
	if (!GNames)
	{
		address = Mem->FindSignature(baseModule, baseSize, (BYTE*)"\x48\x8B\x1D\x00\x00\x00\x00\x48\x85\x00\x75\x3A", (char*)"xxx????xx?xx");
		auto gnamesoffset = Mem->Read<int32_t>(address + 3);
		GNames = Mem->Read<uintptr_t>(address + gnamesoffset + 7);
	}
	if (!GObjects)
	{
		address = Mem->FindSignature(baseModule, baseSize, (BYTE*)"\x48\x8B\x15\x00\x00\x00\x00\x3B\x42\x1C", (char*)"xxx????xxx");
		auto gobjectsoffset = Mem->Read<int32_t>(address + 3);
		auto offset = gobjectsoffset + 7;
		GObjects = Mem->Read<uintptr_t>(address + gobjectsoffset + 7);
	}

	if (Names.empty())
	{
		std::ofstream myfile;
		//myfile.open("Gnames.txt");

		for (int i = 0; i < 206000; i++)
		{

			auto temp = getNameFromIDmem(i);

			if (temp != "FAIL")
			{
				Names.insert(std::pair<int, std::string>(i, temp));
				//myfile << i << "  |  " << getNameFromIDmem(i) << "\n";
			}
		}

		//myfile.close();

	}
	auto world = Mem->Read<cUWorld>(Mem->Read<uintptr_t>(UWorld));
	//auto world = Mem->Read<cUWorld>(baseModule + 0x59B12D8);
	auto LocalPlayer = world.GetGameInstance().GetLocalPlayer();
	auto player_controller = LocalPlayer.GetPlayerController();
	auto CameraManager = player_controller.GetCameraManager();

	SOT->localPlayer.name = Misc->wstringToString(player_controller.GetActor().GetPlayerState().GetName());

	SOT->localPlayer.position = player_controller.GetActor().GetRootComponent().GetPosition();
	SOT->localCamera.fov = CameraManager.GetCameraFOV();
	SOT->localCamera.angles = CameraManager.GetCameraRotation();
	SOT->localCamera.position = CameraManager.GetCameraPosition();

	auto level = world.GetLevel();
	auto actors = level.GetActors();
	if (!actors.IsValid())
		return;

	for (int i = 0; i < actors.Length(); ++i)
	{

		auto actor = *reinterpret_cast<AActor*>(&actors[i]);
		if (actor == player_controller.GetActor())
			continue;

		auto id = actor.GetID();
		auto name = getNameFromIDmem(id);

		

		if (name.find("BP_Skeleton") != std::string::npos && name.find("Pawn") != std::string::npos)
		{
			if (!Vars.ESP.Skeleton.bActive)
				continue;

			auto pos = actor.GetRootComponent().GetPosition();
			auto distance = SOT->localCamera.position.DistTo(pos) / 100.00f;	

			Vector2 Screen;
			if (Misc->WorldToScreen(pos, &Screen))
			{
				Vector3 headPos = Vector3(pos.x, pos.y, pos.z + 100);

				Vector2 ScreenTop;
				Color boxColor = { Vars.ESP.Skeleton.colorSkeleton[0],Vars.ESP.Skeleton.colorSkeleton[1],Vars.ESP.Skeleton.colorSkeleton[2],Vars.ESP.Skeleton.colorSkeleton[3] };
				//Color boxColor = { 255,255,0,255 };

				if (Misc->WorldToScreen(headPos, &ScreenTop))
				{
					int hi = (Screen.y - ScreenTop.y) * 2;
					int wi = hi * 0.65;			

					DrawBox(ScreenTop.x - wi / 2, ScreenTop.y, wi, hi, boxColor);
					DrawString(std::string("Skeleton [ " + std::to_string((int)distance) + "m ]").c_str(), ScreenTop.x, ScreenTop.y - 14, Color{ 255,255,255 }, true, "small");
					if (Vars.ESP.Skeleton.bWeapon)
					{
						auto ItemName = actor.GetWieldedItemComponent().GetWieldedItem().GetItemInfo().GetItemDesc().GetName();

						if (ItemName.length() < 32)
							DrawString(ItemName.c_str(), ScreenTop.x, ScreenTop.y + hi, Color{ 255,255,255 }, true, "small");
					}

				}
			}
		}

		else if (name.find("IslandService") != std::string::npos)
		{
			if (!Vars.ESP.World.bIslands)
				continue;


			auto IslandService = *reinterpret_cast<AIslandService*>(&actors[i]);

			auto Islands = IslandService.GetIslandArray();
			if (!Islands.IsValid())
				continue;

			for (int p = 0; p < Islands.Length(); ++p)
			{
				auto Island = Islands[p];

				Color color = { Vars.ESP.World.colorWorld[0],Vars.ESP.World.colorWorld[1],Vars.ESP.World.colorWorld[2],Vars.ESP.World.colorWorld[3] };

				Vector2 Screen;
				if (Misc->WorldToScreen(Vector3(Island.IslandBoundsCentre.x, Island.IslandBoundsCentre.y, 20000), &Screen))
					DrawString(std::string(getNameFromIDmem(Island.IslandNameId)).c_str(), Screen.x, Screen.y, color, true, "default");

			}
		}
		
		else if (name.find("BP_SkellyShip_ShipCloud") != std::string::npos)
		{



			if (!Vars.ESP.World.bFort)
				continue;

			auto pos = actor.GetRootComponent().GetPosition();
			auto distance = SOT->localCamera.position.DistTo(pos) / 100.00f;

			Color color = { Vars.ESP.World.colorWorld[0],Vars.ESP.World.colorWorld[1],Vars.ESP.World.colorWorld[2],Vars.ESP.World.colorWorld[3] };

			Vector2 Screen;
			if (Misc->WorldToScreen(pos, &Screen))
				DrawString(std::string("Ship Cloud [ " + std::to_string((int)distance) + "m ]").c_str(), Screen.x, Screen.y, color, true, "default");

		}

		else if (name.find("BP_SkellyFort_SkullCloud_C") != std::string::npos)
		{
			if (!Vars.ESP.World.bFort)
				continue;

			auto pos = actor.GetRootComponent().GetPosition();
			auto distance = SOT->localCamera.position.DistTo(pos) / 100.00f;

			Color color = { Vars.ESP.World.colorWorld[0],Vars.ESP.World.colorWorld[1],Vars.ESP.World.colorWorld[2],Vars.ESP.World.colorWorld[3] };


			Vector2 Screen;
			if (Misc->WorldToScreen(pos, &Screen))
				DrawString(std::string("Skull Fort [ " + std::to_string((int)distance) + "m ]").c_str(), Screen.x, Screen.y, color, true, "default");

		}


		else if (name.find("MapTable_C") != std::string::npos)
		{
			auto Table = *reinterpret_cast<AMapTable*>(&actors[i]);


			auto Ships = Table.GetTrackedShips();

			if (Ships.IsValid())
			{

				for (int p = 0; p < Ships.Length(); ++p)
				{
					auto Ship = Ships[p];

					for (int c = 0; c < 6; ++c)
					{
						if (Ship.GetCrewId() == SOT->Ships[c].crewID)
						{
							int id = Ship.GetUObject().nameId;
							auto temp = getNameFromIDmem(id);
							SOT->Ships[c].type = getNameFromIDmem(Ship.GetUObject().nameId);
						}
					}

				}
			}

			auto Chests = Table.GetTrackedBootyItemLocations();

			if (Chests.IsValid())
			{

				for (int p = 0; p < Chests.Length(); ++p)
				{
					auto Chest = Chests[p];
					Vector2 Screen;

					Color color = { Vars.ESP.World.colorWorld[0],Vars.ESP.World.colorWorld[1],Vars.ESP.World.colorWorld[2],Vars.ESP.World.colorWorld[3] };

					int dist = (int)(SOT->localPlayer.position.DistTo(Chest) / 100.f);

					if (dist > 150)
					{
						if (Misc->WorldToScreen(Chest, &Screen))
							DrawString(std::string("Reapers Chest [" + std::to_string(dist) + "m] ").c_str(), Screen.x, Screen.y, color, true, "default");
					}
				}
			}

			if (!Vars.ESP.World.bMapPins)
				continue;

			auto pins = Table.GetMapPins();

			if (!pins.IsValid())
				continue;
		
			Color color = { Vars.ESP.World.colorWorld[0],Vars.ESP.World.colorWorld[1],Vars.ESP.World.colorWorld[2],Vars.ESP.World.colorWorld[3] };

			for (int p = 0; p < pins.Length(); ++p)
			{
				auto pin = pins[p];
				
				Vector3 worldPin = Vector3(pin.x * 100, pin.y * 100, 5000);

				Vector2 Screen;
				if (Misc->WorldToScreen(worldPin, &Screen))
					DrawString(std::string("PIN [" + std::to_string((int)(SOT->localPlayer.position.DistTo(worldPin) / 100.f)) + "m] ").c_str(), Screen.x, Screen.y, color, true, "default");

			}

		}

		else if (name.find("CrewService") != std::string::npos)
		{
			Crews.clear();

			Team tempTeam;
			TeamMate tempPlayers;
			auto crewService = *reinterpret_cast<ACrewService*>(&actors[i]);;
			auto crews = crewService.GetCrews();
			if (!crews.IsValid())
				continue;

			for (int c = 0; c < crews.Length(); ++c)
			{
				auto players = crews[c].GetPlayers();
				if (!players.IsValid())
					continue;

				tempTeam.teamName = crews[c].GetShipType();
				if (SOT->Ships[c].type.find("azure") != std::string::npos)
					tempTeam.color = Color(0, 255, 255);
				else if (SOT->Ships[c].type.find("regal") != std::string::npos)
					tempTeam.color = Color(255, 0, 255);
				else if (SOT->Ships[c].type.find("lucky") != std::string::npos)
					tempTeam.color = Color(0, 255, 0);
				else if (SOT->Ships[c].type.find("flaming") != std::string::npos)
					tempTeam.color = Color(255, 0, 0);
				else if (SOT->Ships[c].type.find("golden") != std::string::npos)
					tempTeam.color = Color(255, 255, 0);
				else
					tempTeam.color = Color(255, 255, 255);

				SOT->Ships[c].crewID = crews[c].GetCrewID();

				for (int p = 0; p < players.Length(); ++p)
				{
					tempPlayers.PlayerName = Misc->wstringToString(players[p].GetName());
					if (tempPlayers.PlayerName == SOT->localPlayer.name)
						SOT->localPlayer.crewID = crews[c].GetCrewID();

					SOT->Pirates[(c * 4) + p].crewID = crews[c].GetCrewID();
					SOT->Pirates[(c * 4) + p].name = tempPlayers.PlayerName;

					tempTeam.Players.push_back(tempPlayers);
				}
				Crews.push_back(tempTeam);
				tempTeam.Players.clear();
			}

		}

		else if (name.find("BP_PlayerPirate_C") != std::string::npos)
		{
			if (!Vars.ESP.Player.bActive)
				continue;

			auto pos = actor.GetRootComponent().GetPosition();
			auto distance = SOT->localCamera.position.DistTo(pos) / 100.00f;

			int health = actor.GetHealthComponent().GetHealth();
			if (health <= 0)
				continue;

			auto ItemName = actor.GetWieldedItemComponent().GetWieldedItem().GetItemInfo().GetItemDesc().GetName();

			Vector2 Screen;
			if (Misc->WorldToScreen(pos, &Screen))
			{
				Vector3 headPos = Vector3(pos.x, pos.y, pos.z + 100);

				Vector2 ScreenTop;
				Color boxColor = { Vars.ESP.Player.colorEnemy[0],Vars.ESP.Player.colorEnemy[1],Vars.ESP.Player.colorEnemy[2],Vars.ESP.Player.colorEnemy[3] };
				bool bTeammate = false;

				if (Misc->WorldToScreen(headPos, &ScreenTop))
				{
					int hi = (Screen.y - ScreenTop.y) * 2;
					int wi = hi * 0.65;

					auto pirateName = Misc->wstringToString(actor.GetPlayerState().GetName());
				
					for (int pirates = 0; pirates < 24; ++pirates)
					{
						if (SOT->Pirates[pirates].name == "")
							continue;

						if (pirateName == SOT->Pirates[pirates].name)
						{

							SOT->Pirates[pirates].distance = distance;
							if (SOT->Pirates[pirates].crewID == SOT->localPlayer.crewID)
							{
								boxColor = Color{ Vars.ESP.Player.colorTeam[0],Vars.ESP.Player.colorTeam[1],Vars.ESP.Player.colorTeam[2],Vars.ESP.Player.colorTeam[3] };
								bTeammate = true;
							}
						}
					}

					if (!Vars.ESP.Player.bTeam && bTeammate)
						continue;

						DrawBox(ScreenTop.x - wi / 2, ScreenTop.y, wi, hi, boxColor);
						if (Vars.ESP.Player.bName)
							DrawString(std::string(pirateName + " [ " + std::to_string((int)distance) + "m ]").c_str(), ScreenTop.x, ScreenTop.y - 14, Color{ 255,255,255 }, true, "small");
						if (Vars.ESP.Player.bWeapon)
						{
							if (ItemName.length() < 32)
								DrawString(ItemName.c_str(), ScreenTop.x, ScreenTop.y + hi, Color{ 255,255,255 }, true, "small");
						}
						if (Vars.ESP.Player.bHealth)
						{
							float maxHealth = actor.GetHealthComponent().GetMaxHealth();
							DrawHealthBar(health, maxHealth, ScreenTop.x, ScreenTop.y + 1, wi, hi);
						}
					
					
				}
			}
		}

#ifndef Ships

		else if (name.find("BP_SmallShipTemplate_C") != std::string::npos || name.find("BP_SmallShipNetProxy") != std::string::npos)
		{
		if (!Vars.ESP.Ships.bActive)
			continue;

			auto pos = actor.GetRootComponent().GetPosition();
			auto distance = SOT->localCamera.position.DistTo(pos) / 100.00f;

			auto Ship = *reinterpret_cast<AShip*>(&actors[i]);

			FGuid crewid = Ship.GetCrewOwnershipComponent().GetCrewId();

			if (name.find("NetProxy") != std::string::npos)
			{
				if (Ship.GetOwningActor())
					continue;

				crewid = FGuid();
			}
				
			Color color = { Vars.ESP.Ships.colorEnemy[0],Vars.ESP.Ships.colorEnemy[1],Vars.ESP.Ships.colorEnemy[2],Vars.ESP.Ships.colorEnemy[3] };


			

			if (SOT->localPlayer.crewID == crewid)
			{
				color = { Vars.ESP.Ships.colorTeam[0],Vars.ESP.Ships.colorTeam[1],Vars.ESP.Ships.colorTeam[2],Vars.ESP.Ships.colorTeam[3] };
			}
			Vector2 Screen;
			if (Misc->WorldToScreen(Vector3(pos.x,pos.y,pos.z + 2000), &Screen))
				DrawString(std::string("Sloop [ " + std::to_string((int)distance) + "m ]").c_str() , Screen.x, Screen.y, color, true, "default");
		}

		else if (name.find("BP_MediumShipTemplate_C") != std::string::npos || name.find("BP_MediumShipNetProxy") != std::string::npos)
		{
		if (!Vars.ESP.Ships.bActive)
			continue;
		auto pos = actor.GetRootComponent().GetPosition();
		auto distance = SOT->localCamera.position.DistTo(pos) / 100.00f;

			auto Ship = *reinterpret_cast<AShip*>(&actors[i]);

			if (name.find("NetProxy") != std::string::npos)
				if (Ship.GetOwningActor())
					continue;


			Color color = { Vars.ESP.Ships.colorEnemy[0],Vars.ESP.Ships.colorEnemy[1],Vars.ESP.Ships.colorEnemy[2],Vars.ESP.Ships.colorEnemy[3] };

			if (SOT->localPlayer.crewID == Ship.GetCrewOwnershipComponent().GetCrewId())
				color = { Vars.ESP.Ships.colorTeam[0],Vars.ESP.Ships.colorTeam[1],Vars.ESP.Ships.colorTeam[2],Vars.ESP.Ships.colorTeam[3] };


			Vector2 Screen;
			if (Misc->WorldToScreen(Vector3(pos.x, pos.y, pos.z + 2000), &Screen))
				DrawString(std::string("Brigantine [ " + std::to_string((int)distance) + "m ]").c_str(), Screen.x, Screen.y, color, true, "default");
		}

		else if (name.find("BP_LargeShipTemplate_C") != std::string::npos || name.find("BP_LargeShipNetProxy") != std::string::npos)
		{
		if (!Vars.ESP.Ships.bActive)
			continue;


		auto pos = actor.GetRootComponent().GetPosition();
		auto distance = SOT->localCamera.position.DistTo(pos) / 100.00f;

			auto Ship = *reinterpret_cast<AShip*>(&actors[i]);

			if (name.find("NetProxy") != std::string::npos)
				if (Ship.GetOwningActor())
					continue;

			Color color = { Vars.ESP.Ships.colorEnemy[0],Vars.ESP.Ships.colorEnemy[1],Vars.ESP.Ships.colorEnemy[2],Vars.ESP.Ships.colorEnemy[3] };

			if (SOT->localPlayer.crewID == Ship.GetCrewOwnershipComponent().GetCrewId())
				color = { Vars.ESP.Ships.colorTeam[0],Vars.ESP.Ships.colorTeam[1],Vars.ESP.Ships.colorTeam[2],Vars.ESP.Ships.colorTeam[3] };


			Vector2 Screen;
			if (Misc->WorldToScreen(Vector3(pos.x, pos.y, pos.z + 2000), &Screen))
				DrawString(std::string("Galleon [ " + std::to_string((int)distance) + "m ]").c_str(), Screen.x, Screen.y, color, true, "default");
		}
	
		else if (name.find("BP_AISmallShipTemplate") != std::string::npos || name.find("BP_AISmallShipNetProxy") != std::string::npos)
		{
		if (!Vars.ESP.Ships.bActive)
			continue;
		auto pos = actor.GetRootComponent().GetPosition();
		auto distance = SOT->localCamera.position.DistTo(pos) / 100.00f;

		auto Ship = *reinterpret_cast<AShip*>(&actors[i]);

		if (name.find("NetProxy") != std::string::npos)
		{
			if (Ship.GetOwningActor())
				continue;
		}

		Color color = { Vars.ESP.World.colorWorld[0],Vars.ESP.World.colorWorld[1],Vars.ESP.World.colorWorld[2],Vars.ESP.World.colorWorld[3] };

		Vector2 Screen;
		if (Misc->WorldToScreen(Vector3(pos.x, pos.y, pos.z + 2000), &Screen))
			DrawString(std::string("Skeleton Sloop [ " + std::to_string((int)distance) + "m ]").c_str(), Screen.x, Screen.y, color, true, "default");
		}
		else if (name.find("BP_AILargeShipTemplate") != std::string::npos || name.find("BP_AILargeShipNetProxy") != std::string::npos)
		{
		if (!Vars.ESP.Ships.bActive)
			continue;
		auto pos = actor.GetRootComponent().GetPosition();
		auto distance = SOT->localCamera.position.DistTo(pos) / 100.00f;

		auto Ship = *reinterpret_cast<AShip*>(&actors[i]);

		if (name.find("NetProxy") != std::string::npos)
		{
			if (Ship.GetOwningActor())
				continue;
		}

		Color color = { Vars.ESP.World.colorWorld[0],Vars.ESP.World.colorWorld[1],Vars.ESP.World.colorWorld[2],Vars.ESP.World.colorWorld[3] };

		Vector2 Screen;
		if (Misc->WorldToScreen(Vector3(pos.x, pos.y, pos.z + 2000), &Screen))
			DrawString(std::string("Skeleton Galleon [ " + std::to_string((int)distance) + "m ]").c_str(), Screen.x, Screen.y, color, true, "default");
		}

#endif // !Ships
		else if (name.find("BP_Shipwreck_") != std::string::npos)
		{

		if (!Vars.ESP.World.bShipWreck)
			continue;

		auto pos = actor.GetRootComponent().GetPosition();
		auto distance = SOT->localCamera.position.DistTo(pos) / 100.00f;

		Color color = { Vars.ESP.World.colorWorld[0],Vars.ESP.World.colorWorld[1],Vars.ESP.World.colorWorld[2],Vars.ESP.World.colorWorld[3] };


		Vector2 Screen;
		if (Misc->WorldToScreen(Vector3(pos.x, pos.y, 2000), &Screen))
			DrawString(std::string("ShipWreck [ " + std::to_string((int)distance) + "m ]").c_str(), Screen.x, Screen.y, color, true, "default");
		}

		else if (name.find("BP_SunkenCurseArtefact_") != std::string::npos)
		{
		auto pos = actor.GetRootComponent().GetPosition();
		auto distance = SOT->localCamera.position.DistTo(pos) / 100.00f;

			Vector2 Screen;
			if (Misc->WorldToScreen(Vector3(pos.x, pos.y, pos.z), &Screen))
			{
				if (name.find("Ruby") != std::string::npos)
				{
					DrawString(std::string("Ruby Statue [ " + std::to_string((int)distance) + "m ]").c_str(), Screen.x, Screen.y, Color{ 255,0,0 }, true, "default");
				}
				else if (name.find("Emerald") != std::string::npos)
				{
					DrawString(std::string("Emerald Statue [ " + std::to_string((int)distance) + "m ]").c_str(), Screen.x, Screen.y, Color{ 0,255,0 }, true, "default");
				}
				else if (name.find("Sapphire") != std::string::npos)
				{
					DrawString(std::string("Sapphire Statue [ " + std::to_string((int)distance) + "m ]").c_str(), Screen.x, Screen.y, Color{ 0,0,255 }, true, "default");
				}
			}
		}

		else if ((name.find("BP_Pig_") != std::string::npos && Vars.ESP.Animals.bPig) || (name.find("BP_Snake_") != std::string::npos && Vars.ESP.Animals.bSnake) || (name.find("BP_Chicken_") != std::string::npos && Vars.ESP.Animals.bChicken))
		{
		if (!Vars.ESP.Animals.bActive)
		continue;

		auto pos = actor.GetRootComponent().GetPosition();
		auto distance = SOT->localCamera.position.DistTo(pos) / 100.00f;

		auto Fauna = *reinterpret_cast<AFauna*>(&actors[i]);

		Color color = Color{ Vars.ESP.colorOther[0], Vars.ESP.colorOther[1], Vars.ESP.colorOther[2], Vars.ESP.colorOther[3] };

		if (name.find("Common") != std::wstring::npos)
			color = Color{ Vars.ESP.colorCommon[0], Vars.ESP.colorCommon[1], Vars.ESP.colorCommon[2], Vars.ESP.colorCommon[3] };

		else if (name.find("Rare") != std::wstring::npos)
			color = Color{ Vars.ESP.colorRare[0], Vars.ESP.colorRare[1], Vars.ESP.colorRare[2], Vars.ESP.colorRare[3] };

		else if (name.find("Mythical") != std::wstring::npos)
			color = Color{ Vars.ESP.colorMythical[0], Vars.ESP.colorMythical[1], Vars.ESP.colorMythical[2], Vars.ESP.colorMythical[3] };

		else if (name.find("Legendary") != std::wstring::npos)
			color = Color{ Vars.ESP.colorLegendary[0], Vars.ESP.colorLegendary[1], Vars.ESP.colorLegendary[2], Vars.ESP.colorLegendary[3] };


		Vector2 Screen;
		if (Misc->WorldToScreen(pos, &Screen))
			DrawString(std::wstring(Fauna.GetName() + L" [ " + std::to_wstring((int)distance) + L"m ] ").c_str(), Screen.x, Screen.y, color, true, "default");

		}

		else if (name.find("Proxy") != std::string::npos)
		{
		if (!Vars.ESP.Treasure.bActive)
			continue;
		auto pos = actor.GetRootComponent().GetPosition();
		auto distance = SOT->localCamera.position.DistTo(pos) / 100.00f;

			auto treasure = *reinterpret_cast<AItemProxy*>(&actors[i]);

			int booty = treasure.GetBootyItemInfo().GetBootyType();

			if (booty > EBootyTypes::EBootyTypes__EBootyTypes_MAX || booty < 1)
				continue;

			auto rarity = getNameFromIDmem(treasure.GetBootyItemInfo().GetRareityId());

			Color color = Color{ Vars.ESP.colorOther[0], Vars.ESP.colorOther[1], Vars.ESP.colorOther[2], Vars.ESP.colorOther[3] };

			if (name.find("Common") != std::wstring::npos)
				color = Color{ Vars.ESP.colorCommon[0], Vars.ESP.colorCommon[1], Vars.ESP.colorCommon[2], Vars.ESP.colorCommon[3] };

			else if (name.find("Rare") != std::wstring::npos)
				color = Color{ Vars.ESP.colorRare[0], Vars.ESP.colorRare[1], Vars.ESP.colorRare[2], Vars.ESP.colorRare[3] };

			else if (name.find("Mythical") != std::wstring::npos)
				color = Color{ Vars.ESP.colorMythical[0], Vars.ESP.colorMythical[1], Vars.ESP.colorMythical[2], Vars.ESP.colorMythical[3] };

			else if (name.find("Legendary") != std::wstring::npos)
				color = Color{ Vars.ESP.colorLegendary[0], Vars.ESP.colorLegendary[1], Vars.ESP.colorLegendary[2], Vars.ESP.colorLegendary[3] };
			else if (rarity.find("Fort") != std::string::npos || rarity.find("Stronghold") != std::string::npos || rarity.find("PirateLegend") != std::string::npos || rarity.find("Drunken") != std::string::npos || rarity.find("Weeping") != std::string::npos || rarity.find("AIShip") != std::string::npos)
				color = Color{ Vars.ESP.colorSpecial[0], Vars.ESP.colorSpecial[1], Vars.ESP.colorSpecial[2], Vars.ESP.colorSpecial[3] };

			Vector2 Screen;
			if (Misc->WorldToScreen(pos, &Screen))
				DrawString(std::wstring(treasure.GetBootyItemInfo().GetItemDesc().GetName() + L" [ " + std::to_wstring((int)distance) + L"m ] ").c_str(), Screen.x, Screen.y, color, true, "default");
		}


	}



}
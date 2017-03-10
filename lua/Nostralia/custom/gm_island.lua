--[[
 * http://nostralia.org/ True Australian Vanilla World of Warcraft
 * DESC   : GM Island teleport.
 * UPDATED: 7th March
 * AUTHOR : sundays
--]]

local ZONE_GM_ISLAND = 876;
local SPELL_GM_FREEZE = 9454;
local NPC_GUARDIAN = 5764;
local FACTION_FRIENDLY = 35;
local FLAG_NOT_TARGETABLE = 33554432;

local function NotifyGameMasters(playerName)
	local gmsInWorld = GetPlayersInWorld(2, true); -- TEAM_NEUTRAL
	local text = string.format("ATTENTION: Player %s has entered GM Island. Your assistance is required.", playerName);
	local next = next;
	
	if (next(gmsInWorld)) then
		for _, pPlayer in pairs(gmsInWorld) do
			pPlayer:SendBroadcastMessage(text);
			pPlayer:SendNotification(text);
		end
	end
end

local function InfoNotice(event, delay, repeats, pPlayer)
	if (not pPlayer) then
		return;
	end
	
	if (not pPlayer:HasAura(SPELL_GM_FREEZE)) then
		pPlayer:RemoveEventById(event);
		return;
	end
	
	pPlayer:SendBroadcastMessage("A Game Master will attend to you shortly.");
	NotifyGameMasters(pPlayer:GetName());
end

local function TeleportWhiteRoom(event, delay, repeats, pPlayer)
	if (not pPlayer) then
		return;
	end
	pPlayer:NearTeleport(16227.799805, 16403.400391, -64.380402, 0);
	pPlayer:RegisterEvent(InfoNotice, 60000, 0);
end

local function OnPlayerMapChange(event, pPlayer, newZone, newArea)
	if (newZone ~= ZONE_GM_ISLAND) then
		return;
	end
	
	if (not pPlayer) then
		return;
	end
	
	if (pPlayer:GetGMRank() >= 1) then
		return;
	end

	-- Got teleported here by the script.
	if (pPlayer:HasAura(SPELL_GM_FREEZE)) then
		return;
	end
	
	local x, y, z;
	local pGuardian;
	pPlayer:CastSpell(pPlayer, SPELL_GM_FREEZE, true);
	pPlayer:SetPlayerLock(true);
	pPlayer:NearTeleport(16226.2, 16257, 13.202, 1.65); -- GM Island
	x, y, z = pPlayer:GetRelativePoint(8, math.random(0, math.pi * 2));
	pGuardian = pPlayer:SpawnCreature(NPC_GUARDIAN, x, y, z, 0, 3, 10000);
	pGuardian:SetFacingToObject(pPlayer);
	pGuardian:SetFaction(FACTION_FRIENDLY);
	pGuardian:SetFlag(46, FLAG_NOT_TARGETABLE);
	PrintError(string.format("Player %s (guid %d) entered GM Island.", pPlayer:GetName(), pPlayer:GetGUIDLow())); -- Need a better log :)
	pPlayer:SendBroadcastMessage("Your actions have been logged and reported to a Game Master.");
	pPlayer:SendBroadcastMessage("You will soon be teleported to a safe location.");
	pPlayer:RegisterEvent(TeleportWhiteRoom, 8000, 1);
	NotifyGameMasters(pPlayer:GetName());
end

RegisterPlayerEvent(27, OnPlayerMapChange); -- PLAYER_EVENT_ON_UPDATE_ZONE

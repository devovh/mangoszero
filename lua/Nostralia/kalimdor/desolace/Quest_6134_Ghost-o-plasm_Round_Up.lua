--[[
 * http://nostralia.org/ True Australian Vanilla World of Warcraft
 * DESC   : Script for the quest 'Ghost-o-Plasm Roundup'.
 * UPDATED: 2nd March 2017
 * AUTHOR : sundays
 * NOTE   : Broken up into two parts; Magnet AI and Ghost AI.
          : Timings come from Nostalrius and some Youtube videos of patch > 4.3.4.
--]]

--[[
Emotes not confirmed on 1.12.1

When approaching magnet: Magrami Spectre is drawn to the ghost magnet...
When changing to hostile: Magrami Spectre is angered!
--]]

local NPC_MAGRAMI_SPECTRE      = 11560;
local GO_GHOST_MAGNET          = 177746;
local GO_GHOST_MAGNET_AURA     = 177749;
local SPELL_CURSE_OF_THE_FALLEN_MAGRAM = 18159;
local SPELL_DESTROY_MAGNET     = 19571; -- Not used?
local SPELL_PLACE_GHOST_MAGNET = 19588;
local SPELL_BLUE_AURA          = 17327; -- Neutral, walk to target.
local SPELL_GREEN_AURA         = 18951;
local FACTION_NEUTRAL          =   634; -- 58?
local FACTION_ENEMY            =    16;

local QuestGhostoplasm = {
	Magnets = {},
};

-- Magnet AI
function QuestGhostoplasm.OnMagnetSpawned(event, go)
	local magnetGUID = go:GetGUIDLow();
	local magnetsInRange;
	local magnetCount;
	local x, y, z;

	-- Need to check if this already exists?
	QuestGhostoplasm.Magnets[magnetGUID] = {};
	QuestGhostoplasm.Magnets[magnetGUID].state = true;
	QuestGhostoplasm.Magnets[magnetGUID].numberToSpawn = 8;
	QuestGhostoplasm.Magnets[magnetGUID].nextGhostTimer = 5000;
	
	magnetsInRange = go:GetGameObjectsInRange(30, GO_GHOST_MAGNET, 0);
	magnetCount = #magnetsInRange;
	
	for i = 1, magnetCount do
		if (magnetsInRange[i] and magnetsInRange[i]:IsSpawned()) then
			-- If another active magnet is available, overwrite it.
			local otherMagnet = magnetsInRange[i]:GetGUIDLow();
			if (QuestGhostoplasm.Magnets[otherMagnet]) then
				QuestGhostoplasm.Magnets[otherMagnet].state = false;
				break;
			end
		end
	end

	x, y, z = go:GetLocation();
	go:SummonGameObject(GO_GHOST_MAGNET_AURA, x, y, z, 0, 120);
end

function QuestGhostoplasm.MagnetUpdateAI(event, pObject, diff)
	local magnetGUIDLow = pObject:GetGUIDLow();
	local magnetData = QuestGhostoplasm.Magnets[magnetGUIDLow];
	
	if (not magnetData or not magnetData.state) then
		return;
	end

	if (magnetData.nextGhostTimer <= diff) then
		QuestGhostoplasm.SpawnSpectre(pObject);
		magnetData.nextGhostTimer = math.random(3000, 8000);
	else
		magnetData.nextGhostTimer = magnetData.nextGhostTimer - diff;
	end
end

function QuestGhostoplasm.SpawnSpectre(pMagnet)
	local magnetGUID = pMagnet:GetGUIDLow();
	local x, y, z = pMagnet:GetRelativePoint(40, math.random(0, 2 * math.pi)); -- Ghost spawn
	local magnetData = QuestGhostoplasm.Magnets[magnetGUID];
	
	if (magnetData.numberToSpawn <= 0) then
		magnetData.state = false;
		return;
	end
	
	pSpectre = pMagnet:SpawnCreature(NPC_MAGRAMI_SPECTRE, x, y, z, 0, 4, 120000); -- TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT
	if (pSpectre) then
		pSpectre:SetData(2, pMagnet:GetGUIDLow());
		magnetData.numberToSpawn = magnetData.numberToSpawn - 1;
	end
end

function QuestGhostoplasm.OnMagnetRemoved(event, pObject)
	QuestGhostoplasm.Magnets[pObject:GetGUIDLow()] = nil;

	-- Aura never disappears when spawned with SummonGameObject, for some reason.
	local pMyAura = pObject:GetNearestGameObject(1, GO_GHOST_MAGNET_AURA, 0);
	if (pMyAura) then
		pMyAura:RemoveFromWorld();
	end
end

RegisterGameObjectEvent(GO_GHOST_MAGNET,  1, QuestGhostoplasm.MagnetUpdateAI);
RegisterGameObjectEvent(GO_GHOST_MAGNET,  2, QuestGhostoplasm.OnMagnetSpawned);
RegisterGameObjectEvent(GO_GHOST_MAGNET, 13, QuestGhostoplasm.OnMagnetRemoved);

-- Spectre
function QuestGhostoplasm.TurnSpectreGreen(event, delay, repeats, pCreature)
	if (pCreature:HasAura(SPELL_BLUE_AURA)) then
		pCreature:RemoveAura(SPELL_BLUE_AURA);
	end
	pCreature:SetFaction(FACTION_ENEMY);
	pCreature:AddAura(SPELL_GREEN_AURA, pCreature);
	pCreature:SetData(1, true); -- isGreen
	
	local pPlayer = pCreature:GetNearestPlayer(15, 1, 1); -- 45yd, hostile, alive
	if (pPlayer) then
		pCreature:AttackStart(pPlayer);
	else
		pCreature:MoveRandom(30); -- Not blizzlike(?), but a good solution.
	end
end

function QuestGhostoplasm.OnSpectreSpawned(event, pCreature)
	local magnetGUIDLow = pCreature:GetData(2);
	local magnetGUID = tonumber(GO_GHOST_MAGNET .. magnetGUIDLow .. 0); -- entry, guidlow, typeid make up the object's GUID
	local pMap = pCreature:GetMap();
	local facing;
	local x, y, z;
	local pMagnet = pMap:GetWorldObject(magnetGUID);
	local firstCurse = math.random(8000, 14000);
	
	pCreature:AddAura(SPELL_BLUE_AURA, pCreature);
	pCreature:SetFaction(FACTION_NEUTRAL);
	pCreature:SetFacingToObject(pMagnet);
	facing = pCreature:GetO();
	x, y, z = pMagnet:GetRelativePoint(2, facing); -- 2pi = 360
	pCreature:SetHomePosition(x, y, z, facing - math.pi);
	-- pCreature:SetWalk(true); -- Doesn't work when creature moving home. Always runs.
	-- pCreature:MoveTo(0, x, y, z, true);
	pCreature:SetSpeed(1, 0.5, true);
	pCreature:MoveHome();
	pCreature:RegisterEvent(QuestGhostoplasm.MagramiCurse, firstCurse, 1);
end

function QuestGhostoplasm.OnSpectreReachHome(event, pCreature)
	local isGreen = pCreature:GetData(1);
	
	if (isGreen) then
		return;
	end
	pCreature:MoveIdle();
	pCreature:RegisterEvent(QuestGhostoplasm.TurnSpectreGreen, 2500, 1);
end

function QuestGhostoplasm.MagramiCurse(event, delay, repeats, pCreature)
	local pVictim = pCreature:GetVictim();
	local nextCurse = 5000; -- milliseconds
	
	if (pVictim and not pVictim:HasAura(SPELL_CURSE_OF_THE_FALLEN_MAGRAM)) then
		if (math.random(1, 10) > 7) then
			pCreature:CastSpell(pVictim, SPELL_CURSE_OF_THE_FALLEN_MAGRAM, false);
		end
		nextCurse = math.random(15000, 21000);
	end
	
	pCreature:RegisterEvent(QuestGhostoplasm.MagramiCurse, nextCurse, 1);
end

function QuestGhostoplasm.MagramiReset(event, pCreature)
	if (not pCreature:IsAlive()) then
		local magnetGUID = pCreature:GetData(2);
		QuestGhostoplasm.Magnets[magnetGUID].nextGhostTimer = 0; -- Spawn next ghost after death.
	end
	pCreature:RemoveEvents(); -- Curse event
end

RegisterCreatureEvent(NPC_MAGRAMI_SPECTRE,  2, QuestGhostoplasm.MagramiReset);
RegisterCreatureEvent(NPC_MAGRAMI_SPECTRE,  4, QuestGhostoplasm.MagramiReset);
RegisterCreatureEvent(NPC_MAGRAMI_SPECTRE,  5, QuestGhostoplasm.OnSpectreSpawned);
RegisterCreatureEvent(NPC_MAGRAMI_SPECTRE, 24, QuestGhostoplasm.OnSpectreReachHome);

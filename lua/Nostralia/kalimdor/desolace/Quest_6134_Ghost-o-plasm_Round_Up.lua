--[[
 * http://nostralia.org/ True Australian Vanilla World of Warcraft
 * DESC   : Script for the quest 'Ghost-o-Plasm Roundup'.
 * UPDATED: 2nd March 2017
 * AUTHOR : sundays
 * NOTE   : Broken up into two parts; Magnet AI and Ghost AI.
--]]

return; -- Not yet finished.

local NPC_MAGRAMI_SPECTRE      = 11560;
local GO_GHOST_MAGNET          = 177746;
local GO_GHOST_MAGNET_AURA     = 177749;
local SPELL_CURSE_OF_THE_FALLEN_MAGRAM = 18159;
local SPELL_DESTROY_MAGNET     = 19571; -- Not used?
local SPELL_PLACE_GHOST_MAGNET = 19588;
local SPELL_BLUE_AURA          = 17327; -- Neutral, walk to target.
local SPELL_GREEN_AURA         = 18951;
local FACTION_NEUTRAL          = 634; -- 58?
local FACTION_ENEMY            = 16;

local QuestGhostoplasm = {
	Magnets = {},
	Ghosts  = {},
};

-- Magnet AI
function QuestGhostoplasm.OnMagnetSpawn(event, go)
	local magnetGUID = go:GetGUIDLow();
	local magnetsInRange;
	local magnetCount;
	
	if (not QuestGhostoplasm.Magnets[magnetGUID]) then
		QuestGhostoplasm.Magnets[magnetGUID] = {};
	end
	
	magnetsInRange = go:GetGameObjectsInRange(30, GO_GHOST_MAGNET_AURA, 0);
	magnetCount = #magnetsInRange;
	
	for (i = 1, magnetCount) do
		if (magnetsInRange[i] and magnetsInRange[i]:IsSpawned()) then
			QuestGhostoplasm.Magnets[magnetGUID].state = false;
			QuestGhostoplasm.Magnets[magnetGUID].numberToSpawn = 8;
			break;
		end
	end
	
	if (QuestGhostoplasm.Magnets[magnetGUID].state) then
		local x, y, z = go:GetLocation();
		go:SummonGameObject(GO_GHOST_MAGNET_AURA, x, y, z, 0, 120);
	end
	
	go:RegisterEvent(QuestGhostoplasm.SpawnSpectre, 5000, 1); -- Spawn spectre event
end

function QuestGhostoplasm.SpawnSpectre(event, delay, repeats, worldobject)
	local magnetGUID = worldobject:GetGUIDLow();
	local x, y, z = go:GetRelativePoint(40, math.random(0, math.pi)); -- Ghost spawn
	local numToSpawn = QuestGhostoplasm.Magnets[magnetGUID].numberToSpawn;
	
	if (numToSpawn <= 0) then
		return;
	end
	
	pSpectre = go:SpawnCreature(NPC_MAGRAMI_SPECTRE, x, y, z, 0, 5, 120000); -- TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT
	if (pSpectre) then
		QuestGhostoplasm.Ghosts[pSpectre:GetGUIDLow()].magnetRelation = worldobject:GetGUIDLow(); -- guid of magnet that spawned the ghost.
		pSpectre:SetRespawnDelay(425000);
		numToSpawn = numToSpawn - 1;
	end
	worldobject:RegisterEvent(QuestGhostoplasm.SpawnSpectre, math.random(3000, 8000), 1); -- Spawn spectre event
end

RegisterGameObjectEvent(GO_GHOST_MAGNET, 2, QuestGhostoplasm.OnMagnetSpawn);

-- Spectre
function QuestGhostoplasm.OnSpectreDied(event, creature)
	local spectreGUID = creature:GetGUIDLow();
	local pMap = creature:GetMap();
	
	if (QuestGhostoplasm.Ghosts[spectreGUID]) then
		local pMagnet = pMap:GetWorldObject(QuestGhostoplasm.Ghosts[spectreGUID].magnetRelation);
		QuestGhostoplasm
	end
end

RegisterCreatureEvent(NPC_MAGRAMI_SPECTRE, 4, QuestGhostoplasm.OnSpectreDied);
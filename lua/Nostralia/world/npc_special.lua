--[[
 * http://nostralia.org/ True Australian Vanilla World of Warcraft
 * DESC   : Script for NPCs that are ubiquitous throughout the world.
 * UPDATED: 5th March 2017
 * AUTHOR : sundays
--]]

--[[
 * Target Dummy
--]]

local NPC_TARGET_DUMMY = 2673;
local NPC_ADVANCED_TARGET_DUMMY = 2674;
local NPC_MASTERWORK_TARGET_DUMMY = 12426;

local NpcTargetDummy = {
	-- spawnEffect, passiveEffect
	DummySpellAssoc = {
		[NPC_TARGET_DUMMY] = {4507, 4044},
		[NPC_ADVANCED_TARGET_DUMMY] = {4092, 4048},
		[NPC_MASTERWORK_TARGET_DUMMY] = {4092, 19809}, -- Masterwork dummy has no spawn effect, use the one from Advanced Dummy.
	};
};

function NpcTargetDummy.Despawn(event, delay, repeats, pCreature)
	if (not pCreature) then
		return;
	end
	pCreature:Kill(pCreature);
end

function NpcTargetDummy.OnSpawn(event, pCreature)
	local dummyEntry = pCreature:GetEntry();
	local spawnEffect;
	local passiveEffect;
	
	if (not NpcTargetDummy.DummySpellAssoc[dummyEntry]) then
		return;
	end
	
	spawnEffect = NpcTargetDummy.DummySpellAssoc[dummyEntry][1];
	passiveEffect = NpcTargetDummy.DummySpellAssoc[dummyEntry][2];
	pCreature:SetDefaultMovementType(0); -- Idle
	pCreature:AddUnitState(24); -- UNIT_STAT_STUNNED & UNIT_STAT_ROOT
	pCreature:CastSpell(pCreature, spawnEffect, true); -- Initial taunt
	pCreature:AddAura(passiveEffect, pCreature);       -- Taunts every 3 seconds.
	pCreature:RegisterEvent(NpcTargetDummy.Despawn, 15000, 1);
end

function NpcTargetDummy.OnDamageTaken(event, pCreature, pAttacker, damage)
	pCreature:AddThreat(pAttacker, damage);
end

RegisterCreatureEvent(NPC_TARGET_DUMMY, 5, NpcTargetDummy.OnSpawn);
RegisterCreatureEvent(NPC_TARGET_DUMMY, 9, NpcTargetDummy.OnDamageTaken);

RegisterCreatureEvent(NPC_ADVANCED_TARGET_DUMMY, 5, NpcTargetDummy.OnSpawn);
RegisterCreatureEvent(NPC_ADVANCED_TARGET_DUMMY, 9, NpcTargetDummy.OnDamageTaken);

RegisterCreatureEvent(NPC_MASTERWORK_TARGET_DUMMY, 5, NpcTargetDummy.OnSpawn);
RegisterCreatureEvent(NPC_MASTERWORK_TARGET_DUMMY, 9, NpcTargetDummy.OnDamageTaken);

--[[
 * Goblin Land Mine
--]]

local NpcLandMine = {
	Landmines = {},
};

local NPC_GOBLIN_LAND_MINE = 7527;
local SPELL_DETONATION = 4043;

function NpcLandMine.OnSpawn(event, pCreature)
	local landmineGUID = tostring(pCreature:GetGUID());
	NpcLandMine.Landmines[landmineGUID] = {};
	NpcLandMine.Landmines[landmineGUID].isArmed = false;
	NpcLandMine.Landmines[landmineGUID].isDetonating = false;
	NpcLandMine.Landmines[landmineGUID].isDespawning = false;
	
	pCreature:SetDefaultMovementType(0); -- Idle
	pCreature:AddUnitState(24); -- UNIT_STAT_STUNNED & UNIT_STAT_ROOT
	pCreature:RegisterEvent(NpcLandMine.ArmLandmine, 10000, 1);
	pCreature:RegisterEvent(NpcLandMine.Despawn, 70000, 1); -- 60s + 10s arm time
end

function NpcLandMine.MoveInLineOfSight(event, pCreature, pUnit)
	local landmineGUID = tostring(pCreature:GetGUID());
	
	-- MoveInLineOfSight can be called before the creature spawn event registers... nice one.
	if (not NpcLandMine.Landmines[landmineGUID]) then
		return;
	end
	
	local isArmed = NpcLandMine.Landmines[landmineGUID].isArmed;
	local isDetonating = NpcLandMine.Landmines[landmineGUID].isDetonating;
	local isDespawning = NpcLandMine.Landmines[landmineGUID].isDespawning;
	
	if (not isArmed or isDetonating or isDespawning) then
		return;
	end
	
	-- TODO: NEEDS HOSTILE CHECK (NYI)
	if (pCreature:GetDistance(pUnit) < 5) then
		NpcLandMine.Landmines[landmineGUID].isDetonating = true;
		pCreature:RegisterEvent(NpcLandMine.Detonate, 500, 1);
	end
end

function NpcLandMine.ArmLandmine(event, delay, repeats, pCreature)
	if (not pCreature) then
		return;
	end
	
	local guid = tostring(pCreature:GetGUID());
	NpcLandMine.Landmines[guid].isArmed = true;
end

function NpcLandMine.Detonate(event, delay, repeats, pCreature)
	local landmineGUID = tostring(pCreature:GetGUID());
	pCreature:CastSpell(pCreature, SPELL_DETONATION, false);
	NpcLandMine.Landmines[landmineGUID].isDespawning = true;
	NpcLandMine.Landmines[landmineGUID].isDetonating = false;
	pCreature:RegisterEvent(NpcLandMine.Despawn, 500, 1);
end

function NpcLandMine.Despawn(event, delay, repeats, pCreature)
	local landmineGUID = tostring(pCreature:GetGUID());
	NpcLandMine.Landmines[landmineGUID] = nil;
	PrintInfo("Goblin Land Mine is missing the hostile check for units. Please implement for correct behaviour.");
	pCreature:RemoveEvents();
	pCreature:Kill(pCreature);
end

RegisterCreatureEvent(NPC_GOBLIN_LAND_MINE,  5, NpcLandMine.OnSpawn);
RegisterCreatureEvent(NPC_GOBLIN_LAND_MINE, 27, NpcLandMine.MoveInLineOfSight);

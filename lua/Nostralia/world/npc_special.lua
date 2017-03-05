--[[
 * http://nostralia.org/ True Australian Vanilla World of Warcraft
 * DESC   : Script for NPCs that are ubiquitous throughout the world.
 * UPDATED: 4th March 2017
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
	pCreature:SetFaction(679);
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
--[[
 * http://nostralia.org/ True Australian Vanilla World of Warcraft
 * DESC   : Scripts for quests that appear in more than one place.
 * UPDATED: 6th March 2017
 * AUTHOR : sundays
--]]

--[[
 * Quest: Investigate the Blue Recluse (1920) & Investigate the Alchemist Shop (1960)
--]]

local NPC_RIFT_SPAWN = 6492;
-- LOCAL_SPELL_CREATE_COFFER = 9082;		-- DND! USED BY THE GIVEN QUEST ITEM!
-- local SPELL_CREATE_FILLED_COFFER = 9010;    -- Creates lootable gobject that contains the required quest item.
local SPELL_ATTRACT_RIFT_SPAWN = 9012;      -- 25yd (DUMMY EFFECT) Used to attract nearby rift spawns to caster. [NYI]
local SPELL_RIFT_SPAWN_INVISIBILITY = 9093; -- Makes rift spawns invisible.
local SPELL_CANTATION_OF_MANIFESTATION = 9095; -- Cast by item, casues rift spawns to become visible.
local SPELL_RIFT_SPAWN_BECOMES_VISIBLE = 9738; -- ??
local SPELL_STUN_VISUAL = 18970; 			   -- Probably not blizzlike
local TEXT_RIFT_SPAWN_ATTACK = "%s is angered and attacks!";
local TEXT_RIFT_SPAWN_ESCAPE = "%s escapes into the void!";
local FACTION_ENEMY   = 91; -- From database.
local FACTION_NEUTRAL = 7;  -- Hack to prevent entering/leaving combat spam due to creature being invisible and unable to attack player.

-- Rift Spawn
local NpcRiftSpawn = {};

function NpcRiftSpawn.OnDamageTaken(event, pCreature, pAttacker, damage)
	local health = pCreature:GetHealth();
	if (damage >= health) then
		-- Stun ourselves and disappear soon after.
		if (not pCreature:GetData(1)) then
			pCreature:SetData(1, true); -- isStunned
			pCreature:RegisterEvent(NpcRiftSpawn.Despawn, 10000, 1); -- Guessed timer.
			pCreature:AddUnitState(8 + 16); -- UNIT_STAT_STUNNED & UNIT_STAT_ROOT
			pCreature:CastSpell(pCreature, SPELL_STUN_VISUAL, true);
			pCreature:CastSpell(pCreature, 29230, true); -- Immune to all. Ugly hack. Please kill me.
			pCreature:SetHealth(pCreature:GetMaxHealth());
		end
		damage = 0;
		return damage;
	end
end

function NpcRiftSpawn.OnHitBySpell(event, pCreature, pCaster, spellid)
	if (spellid == SPELL_CANTATION_OF_MANIFESTATION) then
		if (pCreature:HasAura(SPELL_RIFT_SPAWN_INVISIBILITY)) then
			pCreature:RemoveAura(SPELL_RIFT_SPAWN_INVISIBILITY);
		end
		
		if (pCreature:HasAura(SPELL_CANTATION_OF_MANIFESTATION)) then
			pCreature:RemoveAura(SPELL_CANTATION_OF_MANIFESTATION);
		end
		pCreature:SetFaction(FACTION_ENEMY);
		pCreature:SendUnitEmote(TEXT_RIFT_SPAWN_ATTACK, nil, false);
	end
end

function NpcRiftSpawn.Despawn(event, delay, repeats, pCreature)
	-- If the creature is already despawned due to being captured, we return safely.
	if (not pCreature) then
		return;
	end
	pCreature:SendUnitEmote(TEXT_RIFT_SPAWN_ESCAPE, nil, false);
	pCreature:DespawnOrUnsummon();
end

function NpcRiftSpawn.OnReset(event, pCreature)
	pCreature:SetFaction(FACTION_NEUTRAL);
	if (not pCreature:HasAura(SPELL_RIFT_SPAWN_INVISIBILITY)) then
		pCreature:CastSpell(pCreature, SPELL_RIFT_SPAWN_INVISIBILITY, true);
	end
end

RegisterCreatureEvent(NPC_RIFT_SPAWN,  9, NpcRiftSpawn.OnDamageTaken);
RegisterCreatureEvent(NPC_RIFT_SPAWN, 14, NpcRiftSpawn.OnHitBySpell);
RegisterCreatureEvent(NPC_RIFT_SPAWN, 23, NpcRiftSpawn.OnReset);

-- Containment Coffer
local GoContainmentCoffer = {};

local GO_CONTAINMENT_COFFER = 122088;
local GO_FILLED_COFFER = 103574; -- Actually created via spell, but it's easier to do it like this.

function GoContainmentCoffer.UpdateAI(event, pObject, diff)
	local guid = pObject:GetGUIDLow();
	local openTimer = GoContainmentCoffer[guid].openTimer;
	local despawnTimer = GoContainmentCoffer[guid].despawnTimer;
	
	if (not GoContainmentCoffer[guid].isOpen) then
		if (openTimer <= diff) then
			pObject:UseDoorOrButton();
			GoContainmentCoffer[guid].isOpen = true;
			local pRiftSpawn = pObject:GetNearestCreature(0, NPC_RIFT_SPAWN, 0, 1);
			if (pRiftSpawn) then
				pRiftSpawn:RemoveEvents();
				pRiftSpawn:DespawnOrUnsummon(2000);
			end
		else
			GoContainmentCoffer[guid].openTimer = openTimer - diff;
		end
	end
	
	if (despawnTimer <= diff) then
		local x, y, z = pObject:GetLocation();
		pObject:SummonGameObject(GO_FILLED_COFFER, x, y, z, 0, 60); -- Actually spawned via spell(?)
		pObject:RemoveFromWorld();
	else
		GoContainmentCoffer[guid].despawnTimer = despawnTimer - diff;
	end
end

function GoContainmentCoffer.OnSpawn(event, pObject)
	local guid = pObject:GetGUIDLow();
	GoContainmentCoffer[guid] = {};
	GoContainmentCoffer[guid].openTimer = 750;
	GoContainmentCoffer[guid].despawnTimer = 3000;
end

RegisterGameObjectEvent(GO_CONTAINMENT_COFFER, 1, GoContainmentCoffer.UpdateAI);
RegisterGameObjectEvent(GO_CONTAINMENT_COFFER, 2, GoContainmentCoffer.OnSpawn);

-- Filled Containment Coffer
--[[
local function DespawnFilledCoffer(event, delay, repeats, pObject)
	if (not pObject) then
		return;
	end
	pObject:RemoveFromWorld();
end

local function OnFilledCofferSpawn(event,  pObject)
	pObject:RegisterEvent(DespawnFilledCoffer, 5000, 1);
end

RegisterGameObjectEvent(GO_FILLED_COFFER, 2, OnFilledCofferSpawn);
--]]

-- Chest of Containment Coffer
local ITEM_CHEST_OF_COFFERS = 7247;

local function OnUseChestOfContainmentCoffers(event, pPlayer, pItem, pTarget)
	if (not pPlayer) then
		return;
	end
	
	local pTarget = pPlayer:GetSelection(); -- Might not be correct.
	if (not pTarget) then
		pPlayer:SendNotification("Invalid target");
	end
	
	-- Only on Rift Spawns
	if (pTarget:GetEntry() ~= NPC_RIFT_SPAWN) then
		pPlayer:SendNotification("Invalid target");
		return false;
	end
	
	-- Only if the Rift Spawn is stunned. (HasUnitState(8) doesn't work)
	if (not pTarget:HasUnitState(8)) then
		pPlayer:SendNotification("Invalid target");
		return false;
	end
end

RegisterItemEvent(ITEM_CHEST_OF_COFFERS, 2, OnUseChestOfContainmentCoffers);

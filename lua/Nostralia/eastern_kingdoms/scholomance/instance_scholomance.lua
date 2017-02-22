--[[
 * http://nostralia.org/ True Australian Vanilla World of Warcraft
 * DESC   : Contains scripts not already handled in ACID or ScriptDev3.
 * UPDATED: 22nd Feb 2017
 * AUTHOR : sundays
--]]

-- Spectral Projection (summoned by Spectral Tutor)
-- Partly handled in ScriptDev3.
local SpectralProjection = {};

local NPC_SPECTRAL_PROJECTION     = 11263;
local SPELL_IMAGE_PROJECTION_HEAL = 17652;

function SpectralProjection.OnLeaveCombat(event, creature)
	creature:Kill(creature); -- Don't hang around, happens if Spectral Tutor resets during the Spectral Projection timer.
end

function SpectralProjection.OnHitBySpell(event, creature, caster, spellid)
	-- Spectral Tutor is ready to enter in combat again.
	if (spellid == SPELL_IMAGE_PROJECTION_HEAL) then
		creature:Kill(creature); -- Despawns on death.
	end
end

RegisterCreatureEvent(NPC_SPECTRAL_PROJECTION,  2, SpectralProjection.OnLeaveCombat); -- CREATURE_EVENT_ON_LEAVE_COMBAT
RegisterCreatureEvent(NPC_SPECTRAL_PROJECTION, 14, SpectralProjection.OnHitBySpell);  -- CREATURE_EVENT_ON_HIT_BY_SPELL

--[[
 * http://nostralia.org/ True Australian Vanilla World of Warcraft
 * DESC   : Script for the quest 'Valik' and the NPC 'Henchman Valik'.
 * UPDATED: 19th Feb 2017
 * AUTHOR : sundays
 * NOTE   : Perhaps missing data? 7th March 2016 the quest did not work on
 *          Nostalrius (watch?v=ZYxAS2sRd2A). Removed from retail in patch
 *          4.0.3, no relevant wowhead data. Maybe has aggro text?
--]]

local QuestValik = {};

local QUEST_VALIK = 535;
local CREATURE_HENCHMAN_VALIK = 2333;
local FACTION_HOSTILE = 83; -- Changes to this faction on quest completion.
local FACTION_FRIENDLY = 35;

function QuestValik.OnQuestComplete(event, player, creature, quest, opt)
    if (quest:GetId() == QUEST_VALIK) then
        creature:SetFaction(FACTION_HOSTILE);
        if (player) then
            creature:AttackStart(player);
        end
    end
end

function QuestValik.OnValikReset(event, creature)
    creature:SetFaction(FACTION_FRIENDLY);
end

RegisterCreatureEvent(CREATURE_HENCHMAN_VALIK, 23, QuestValik.OnValikReset);
RegisterCreatureEvent(CREATURE_HENCHMAN_VALIK, 34, QuestValik.OnQuestComplete);

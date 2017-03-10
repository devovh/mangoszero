/**
 * ScriptDev3 is an extension for mangos providing enhanced features for
 * area triggers, creatures, game objects, instances, items, and spells beyond
 * the default database scripting in mangos.
 *
 * Copyright (C) 2006-2013  ScriptDev2 <http://www.scriptdev2.com/>
 * Copyright (C) 2014-2017  MaNGOS  <https://getmangos.eu>
 * Copyright (C) 2017       NostraliaWoW  <https://nostralia.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

/**
 * ScriptData
 * SDName:      Boss_Lucifron
 * SD%Complete: 100
 * SDCategory:  Molten Core
 * EndScriptData
 */

#include "precompiled.h"
#include "molten_core.h"

enum
{
    SPELL_IMPENDINGDOOM     = 19702,
    SPELL_LUCIFRONCURSE     = 19703,
    SPELL_SHADOWSHOCK       = 19460
};

struct boss_lucifron : public CreatureScript
{
    boss_lucifron() : CreatureScript("boss_lucifron") {}

    struct boss_lucifronAI : public ScriptedAI
    {
        boss_lucifronAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
            Reset();
        }

        uint32 m_uiImpendingDoomTimer;
        uint32 m_uiLucifronCurseTimer;
        uint32 m_uiShadowShockTimer;

        ScriptedInstance* m_pInstance;

        void Reset() override
        {
            m_uiImpendingDoomTimer = 10000;
            m_uiLucifronCurseTimer = 20000;
            m_uiShadowShockTimer = 6000;

            if (m_pInstance && m_creature->IsAlive())
                m_pInstance->SetData(TYPE_LUCIFRON, NOT_STARTED);
        }

        void Aggro(Unit* /*pWho*/) override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_LUCIFRON, IN_PROGRESS);
            }
            m_creature->SetInCombatWithZone();
        }

        void JustDied(Unit* /*pKiller*/) override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_LUCIFRON, DONE);
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            // Impending doom timer
            if (m_uiImpendingDoomTimer < uiDiff)
            {
                if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                {
                    if (DoCastSpellIfCan(pTarget, SPELL_IMPENDINGDOOM) == CAST_OK)
                        m_uiImpendingDoomTimer = 20000;
                    else
                        m_uiImpendingDoomTimer = 100;
                }
            }
            else
            {
                m_uiImpendingDoomTimer -= uiDiff;
            }

            // Lucifron's curse timer
            if (m_uiLucifronCurseTimer < uiDiff)
            {
                if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                {
                    if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_LUCIFRONCURSE) == CAST_OK)
                        m_uiLucifronCurseTimer = 15000;
                    else
                        m_uiLucifronCurseTimer = 100;
                }
            }
            else
            {
                m_uiLucifronCurseTimer -= uiDiff;
            }

            // Shadowshock
            if (m_uiShadowShockTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_SHADOWSHOCK) == CAST_OK)
                    m_uiShadowShockTimer = 2000 + rand() % 4000;
            }
            else
            {
                m_uiShadowShockTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_lucifronAI(pCreature);
    }
};

void AddSC_boss_lucifron()
{
    Script* s;
    s = new boss_lucifron();
    s->RegisterSelf();
}

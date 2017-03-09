/**
 * ScriptDev3 is an extension for mangos providing enhanced features for
 * area triggers, creatures, game objects, instances, items, and spells beyond
 * the default database scripting in mangos.
 *
 * Copyright (C) 2006-2013  ScriptDev2 <http://www.scriptdev2.com/>
 * Copyright (C) 2014-2017  MaNGOS  <https://getmangos.eu>
 * Copyright (C) 2017       NostraliaWoW  <https://nostralia.org>
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
 * SDName:      Boss_Baron_Geddon
 * SD%Complete: 100
 * SDCategory:  Molten Core
 * EndScriptData
 */

#include "precompiled.h"
#include "molten_core.h"

enum
{
    EMOTE_SERVICE               = -1409000,

    SPELL_INFERNO               = 19695,
    SPELL_IGNITE_MANA           = 19659,
    SPELL_LIVING_BOMB           = 20475,
    SPELL_ARMAGEDDON            = 20478
};

struct boss_baron_geddon : public CreatureScript
{
    boss_baron_geddon() : CreatureScript("boss_baron_geddon") {}

    struct boss_baron_geddonAI : public ScriptedAI
    {
        boss_baron_geddonAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        }

        bool m_bIsArmageddon;
        bool m_bIsInferno;

        uint32 m_uiIgniteManaTimer;
        uint32 m_uiLivingBombTimer;
        uint32 m_uiInfernoTimer;
        uint32 m_uiInfernoAltTimer;
        uint32 m_uiRestoreTargetTimer;
        uint32 InfCount;
        uint32 Tick;

        ScriptedInstance* m_pInstance;

        void Reset() override
        {
            m_bIsArmageddon         = false;
            m_bIsInferno            = false;
            m_uiInfernoTimer        = urand(18000, 24000);
            m_uiIgniteManaTimer     = urand(10000, 15000);
            m_uiLivingBombTimer     = urand(15000, 20000);
            m_uiRestoreTargetTimer  = 0;

            if (m_pInstance && m_creature->IsAlive())
                m_pInstance->SetData(TYPE_GEDDON, NOT_STARTED);

            m_creature->clearUnitState(UNIT_STAT_ROOT);
        }

        void Aggro(Unit* /*pWho*/) override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_GEDDON, IN_PROGRESS);
            }
            m_creature->SetInCombatWithZone();
        }

        void JustDied(Unit* /*pKiller*/) override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_GEDDON, DONE);
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            if (m_bIsArmageddon)                                // Do nothing untill armageddon triggers
            {
                return;
            }

            // If we are <5% hp cast Armageddom
            if (!m_bIsArmageddon)
            {
                if (m_creature->GetHealthPercent() < 5.0f)
                {
                    m_creature->InterruptNonMeleeSpells(true);
                    SetCombatMovement(false);
                    m_creature->CastSpell(m_creature, SPELL_ARMAGEDDON, true);
                    DoScriptText(EMOTE_SERVICE, m_creature);
                    m_bIsArmageddon = true;
                    return;
                }
            }

            // Inferno_Timer
            if (m_uiInfernoTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_INFERNO) == CAST_OK)
                {
                    m_uiInfernoTimer = urand(18000, 24000);
                    InfCount = 0;
                    Tick = 1000;
                    m_bIsInferno = true;
                    m_creature->addUnitState(UNIT_STAT_ROOT);
                }
            }
            else
            {
                m_uiInfernoTimer -= uiDiff;
            }

            // Inferno damage increases with each tick
            if (m_bIsInferno)
            {
                if (Tick >= 1000)
                {
                    int Damage = 0;
                    switch (InfCount)
                    {
                        case 0:
                        case 1:
                            Damage = 500;
                            break;
                        case 2:
                        case 3:
                            Damage = 1000;
                            break;
                        case 4:
                        case 5:
                            Damage = 1500;
                            break;
                        case 6:
                        case 7:
                            Damage = 2000;
                            break;
                        case 8:
                            Damage = 2500;
                            m_bIsInferno = false;
                            m_creature->clearUnitState(UNIT_STAT_ROOT);
                            break;
                    }
                    m_creature->CastCustomSpell(m_creature, 19698, &Damage, NULL, NULL, true);
                    InfCount++;
                    Tick = 0;
                }
                Tick += uiDiff;
                return;
            }

            // Ignite Mana Timer
            if (m_uiIgniteManaTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_IGNITE_MANA) == CAST_OK)
                {
                    m_uiIgniteManaTimer = urand(20000, 30000);
                }
            }
            else
            {
                m_uiIgniteManaTimer -= uiDiff;
            }

            // Living Bomb Timer
            if (m_uiLivingBombTimer < uiDiff)
            {
                if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                {
                    if (DoCastSpellIfCan(pTarget, SPELL_LIVING_BOMB) == CAST_OK)
                    {
                        m_creature->SetInFront(pTarget);
                        m_creature->SetTargetGuid(pTarget->GetObjectGuid());
                        m_uiLivingBombTimer = urand(12000, 15000);
                        m_uiRestoreTargetTimer = 800;
                    }
                }
            }
            else
            {
                m_uiLivingBombTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_baron_geddonAI(pCreature);
    }
};

void AddSC_boss_baron_geddon()
{
    Script* s;
    s = new boss_baron_geddon();
    s->RegisterSelf();
}

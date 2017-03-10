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
 * SDName:      Boss_Garr
 * SD%Complete: 50
 * SDComment:   Garr's enrage is missing
 * SDCategory:  Molten Core
 * EndScriptData
 */

#include "precompiled.h"
#include "molten_core.h"

enum
{
    // Garr spells
    SPELL_ANTIMAGICPULSE        = 19492,
    SPELL_MAGMASHACKLES         = 19496,
    SPELL_ENRAGE                = 19516,

    // Add spells
    SPELL_IMMOLATE              = 20294,
    SPELL_THRASH                = 3391,
    SPELL_ADD_ERUPTION          = 19497,
    SPELL_MASSIVE_ERUPTION      = 20483,

    EMOTE_MASSIVE_ERUPTION      = -1409001
};

struct boss_garr : public CreatureScript
{
    boss_garr() : CreatureScript("boss_garr") {}

    struct boss_garrAI : public ScriptedAI
    {
        boss_garrAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        }

        uint32 m_uiAntiMagicPulseTimer;
        uint32 m_uiMagmaShacklesTimer;
        uint32 m_uiCheckAddsTimer;
        uint32 m_uiExplodeTimer;

        ScriptedInstance* m_pInstance;

        void Reset() override
        {
            m_uiAntiMagicPulseTimer = 25000;
            m_uiMagmaShacklesTimer  = 15000;
            m_uiCheckAddsTimer      = 2000;
            m_uiExplodeTimer        = urand(3000, 6000);

            if (m_pInstance && m_creature->IsAlive())
            {
                m_pInstance->SetData(TYPE_GARR, NOT_STARTED);
            }
        }

        void Aggro(Unit* /*pWho*/) override
        {
            if (m_pInstance)
            {
                if (m_pInstance->GetData(TYPE_GARR) != DONE)
                {
                    m_pInstance->SetData(TYPE_GARR, IN_PROGRESS);
                }
            }
        }

        void JustDied(Unit* /*pKiller*/) override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_GARR, DONE);
            }
        }

        void JustReachedHome() override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_GARR, FAIL);
            }
        }

        void DoMassiveEruption()
        {
            std::list<Creature*> LigesListe;
            GetCreatureListWithEntryInGrid(LigesListe, m_creature, NPC_FIRESWORN, 150.0f);
            uint32 numLiges = LigesListe.size();
            if (!numLiges)
            {
                return;
            }

            int32 explodeIdx = urand(0, numLiges-1);
            Creature* validCreature = NULL;
            std::list<Creature*>::iterator itr = LigesListe.begin();
            while (explodeIdx > 0 || !validCreature)
            {
                if (itr == LigesListe.end())
                    break;

                if ((*itr)->IsAlive())
                    validCreature = *itr;
                ++itr;
                --explodeIdx;
            }
            if (validCreature)
            { 
                DoScriptText(EMOTE_MASSIVE_ERUPTION, m_creature);
/*                // Garr can choose a banished add as one of his random selects, and it will fail
                if (!(validCreature->HasAura(18647) || validCreature->HasAura(710)))
                {
                    if (mob_fireswornAI* pFireswornAI = dynamic_cast<mob_fireswornAI*> (validCreature->AI()))
                    {
                        pFireswornAI->m_bCommanded = true;
                        validCreature->CastSpell(validCreature, SPELL_DEATHTOUCH, true);
                    }
                }
*/          }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            // AntiMagicPulse_Timer
            if (m_uiAntiMagicPulseTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_ANTIMAGICPULSE) == CAST_OK)
                {
                    m_uiAntiMagicPulseTimer = urand(10000, 15000);
                }
            }
            else
            {
                m_uiAntiMagicPulseTimer -= uiDiff;
            }

            // MagmaShackles_Timer
            if (m_uiMagmaShacklesTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_MAGMASHACKLES) == CAST_OK)
                {
                    m_uiMagmaShacklesTimer = urand(8000, 12000);
                }
            }
            else
            {
                m_uiMagmaShacklesTimer -= uiDiff;
            }

            if (m_creature->GetHealthPercent() < 50.0f)
            {
                if (m_uiExplodeTimer < uiDiff)
                {
                    DoMassiveEruption();
                    m_uiExplodeTimer = urand(10000, 20000);
                }
                else
                {
                    m_uiExplodeTimer -= uiDiff;
                }
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_garrAI(pCreature);
    }
};

struct mob_firesworn : public CreatureScript
{
    mob_firesworn() : CreatureScript("mob_firesworn") {}

    struct mob_fireswornAI : public ScriptedAI
    {
        mob_fireswornAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
            Reset();
        }

        ScriptedInstance* m_pInstance;

        uint32 m_uiImmolateTimer;
        uint32 m_uiAnxietyTimer;
        uint32 m_uiThrashTimer;
        bool m_bCommanded;

        void Reset()
        {
            m_uiImmolateTimer  = urand(4000, 7000);
            m_uiThrashTimer    = urand(10000, 15000);
            m_uiAnxietyTimer   = 10000;
            m_bCommanded       = false;
        }

        void Aggro(Unit* /*pWho*/) override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_GARR, IN_PROGRESS);
            }
            m_creature->SetInCombatWithZone();
        }

        void JustDied(Unit*)
        {
            // Buff Garr on death
            if (Creature* pGarr = GetClosestCreatureWithEntry(m_creature, NPC_GARR, 100.0f, true))
            {
                pGarr->CastSpell(pGarr, SPELL_ENRAGE, true);
            }

            if (m_bCommanded)
            {
                m_creature->CastSpell(m_creature, SPELL_MASSIVE_ERUPTION, true);
            }
            else
            {
                m_creature->CastSpell(m_creature, SPELL_ADD_ERUPTION, true);
            }
        }

        void UpdateAI(const uint32 uiDiff)
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            // Immolate
            // This should be a proc aura, need to define a reasonable % to proc in DB before implementing it that way
            if (m_uiImmolateTimer < uiDiff)
            {
                if (m_creature->CanReachWithMeleeAttack(m_creature->getVictim()))
                {
                    DoCastSpellIfCan(m_creature->getVictim(), SPELL_IMMOLATE);
                    m_uiImmolateTimer = urand(5000, 12000);
                    if (m_uiThrashTimer < 2000) m_uiThrashTimer += 2000 - m_uiThrashTimer;
                }
            }
            else
            {
                m_uiImmolateTimer -= uiDiff;
            }

            // Thrash
            // This should be a proc aura, need to define a reasonable % to proc in DB before implementing it that way
            if (m_uiThrashTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_THRASH) == CAST_OK)
                {
                    m_uiThrashTimer = urand(5000, 12000);
                    if (m_uiImmolateTimer < 2000) m_uiImmolateTimer += 2000 - m_uiImmolateTimer;
                }
            }
            else
            {
                m_uiThrashTimer -= uiDiff;
            }

            if (!m_creature->HasAura(SPELL_SEPARATION_ANXIETY))
            {
                if (Creature* pGarr = GetClosestCreatureWithEntry(m_creature, NPC_GARR, 100.0f, true))
                {
                    if (m_creature->GetDistance2d(pGarr) > 45.0f)
                    {
                        if (m_uiAnxietyTimer < uiDiff)
                        {
                            DoCastSpellIfCan(m_creature, SPELL_SEPARATION_ANXIETY);
                            m_uiAnxietyTimer = 5000;
                        }
                        else
                        {
                            m_uiAnxietyTimer -= uiDiff;
                        }
                    }
                }
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new mob_fireswornAI(pCreature);
    }
};

void AddSC_boss_garr()
{
    Script* s;
    s = new boss_garr();
    s->RegisterSelf();
    s = new mob_firesworn();
    s->RegisterSelf();
}

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
 * SDName:      Boss_Golemagg
 * SD%Complete: 80
 * SDComment:   Rager need to be tied to boss (Despawn on boss-death)
 * SDCategory:  Molten Core
 * EndScriptData
 */

#include "precompiled.h"
#include "molten_core.h"

enum
{
    SPELL_MAGMA_SPLASH      = 13879,
    SPELL_PYROBLAST         = 20228,
    SPELL_EARTHQUAKE        = 19798,
    SPELL_ENRAGE            = 19953,
    SPELL_GOLEMAGG_TRUST    = 20553,

    // Core Rager
    EMOTE_LOW_HP            = -1409002,
    SPELL_MANGLE            = 19820,
    SPELL_TRASH             = 3391
};

struct boss_golemagg : public CreatureScript
{
    boss_golemagg() : CreatureScript("boss_golemagg") {}

    struct boss_golemaggAI : public ScriptedAI
    {
        boss_golemaggAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        }

        ScriptedInstance* m_pInstance;

        uint32 m_uiPyroblastTimer;
        uint32 m_uiEarthquakeTimer;
        uint32 TickTimer;
        bool m_bEnraged;

        void Reset() override
        {
            m_uiPyroblastTimer  = 7000;
            m_uiEarthquakeTimer = 3000;
            TickTimer           = 10000;
            m_bEnraged          = false;

            if (m_pInstance && m_creature->IsAlive())
                m_pInstance->SetData(TYPE_GOLEMAGG, NOT_STARTED);

            std::list<Creature*> ChiensListe;
            GetCreatureListWithEntryInGrid(ChiensListe, m_creature, 11672, 150.0f);
            if (ChiensListe.empty() == false)
            {
                for (std::list<Creature*>::iterator itr = ChiensListe.begin(); itr != ChiensListe.end(); ++itr)
                {
                    if ((*itr)->GetDeathState() == ALIVE)
                        (*itr)->DealDamage((*itr), (*itr)->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                    (*itr)->Respawn();
                }
            }

            m_creature->CastSpell(m_creature, SPELL_MAGMA_SPLASH, true);
        }

        void Aggro(Unit* /*pWho*/) override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_GOLEMAGG, IN_PROGRESS);
            }
        }

        void JustDied(Unit* /*pKiller*/) override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_GOLEMAGG, DONE);
            }
        }

        void JustReachedHome() override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_GOLEMAGG, FAIL);
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            // Pyroblast
            if (m_uiPyroblastTimer < uiDiff)
            {
                if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                {
                    if (DoCastSpellIfCan(pTarget, SPELL_PYROBLAST) == CAST_OK)
                    {
                        m_uiPyroblastTimer = (3 + rand() % 4) * IN_MILLISECONDS;
                    }
                }
            }
            else
            {
                m_uiPyroblastTimer -= uiDiff;
            }

            // Enrage
            if (!m_bEnraged && m_creature->GetHealthPercent() < 10.0f)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_ENRAGE) == CAST_OK)
                {
                    m_bEnraged = true;
                }
            }

            if (TickTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_GOLEMAGG_TRUST) == CAST_OK)
                    TickTimer = 2000;
            }
            else
            {
                TickTimer -= uiDiff;
            }

            // Earthquake
            if (m_bEnraged)
            {
                if (m_uiEarthquakeTimer < uiDiff)
                {
                    if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_EARTHQUAKE) == CAST_OK)
                    {
                        m_uiEarthquakeTimer = 3000;
                    }
                }
                else
                {
                    m_uiEarthquakeTimer -= uiDiff;
                }
            }

            // Golemagg's Trust
            if (TickTimer < uiDiff)
            {
                DoCastSpellIfCan(m_creature, SPELL_GOLEMAGG_TRUST);
                TickTimer = 2000;
            }
            else
            {
                TickTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_golemaggAI(pCreature);
    }
};

struct mob_core_rager : public CreatureScript
{
    mob_core_rager() : CreatureScript("mob_core_rager") {}

    struct mob_core_ragerAI : public ScriptedAI
    {
        mob_core_ragerAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        }

        ScriptedInstance* m_pInstance;
        uint32 m_uiMangleTimer;
        uint32 TickTimer;

        void Reset() override
        {
            TickTimer       = 1000;
            m_uiMangleTimer = 7000;
        }

        void DamageTaken(Unit* /*pDoneBy*/, uint32& uiDamage) override
        {
            if (m_pInstance)
            {
                if (Creature* pGolemagg = m_pInstance->instance->GetCreature(m_pInstance->GetData64(DATA_GOLEMAGG)))
                {
                    if (pGolemagg->IsAlive())
                    {
                        if (m_creature->GetHealthPercent() < 50.0f)
                        {
                            DoScriptText(EMOTE_LOW_HP, m_creature);
                            m_creature->SetHealth(m_creature->GetMaxHealth());
                            uiDamage = 0;
                        }
                    }
                }
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            // Mangle
            if (m_uiMangleTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_MANGLE) == CAST_OK)
                {
                    m_uiMangleTimer = 10 * IN_MILLISECONDS;
                }
            }
            else
            {
                m_uiMangleTimer -= uiDiff;
            }

            if (TickTimer < uiDiff)
            {
                TickTimer = 1000;
                if (!m_creature->HasAura(SPELL_TRASH) && !(rand() % 10))
                {
                    m_creature->CastSpell(m_creature, SPELL_TRASH, true);
                }
            }
            else
            {
                TickTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new mob_core_ragerAI(pCreature);
    }
};

void AddSC_boss_golemagg()
{
    Script* s;
    s = new boss_golemagg();
    s->RegisterSelf();
    s = new mob_core_rager();
    s->RegisterSelf();
}

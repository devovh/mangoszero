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
 * SDName:      Boss_Magmadar
 * SD%Complete: 75
 * SDComment:   Lava bomb needs still core support
 * SDCategory:  Molten Core
 * EndScriptData
 */

#include "precompiled.h"
#include "molten_core.h"

enum
{
    EMOTE_GENERIC_FRENZY_KILL   = -1000001,

    SPELL_FRENZY                = 19451,
    SPELL_MAGMASPIT             = 19449,                    // This is actually a buff he gives himself
    SPELL_PANIC                 = 19408,
    SPELL_LAVABOMB              = 19411,                    // This calls a dummy server side effect that isn't implemented yet
    SPELL_LAVABOMB_ALT          = 19428,                    //This is the spell that the lava bomb casts
    SPELL_LAVA_BREATH           = 19272,

    NPC_LAVABOMB                = 20006,

    MODEL_INVISIBLE             = 11686
};

struct boss_magmadar : public CreatureScript
{
    boss_magmadar() : CreatureScript("boss_magmadar") {}

    struct boss_magmadarAI : public ScriptedAI
    {
        boss_magmadarAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        }

        ScriptedInstance* m_pInstance;

        uint32 m_uiFrenzyTimer;
        uint32 m_uiPanicTimer;
        uint32 m_uiLavaBombTimer;
        uint32 m_uiLavaBombTriggerTimer;
        uint32 m_uiLavaBreathTimer;
        uint32 m_uiMagmaSpitTimer;
        uint32 m_uiRestoreTargetTimer;

        void Reset() override
        {
            m_uiFrenzyTimer          = 15000;
            m_uiPanicTimer           = 10000;
            m_uiLavaBombTimer        = 13000;
            m_uiLavaBombTriggerTimer = 0;
            m_uiLavaBreathTimer      = 30000;
            m_uiMagmaSpitTimer       = 10000;
            m_uiRestoreTargetTimer   = 0;

            if (!m_creature->HasAura(SPELL_MAGMASPIT))
                m_creature->CastSpell(m_creature, SPELL_MAGMASPIT, true);

            if (m_pInstance && m_creature->IsAlive())
                m_pInstance->SetData(TYPE_MAGMADAR, NOT_STARTED);
        }

        void Aggro(Unit* /*pWho*/) override
        {
            DoCastSpellIfCan(m_creature, SPELL_MAGMASPIT, true);

            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_MAGMADAR, IN_PROGRESS);
            }
        }

        void JustDied(Unit* /*pKiller*/) override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_MAGMADAR, DONE);
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            // Frenzy_Timer
            if (m_uiFrenzyTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_FRENZY) == CAST_OK)
                {
                    DoScriptText(EMOTE_GENERIC_FRENZY_KILL, m_creature);
                    m_uiFrenzyTimer = urand(15000, 20000);
                }
            }
            else
            {
                m_uiFrenzyTimer -= uiDiff;
            }

            // Panic_Timer
            if (m_uiPanicTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_PANIC) == CAST_OK)
                {
                    m_uiPanicTimer = urand(30000, 35000);
                }
            }
            else
            {
                m_uiPanicTimer -= uiDiff;
            }

            // Lavabomb_Timer
            if (m_uiLavaBombTimer < uiDiff)
            {
                if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                {
                    if (DoCastSpellIfCan(pTarget, SPELL_LAVABOMB) == CAST_OK)
                    {
                        m_creature->SetInFront(pTarget);
                        m_creature->SetTargetGuid(pTarget->GetObjectGuid());
                        if (Creature* Cre = m_creature->SummonCreature(NPC_LAVABOMB,
                                            pTarget->GetPositionX(),
                                            pTarget->GetPositionY(),
                                            pTarget->GetPositionZ(),
                                            pTarget->GetOrientation(),
                                            TEMPSUMMON_TIMED_DESPAWN,
                                            30000))
                        {
                            //scale at 0 is necessary for spell animation
                            Cre->SetObjectScale(0.0f);
                            Cre->setFaction(m_creature->getFaction());
                        }
                        m_uiLavaBombTimer = urand(10000, 13000);
                        m_uiLavaBombTriggerTimer = 500;
                        m_uiRestoreTargetTimer = 800;
                    }
                }
            }
            else
            {
                m_uiLavaBombTimer -= uiDiff;
            }
    
            if (m_uiLavaBombTriggerTimer)
            {
                if (m_uiLavaBombTriggerTimer <= uiDiff)
                {
                    std::list<Creature*> CreListe;
                    GetCreatureListWithEntryInGrid(CreListe, m_creature, NPC_LAVABOMB, 100.0f);
                    for (std::list<Creature*>::iterator itr = CreListe.begin(); itr != CreListe.end(); ++itr)
                        (*itr)->CastSpell((*itr), SPELL_LAVABOMB_ALT, false);
                    m_uiLavaBombTriggerTimer = 1150;
                }
                else
                {
                    m_uiLavaBombTriggerTimer -= uiDiff;
                }
            }


            // restore target after casting a Lava Bomb
            if (m_uiRestoreTargetTimer)
            {
                if (m_uiRestoreTargetTimer <= uiDiff)
                {
                    m_creature->SetInFront(m_creature->getVictim());
                    m_creature->SetTargetGuid(m_creature->getVictim()->GetObjectGuid());
                    m_uiRestoreTargetTimer = 0;
                }
                else
                {
                    m_uiRestoreTargetTimer -= uiDiff;
                }
            }

            if (m_uiMagmaSpitTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), 19450) == CAST_OK)
                {
                    m_uiMagmaSpitTimer = urand(10000, 20000);
                }
            }
            else
            {
                m_uiMagmaSpitTimer -= uiDiff;
            }

            if (m_uiLavaBreathTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_LAVA_BREATH) == CAST_OK)
                {
                    m_uiLavaBreathTimer = urand(10000, 30000);
                }
            }
            else
            {
                m_uiLavaBreathTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }

        void UpdateAI_corpse(const uint32 uiDiff)
        {
            // continue activating Lava Bomb triggers while dead until they've all despawned
            if (m_uiLavaBombTriggerTimer)
            {
                if (m_uiLavaBombTriggerTimer <= uiDiff)
                {
                    bool m_bAllDespawned = true;

                    std::list<Creature*> CreListe;
                    GetCreatureListWithEntryInGrid(CreListe, m_creature, NPC_LAVABOMB, 100.0f);
                    for (std::list<Creature*>::iterator itr = CreListe.begin(); itr != CreListe.end(); ++itr)
                    {
                        (*itr)->CastSpell((*itr), SPELL_LAVABOMB_ALT, false);
                        m_bAllDespawned = false;
                    }
                    m_uiLavaBombTriggerTimer = m_bAllDespawned ? 0 : 1125;
                }
                else
                {
                    m_uiLavaBombTriggerTimer -= uiDiff;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_magmadarAI(pCreature);
    }
};

void AddSC_boss_magmadar()
{
    Script* s;
    s = new boss_magmadar();
    s->RegisterSelf();
}

/*
 * Copyright (C) 2016-2017 ProjectLegion <https://bitbucket.org/projectlegion/projectlegion/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Spell.h"
#include "throne_of_the_tides.h"

enum YellsErunak
{
    SAY_FREED   = 0,
    SAY_VICTORY = 1
};

enum YellsGhursha
{
    SAY_PHASE   = 0,
    SAY_KILL    = 1,
    SAY_DEATH   = 2,
    SAY_ENCLAVE = 3,
    SAY_FOG     = 4
};

enum Spells
{
    // Erunak Stonespeaker
    SPELL_EARTH_SHARDS              = 84931,
    SPELL_EARTH_SHARDS_AURA         = 84935,
    SPELL_EARTH_SHARDS_DMG          = 84945,
    SPELL_EARTH_SHARDS_DMG_H        = 91491,
    SPELL_EMBERSTRIKE               = 76165,
    SPELL_LAVA_BOLT                 = 76171,
    SPELL_LAVA_BOLT_H               = 91412,
    SPELL_MAGMA_SPLASH              = 76170,

    // Mindbender Ghur'sha
    SPELL_ENSLAVE                   = 76207,
    SPELL_ENSLAVE_BUFF              = 76213,
    SPELL_ABSORB_MAGIC              = 76307,
    SPELL_MIND_FOG                  = 76234,
    SPELL_MIND_FOG_AURA             = 76230,
    SPELL_MIND_FOG_VISUAL           = 76231,
    SPELL_UNRELENTING_AGONY         = 76339,
    SPELL_UNRELENTING_AGONY_DMG     = 76341,
    SPELL_UNRELENTING_AGONY_DMG_H   = 91493
};

enum Events
{
    EVENT_EARTH_SHARDS      = 1,
    EVENT_EMBERSTRIKE       = 2,
    EVENT_LAVA_BOLT         = 3,
    EVENT_MAGMA_SPLASH      = 4,
    EVENT_ENSLAVE           = 5,
    EVENT_ABSORB_MAGIC      = 6,
    EVENT_MIND_FOG          = 7,
    EVENT_UNRELENTING_AGONY = 8
};

enum Actions
{
    ACTION_GHURSHA_START    = 1
};

enum Adds
{
    NPC_EARTH_SHARDS    = 45469,
    NPC_MIND_FOG        = 40861
};

class boss_erunak_stonespeaker : public CreatureScript
{
    public:
        boss_erunak_stonespeaker() : CreatureScript("boss_erunak_stonespeaker") { }

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_erunak_stonespeakerAI(creature);
        }

        struct boss_erunak_stonespeakerAI : public ScriptedAI
        {
            boss_erunak_stonespeakerAI(Creature* creature) : ScriptedAI(creature), summons(me)
            {
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_STUN, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FEAR, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_ROOT, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FREEZE, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_POLYMORPH, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_HORROR, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SAPPED, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_CHARM, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_DISORIENTED, true);
                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_CONFUSE, true);
                instance = creature->GetInstanceScript();
            }

            void Reset() override
            {
                bPhase = false;
                events.Reset();
                summons.DespawnAll();
                if (instance)
                    if (instance->GetBossState(DATA_MINDBENDER_GHURSHA) == DONE || bPhase)
                        me->setFaction(35);
            }

            void KilledUnit(Unit* /*victim*/) override
            {
                if (instance)
                    if (Creature* Ghursha = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_MINDBENDER_GHURSHA)))
                        Ghursha->AI()->Talk(SAY_KILL);
            }

            void EnterCombat(Unit* /*who*/) override
            {
                events.ScheduleEvent(EVENT_EARTH_SHARDS, 8000);
                events.ScheduleEvent(EVENT_EMBERSTRIKE, 11000);
                events.ScheduleEvent(EVENT_LAVA_BOLT, 13000);
                events.ScheduleEvent(EVENT_MAGMA_SPLASH, 6000);
                if (instance)
                    instance->SetBossState(DATA_MINDBENDER_GHURSHA, IN_PROGRESS);
            }

            void JustSummoned(Creature* summon) override
            {
                if (me->IsInCombat())
                    summon->SetInCombatWithZone();
            }

            void SummonedCreatureDespawn(Creature* summon) override
            {
                summons.Despawn(summon);
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                if(me->GetHealthPct() < 50 && !bPhase)
                {
                    bPhase = true;
                    events.Reset();
                    me->setFaction(35);
                    EnterEvadeMode();
                    if (instance)
                    {
                        if (Creature* pGhursha = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_MINDBENDER_GHURSHA)))
                            pGhursha->AI()->DoAction(ACTION_GHURSHA_START);
                    }
                    return;
                }

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EVENT_EARTH_SHARDS:
                        if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                            DoCast(pTarget, SPELL_EARTH_SHARDS);
                        events.ScheduleEvent(EVENT_EARTH_SHARDS, 20000);
                        break;
                    case EVENT_EMBERSTRIKE:
                        DoCastVictim(SPELL_EMBERSTRIKE);
                        events.ScheduleEvent(EVENT_EMBERSTRIKE, 11000);
                        break;
                    case EVENT_LAVA_BOLT:
                        if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                            DoCast(pTarget, SPELL_LAVA_BOLT);
                        events.ScheduleEvent(EVENT_LAVA_BOLT, 14000);
                        break;
                    case EVENT_MAGMA_SPLASH:
                        DoCast(me, SPELL_MAGMA_SPLASH);
                        events.ScheduleEvent(EVENT_MAGMA_SPLASH, 13000);
                        break;
                    }
                }            
                DoMeleeAttackIfReady();
            }

        private:
            InstanceScript* instance;
            EventMap events;
            SummonList summons;

            bool bPhase;
        };
};

class boss_mindbender_ghursha : public CreatureScript
{
    public:
        boss_mindbender_ghursha() : CreatureScript("boss_mindbender_ghursha") { }

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_mindbender_ghurshaAI (creature);
        }

        struct boss_mindbender_ghurshaAI : public BossAI
        {
            boss_mindbender_ghurshaAI(Creature* creature) : BossAI(creature, DATA_MINDBENDER_GHURSHA)
            {
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_STUN, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FEAR, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_ROOT, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FREEZE, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_POLYMORPH, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_HORROR, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SAPPED, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_CHARM, true);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_DISORIENTED, true);
                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_CONFUSE, true);
            }

            void InitializeAI()
            {
                if (!instance || static_cast<InstanceMap*>(me->GetMap())->GetScriptId() != sObjectMgr->GetScriptId(TotTScriptName))
                    me->IsAIEnabled = false;
                else if (!me->isDead())
                    Reset();
            }

            void Reset() override
            {
                _Reset();

                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                events.Reset();
                if (Creature* pErunak = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_ERUNAK_STONESPEAKER)))
                    pErunak->AI()->EnterEvadeMode();
            }

            void DoAction(int32 action) override
            {
                if (action == ACTION_GHURSHA_START)
                {
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    if (Unit* pTarget = me->SelectNearestTarget(100.0f))
                        me->GetMotionMaster()->MoveChase(pTarget);
                }
            }

            void EnterCombat(Unit* /*who*/) override
            {
                events.ScheduleEvent(EVENT_ENSLAVE, 13000);
                events.ScheduleEvent(EVENT_ABSORB_MAGIC, 20000);
                events.ScheduleEvent(EVENT_MIND_FOG, urand(6000,12000));
                events.ScheduleEvent(EVENT_UNRELENTING_AGONY, 10000);
                Talk(SAY_PHASE);
                instance->SetBossState(DATA_MINDBENDER_GHURSHA, IN_PROGRESS);
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EVENT_ENSLAVE:
                        Talk(SAY_ENCLAVE);
                        events.ScheduleEvent(EVENT_ENSLAVE, 31000);
                        break;
                    case EVENT_ABSORB_MAGIC:
                        DoCast(me, SPELL_ABSORB_MAGIC);
                        events.ScheduleEvent(EVENT_ABSORB_MAGIC, 15000);
                        break;
                    case EVENT_MIND_FOG:
                        DoCast(me, SPELL_MIND_FOG);
                        events.ScheduleEvent(EVENT_MIND_FOG, urand(23000,25000));
                        break;
                    case EVENT_UNRELENTING_AGONY:
                        DoCast(me, SPELL_UNRELENTING_AGONY);
                        events.ScheduleEvent(EVENT_UNRELENTING_AGONY, 30000);
                        break;
                    }
                }
                DoMeleeAttackIfReady();
            }

            void KilledUnit(Unit* /*victim*/) override
            {
                Talk(SAY_KILL);
            }

            void JustDied(Unit* /*killer*/) override
            {
                _JustDied();
                Talk(SAY_DEATH);
                if (Creature* pErunak = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_ERUNAK_STONESPEAKER)))
                {
                    //pErunak->AI()->EnterEvadeMode();
                    pErunak->AI()->Talk(SAY_VICTORY);
                }
            }

        };
};

class npc_erunak_earth_shards : public CreatureScript
{
    public:
        npc_erunak_earth_shards() : CreatureScript("npc_erunak_earth_shards") { }

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_erunak_earth_shardsAI(creature);
        }

        struct npc_erunak_earth_shardsAI : public ScriptedAI
        {
            npc_erunak_earth_shardsAI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->SetReactState(REACT_PASSIVE);
            }

            void Reset() override
            {
                uiDespawnTimer = 5000;
                DoCast(me, SPELL_EARTH_SHARDS_AURA);
            }

            void UpdateAI(uint32 diff) override
            {
                if (uiDespawnTimer <= diff)
                    me->DespawnOrUnsummon();
                else
                    uiDespawnTimer -= diff;
            }

        private:
            uint32 uiDespawnTimer;
        };
};

class npc_ghursha_mind_fog : public CreatureScript
{
    public:
        npc_ghursha_mind_fog() : CreatureScript("npc_ghursha_mind_fog") { }

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_ghursha_mind_fogAI (creature);
        }

        struct npc_ghursha_mind_fogAI : public ScriptedAI
        {
            npc_ghursha_mind_fogAI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->SetReactState(REACT_PASSIVE);
            }

            void Reset() override
            {
                DoCast(me, SPELL_MIND_FOG_AURA, true);
                DoCast(me, SPELL_MIND_FOG_VISUAL, true);
            }

            void UpdateAI(uint32 /*diff*/) override { }
        };
};

void AddSC_boss_erunak_stonespeaker()
{
    new boss_erunak_stonespeaker();
    new boss_mindbender_ghursha();
    new npc_erunak_earth_shards();
    new npc_ghursha_mind_fog();
}
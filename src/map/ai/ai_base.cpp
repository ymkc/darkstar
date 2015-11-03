/*
===========================================================================

Copyright (c) 2010-2015 Darkstar Dev Teams

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see http://www.gnu.org/licenses/

This file is part of DarkStar-server source code.

===========================================================================
*/

#include "ai_base.h"

#include "states/despawn_state.h"
#include "../entities/baseentity.h"
#include "../packets/entity_animation.h"

CAIBase::CAIBase(CBaseEntity* _PEntity) :
    CAIBase(_PEntity, nullptr, nullptr)
{
}

CAIBase::CAIBase(CBaseEntity* _PEntity, std::unique_ptr<CPathFind>&& _pathfind,
    std::unique_ptr<CController>&& _controller) :
    m_Tick(server_clock::now()),
    m_PrevTick(server_clock::now()),
    PEntity(_PEntity),
    ActionQueue(_PEntity),
    PathFind(std::move(_pathfind)),
    Controller(std::move(_controller))
{
}

CState* CAIBase::GetCurrentState()
{
    if (!m_stateStack.empty())
    {
        return m_stateStack.top().get();
    }
    return nullptr;
}

bool CAIBase::CanChangeState()
{
    return !GetCurrentState() || GetCurrentState()->CanChangeState();
}

void CAIBase::Reset()
{
    if (PathFind)
    {
        PathFind->Clear();
    }
}

void CAIBase::Tick(time_point _tick)
{
    m_PrevTick = m_Tick;
    m_Tick = _tick;
    CBaseEntity* PreEntity = PEntity;
    
    //#TODO: check this in the controller instead maybe? (might not want to check every tick) - same for pathfind
    ActionQueue.checkAction(_tick);

    // check pathfinding
    if (PathFind)
    {
        PathFind->FollowPath();
    }

    if (Controller && Controller->canUpdate)
    {
        Controller->Tick(_tick);
    }

    while (!m_stateStack.empty() && m_stateStack.top()->DoUpdate(_tick))
    {
        m_stateStack.top()->Cleanup(_tick);
        m_stateStack.pop();
    }

    //make sure this AI hasn't been replaced by another
    if (PreEntity->updatemask && PreEntity->PAI.get() == this)
    {
        PreEntity->UpdateEntity();
    }
}

bool CAIBase::IsStateStackEmpty()
{
    return m_stateStack.empty();
}

void CAIBase::ClearStateStack()
{
    while (!m_stateStack.empty())
    {
        m_stateStack.pop();
    }
}

bool CAIBase::IsSpawned()
{
    return PEntity->status != STATUS_DISAPPEAR;
}

bool CAIBase::IsRoaming()
{
    return PEntity->animation == ANIMATION_NONE;
}

bool CAIBase::IsEngaged()
{
    return PEntity->animation == ANIMATION_ATTACK;
}

time_point CAIBase::getTick()
{
    return m_Tick;
}

time_point CAIBase::getPrevTick()
{
    return m_PrevTick;
}

void CAIBase::Despawn()
{
    if (Controller)
    {
        Controller->Despawn();
    }
    else
    {
        Internal_Despawn(0s);
    }
}

void CAIBase::QueueAction(queueAction_t&& action)
{
    ActionQueue.pushAction(std::move(action));
}

void CAIBase::Internal_Despawn(duration spawnTime)
{
    ChangeState<CDespawnState>(PEntity, spawnTime);
}

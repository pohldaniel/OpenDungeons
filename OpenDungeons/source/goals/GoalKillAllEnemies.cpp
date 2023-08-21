/*
 *  Copyright (C) 2011-2016  OpenDungeons Team
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "goals/GoalKillAllEnemies.h"

#include "entities/Creature.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "rooms/Room.h"
#include "rooms/RoomType.h"

#include <iostream>

GoalKillAllEnemies::GoalKillAllEnemies(const std::string& nName,
    const std::string& nArguments) :
    Goal(nName, nArguments)
{
}

bool GoalKillAllEnemies::isMet(const Seat &s, const GameMap& gameMap)
{
    // Loop over all the creatures in the game map and check to see if any of them are of a different color than our seat.
    for (Creature* creature : gameMap.getCreatures())
    {
        if (!creature->getSeat()->isAlliedSeat(&s))
            return false;
    }

    // Considers also creature spawner rooms as enemy to be killed.
    // Temples
    std::vector<Room*> temples = gameMap.getRoomsByType(RoomType::dungeonTemple);
    for (Room* temple : temples)
    {
        if (!temple->getSeat()->isAlliedSeat(&s))
            return false;
    }
    // Portals
    std::vector<Room*> portals = gameMap.getRoomsByType(RoomType::portal);
    for (Room* portal : portals)
    {
        if (!portal->getSeat()->isAlliedSeat(&s))
            return false;
    }

    return true;
}

std::string GoalKillAllEnemies::getSuccessMessage(const Seat&)
{
    return "You have killed all the enemy creatures,\ntemples and portals.";
}

std::string GoalKillAllEnemies::getFailedMessage(const Seat&)
{
    return "You have failed to kill all the enemy creatures,\ntemples and portals.";
}

std::string GoalKillAllEnemies::getDescription(const Seat&)
{
    return "Kill all enemy creatures,\ntemples and portals.";
}

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

#ifndef CREATUREACTIONSEARCHGROUNDTILETOCLAIM_H
#define CREATUREACTIONSEARCHGROUNDTILETOCLAIM_H

#include "creatureaction/CreatureAction.h"

class CreatureActionSearchGroundTileToClaim : public CreatureAction
{
public:
    CreatureActionSearchGroundTileToClaim(Creature& creature, bool forced);
    virtual ~CreatureActionSearchGroundTileToClaim();

    CreatureActionType getType() const override
    { return CreatureActionType::searchGroundTileToClaim; }

    std::function<bool()> action() override;

    static bool handleSearchGroundTileToClaim(Creature& creature, int32_t nbTurns, bool forced);

private:
    bool mForced;
};

#endif // CREATUREACTIONSEARCHGROUNDTILETOCLAIM_H

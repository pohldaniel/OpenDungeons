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

#ifndef TRAPDOOR_H
#define TRAPDOOR_H

#include "Trap.h"
#include "traps/TrapType.h"

class DoorEntity;

class TrapDoor : public Trap
{
public:
    TrapDoor(GameMap* gameMap);

    static const std::string ANIMATION_OPEN;
    static const std::string ANIMATION_CLOSE;

    const TrapType getType() const override
    { return TrapType::doorWooden; }

    bool isDoor() const override
    { return true; }

    // We return true to make sure every creature with vision on the door tile can see it
    bool shoot(Tile* tile) override
    { return true; }

    void doUpkeep() override;

    bool displayTileMesh() const override
    { return true; }

    //! \brief The trap object covers the whole tile under
    //! but while it built, the ground tile still must be shown.
    bool shouldDisplayGroundTile() const override
    { return true; }

    void notifyDoorSlapped(DoorEntity* doorEntity, Tile* tile);

    TrapEntity* getTrapEntity(Tile* tile) override;

    double getCreatureSpeed(const Creature* creature, Tile* tile) const override;

    bool permitsVision(Tile* tile) override;

    //! Returns true if tiles North and South (or east and west) are suitable to have a door on the
    //! given tile
    static bool canDoorBeOnTile(GameMap* gameMap, Tile* tile);

    static bool buildTrapOnTile(GameMap* gameMap, Player* player, Tile* tile);

    static const TrapType mTrapType;

protected:
    void exportToStream(std::ostream& os) const override;
    bool importFromStream(std::istream& is) override;

private:
    //! \brief Wanted state for the door (changes when the player slaps the door)
    bool mIsLocked;
    //! \brief Current state of the door
    bool mIsLockedState;

    void changeDoorState(DoorEntity* doorEntity, Tile* tile, bool locked);
};

#endif // TRAPDOOR_H


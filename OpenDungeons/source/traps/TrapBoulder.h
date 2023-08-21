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

#ifndef TRAPBOULDER_H
#define TRAPBOULDER_H

#include "traps/Trap.h"
#include "traps/TrapType.h"

class TrapBoulder : public Trap
{
public:
    TrapBoulder(GameMap* gameMap);

    virtual const TrapType getType() const override
    { return TrapType::boulder; }

    virtual bool shoot(Tile* tile) override;
    virtual bool isAttackable(Tile* tile, Seat* seat) const override
    {
        return false;
    }

    virtual bool displayTileMesh() const override
    { return true; }

    //! \brief The boulder trap should let the ground tile visible.
    virtual bool shouldDisplayGroundTile() const override
    { return true; }

    virtual TrapEntity* getTrapEntity(Tile* tile) override;

    static const TrapType mTrapType;
};

#endif // TRAPBOULDER_H

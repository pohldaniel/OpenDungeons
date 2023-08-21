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

#ifndef ROOMBRIDGEWOODEN_H
#define ROOMBRIDGEWOODEN_H

#include "rooms/RoomBridge.h"
#include "rooms/RoomType.h"

class Tile;

class RoomBridgeWooden: public RoomBridge
{
public:
    RoomBridgeWooden(GameMap* gameMap);

    virtual RoomType getType() const override
    { return mRoomType; }

    static const RoomType mRoomType;

protected:
    void updateFloodFillTileRemoved(Seat* seat, Tile* tile) override;
};

#endif // ROOMBRIDGEWOODEN_H

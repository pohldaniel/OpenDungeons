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

#include "rooms/Room.h"

#include "creatureaction/CreatureActionSearchJob.h"
#include "entities/BuildingObject.h"
#include "entities/Creature.h"
#include "entities/GameEntityType.h"
#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "modes/InputCommand.h"
#include "modes/InputManager.h"
#include "network/ODClient.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "rooms/RoomManager.h"
#include "rooms/RoomType.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/MakeUnique.h"

#include <istream>
#include <ostream>

Room::Room(GameMap* gameMap):
    Building(gameMap),
    mNumActiveSpots(0)
{
}

GameEntityType Room::getObjectType() const
{
    return GameEntityType::room;
}

bool Room::compareTile(Tile* tile1, Tile* tile2)
{
    if(tile1->getX() < tile2->getX())
        return true;

    if(tile1->getX() == tile2->getX())
        return (tile1->getY() < tile2->getY());

    return false;
}

void Room::addToGameMap()
{
    getGameMap()->addRoom(this);
    getGameMap()->addActiveObject(this);
}

void Room::removeFromGameMap()
{
    fireEntityRemoveFromGameMap();
    getGameMap()->removeRoom(this);
    setIsOnMap(false);
    for(Seat* seat : getGameMap()->getSeats())
    {
        for(Tile* tile : mCoveredTiles)
            seat->notifyBuildingRemovedFromGameMap(this, tile);
        for(Tile* tile : mCoveredTilesDestroyed)
            seat->notifyBuildingRemovedFromGameMap(this, tile);
    }

    removeAllBuildingObjects();
    getGameMap()->removeActiveObject(this);
}

void Room::absorbRoom(Room *r)
{
    OD_LOG_INF(getGameMap()->serverStr() + "Room=" + getName() + " is absorbing room=" + r->getName());

    mCentralActiveSpotTiles.insert(mCentralActiveSpotTiles.end(), r->mCentralActiveSpotTiles.begin(), r->mCentralActiveSpotTiles.end());
    r->mCentralActiveSpotTiles.clear();
    mLeftWallsActiveSpotTiles.insert(mLeftWallsActiveSpotTiles.end(), r->mLeftWallsActiveSpotTiles.begin(), r->mLeftWallsActiveSpotTiles.end());
    r->mLeftWallsActiveSpotTiles.clear();
    mRightWallsActiveSpotTiles.insert(mRightWallsActiveSpotTiles.end(), r->mRightWallsActiveSpotTiles.begin(), r->mRightWallsActiveSpotTiles.end());
    r->mRightWallsActiveSpotTiles.clear();
    mTopWallsActiveSpotTiles.insert(mTopWallsActiveSpotTiles.end(), r->mTopWallsActiveSpotTiles.begin(), r->mTopWallsActiveSpotTiles.end());
    r->mTopWallsActiveSpotTiles.clear();
    mBottomWallsActiveSpotTiles.insert(mBottomWallsActiveSpotTiles.end(), r->mBottomWallsActiveSpotTiles.begin(), r->mBottomWallsActiveSpotTiles.end());
    r->mBottomWallsActiveSpotTiles.clear();
    mNumActiveSpots += r->mNumActiveSpots;

    // Every creature working in this room should go to the new one (this is used in the server map only)
    // To do that, we notify the new room that a creature was using an absorbed one. It is
    // the room responsibility to handle correctly the creatures (making them stop to use the old
    // room if needed)
    // Note that we iterate a copy of r->mCreaturesUsingRoom because handleCreatureUsingAbsorbedRoom
    // will want to make the creature stop using the room and it may break the iteration
    std::vector<Creature*> creaturesUsingRoom = r->mCreaturesUsingRoom;
    for(Creature* creature : creaturesUsingRoom)
        handleCreatureUsingAbsorbedRoom(*creature);

    mCreaturesUsingRoom.insert(mCreaturesUsingRoom.end(), r->mCreaturesUsingRoom.begin(), r->mCreaturesUsingRoom.end());
    r->mCreaturesUsingRoom.clear();

    mBuildingObjects.insert(r->mBuildingObjects.begin(), r->mBuildingObjects.end());
    r->mBuildingObjects.clear();

    // We consider that the new room will be composed with the covered tiles it uses + the covered tiles absorbed. In the
    // absorbed room, we consider all tiles as destroyed. It will get removed from gamemap when enemy vision will be cleared
    for(Tile* tile : r->mCoveredTiles)
    {
        mCoveredTiles.push_back(tile);
        TileData* tileData = r->mTileData[tile];
        mTileData[tile] = tileData->cloneTileData();
        tileData->mHP = 0.0;
        tile->setCoveringBuilding(this);
    }

    r->mCoveredTilesDestroyed.insert(r->mCoveredTilesDestroyed.end(), r->mCoveredTiles.begin(), r->mCoveredTiles.end());
    r->mCoveredTiles.clear();

    // We fire the dead event so that if there are creatures heading for this room or
    // whatever, we release them before the remove from gamemap event
    r->fireEntityDead();
}

void Room::handleCreatureUsingAbsorbedRoom(Creature& creature)
{
    // If the job room is absorbed, we force the creatures working in the old rooms to search
    // a job. If there is space in the new one, they will use it. If not, they
    // will do something else
    creature.clearDestinations(EntityAnimation::idle_anim, true, true);
    creature.clearActionQueue();
    creature.pushAction(Utils::make_unique<CreatureActionSearchJob>(creature, true));
}

void Room::reorderRoomTiles(std::vector<Tile*>& tiles)
{
    // We try to keep the same tile disposition as if the room was created like this in the first
    // place to make sure building objects are disposed the same way
    std::sort(tiles.begin(), tiles.end(), Room::compareTile);
}

bool Room::addCreatureUsingRoom(Creature* c)
{
    if(!hasOpenCreatureSpot(c))
        return false;

    mCreaturesUsingRoom.push_back(c);
    return true;
}

void Room::removeCreatureUsingRoom(Creature *c)
{
    for (unsigned int i = 0; i < mCreaturesUsingRoom.size(); ++i)
    {
        if (mCreaturesUsingRoom[i] == c)
        {
            mCreaturesUsingRoom.erase(mCreaturesUsingRoom.begin() + i);
            break;
        }
    }
}

Creature* Room::getCreatureUsingRoom(unsigned index)
{
    if (index >= mCreaturesUsingRoom.size())
        return nullptr;

    return mCreaturesUsingRoom[index];
}

std::string Room::getRoomStreamFormat()
{
    return "typeRoom\tname\tseatId\tnumTiles\t\tSubsequent Lines: tileX\ttileY";
}

void Room::setupRoom(const std::string& name, Seat* seat, const std::vector<Tile*>& tiles)
{
    setIsOnMap(true);
    setName(name);
    setSeat(seat);
    for(Tile* tile : tiles)
    {
        mCoveredTiles.push_back(tile);
        TileData* tileData = createTileData(tile);
        mTileData[tile] = tileData;
        tileData->mHP = DEFAULT_TILE_HP;

        tile->setCoveringBuilding(this);
    }
}

void Room::checkForRoomAbsorbtion()
{
    bool isRoomAbsorbed = false;
    for (Tile* tile : getGameMap()->tilesBorderedByRegion(getCoveredTiles()))
    {
        Room* room = tile->getCoveringRoom();
        if(room == nullptr)
            continue;
        if(room == this)
            continue;
        if(room->getSeat() != getSeat())
            continue;
        if(room->getType() != getType())
            continue;

        absorbRoom(room);
        // All the tiles from the absorbed room have been transferred to this one
        // No need to delete it since it will be removed during its next upkeep
        isRoomAbsorbed = true;
    }

    if(isRoomAbsorbed)
        reorderRoomTiles(mCoveredTiles);
}

void Room::updateActiveSpots()
{
    std::vector<Tile*> centralActiveSpotTiles;
    std::vector<Tile*> leftWallsActiveSpotTiles;
    std::vector<Tile*> rightWallsActiveSpotTiles;
    std::vector<Tile*> topWallsActiveSpotTiles;
    std::vector<Tile*> bottomWallsActiveSpotTiles;

    // Detect the centers of 3x3 squares tiles
    for(unsigned int i = 0, size = mCoveredTiles.size(); i < size; ++i)
    {
        bool foundTop = false;
        bool foundTopLeft = false;
        bool foundTopRight = false;
        bool foundLeft = false;
        bool foundRight = false;
        bool foundBottomLeft = false;
        bool foundBottomRight = false;
        bool foundBottom = false;
        Tile* tile = mCoveredTiles[i];
        int tileX = tile->getX();
        int tileY = tile->getY();

        // Check all other tiles to know whether we have one center tile.
        for(unsigned int j = 0; j < size; ++j)
        {
            // Don't check the tile itself
            if (tile == mCoveredTiles[j])
                continue;

            // Check whether the tile around the tile checked is already a center spot
            // as we can't have two center spots next to one another.
            if (std::find(centralActiveSpotTiles.begin(), centralActiveSpotTiles.end(), mCoveredTiles[j]) != centralActiveSpotTiles.end())
                continue;

            int tile2X = mCoveredTiles[j]->getX();
            int tile2Y = mCoveredTiles[j]->getY();

            if(tile2X == tileX - 1)
            {
                if (tile2Y == tileY + 1)
                    foundTopLeft = true;
                else if (tile2Y == tileY)
                    foundLeft = true;
                else if (tile2Y == tileY - 1)
                    foundBottomLeft = true;
            }
            else if(tile2X == tileX)
            {
                if (tile2Y == tileY + 1)
                    foundTop = true;
                else if (tile2Y == tileY - 1)
                    foundBottom = true;
            }
            else if(tile2X == tileX + 1)
            {
                if (tile2Y == tileY + 1)
                    foundTopRight = true;
                else if (tile2Y == tileY)
                    foundRight = true;
                else if (tile2Y == tileY - 1)
                    foundBottomRight = true;
            }
        }

        // Check whether we found a tile at the center of others
        if (foundTop && foundTopLeft && foundTopRight && foundLeft && foundRight
                && foundBottomLeft && foundBottomRight && foundBottom)
        {
            centralActiveSpotTiles.push_back(tile);
        }
    }

    // Now that we've got the center tiles, we can test the tile around for walls.
    for (unsigned int i = 0, size = centralActiveSpotTiles.size(); i < size; ++i)
    {
        Tile* centerTile = centralActiveSpotTiles[i];
        if (centerTile == nullptr)
            continue;

        // Test for walls around
        // Up
        Tile* testTile;
        testTile = getGameMap()->getTile(centerTile->getX(), centerTile->getY() + 2);
        if (testTile != nullptr && testTile->isWallClaimedForSeat(getSeat()))
        {
            Tile* topTile = getGameMap()->getTile(centerTile->getX(), centerTile->getY() + 1);
            if (topTile != nullptr)
                topWallsActiveSpotTiles.push_back(topTile);
        }
        // Up for 4 tiles wide room
        testTile = getGameMap()->getTile(centerTile->getX(), centerTile->getY() + 3);
        if (testTile != nullptr && testTile->isWallClaimedForSeat(getSeat()))
        {
            bool isFound = true;
            for(int k = 0; k < 3; ++k)
            {
                Tile* testTile2 = getGameMap()->getTile(centerTile->getX() + k - 1, centerTile->getY() + 2);
                if((testTile2 == nullptr) || (testTile2->getCoveringBuilding() != this))
                {
                    isFound = false;
                    break;
                }
            }

            if(isFound)
            {
                Tile* topTile = getGameMap()->getTile(centerTile->getX(), centerTile->getY() + 2);
                topWallsActiveSpotTiles.push_back(topTile);
            }
        }

        // Down
        testTile = getGameMap()->getTile(centerTile->getX(), centerTile->getY() - 2);
        if (testTile != nullptr && testTile->isWallClaimedForSeat(getSeat()))
        {
            Tile* bottomTile = getGameMap()->getTile(centerTile->getX(), centerTile->getY() - 1);
            if (bottomTile != nullptr)
                bottomWallsActiveSpotTiles.push_back(bottomTile);
        }
        // Down for 4 tiles wide room
        testTile = getGameMap()->getTile(centerTile->getX(), centerTile->getY() - 3);
        if (testTile != nullptr && testTile->isWallClaimedForSeat(getSeat()))
        {
            bool isFound = true;
            for(int k = 0; k < 3; ++k)
            {
                Tile* testTile2 = getGameMap()->getTile(centerTile->getX() + k - 1, centerTile->getY() - 2);
                if((testTile2 == nullptr) || (testTile2->getCoveringBuilding() != this))
                {
                    isFound = false;
                    break;
                }
            }

            if(isFound)
            {
                Tile* topTile = getGameMap()->getTile(centerTile->getX(), centerTile->getY() - 2);
                bottomWallsActiveSpotTiles.push_back(topTile);
            }
        }

        // Left
        testTile = getGameMap()->getTile(centerTile->getX() - 2, centerTile->getY());
        if (testTile != nullptr && testTile->isWallClaimedForSeat(getSeat()))
        {
            Tile* leftTile = getGameMap()->getTile(centerTile->getX() - 1, centerTile->getY());
            if (leftTile != nullptr)
                leftWallsActiveSpotTiles.push_back(leftTile);
        }
        // Left for 4 tiles wide room
        testTile = getGameMap()->getTile(centerTile->getX() - 3, centerTile->getY());
        if (testTile != nullptr && testTile->isWallClaimedForSeat(getSeat()))
        {
            bool isFound = true;
            for(int k = 0; k < 3; ++k)
            {
                Tile* testTile2 = getGameMap()->getTile(centerTile->getX() - 2, centerTile->getY() + k - 1);
                if((testTile2 == nullptr) || (testTile2->getCoveringBuilding() != this))
                {
                    isFound = false;
                    break;
                }
            }

            if(isFound)
            {
                Tile* topTile = getGameMap()->getTile(centerTile->getX() - 2, centerTile->getY());
                leftWallsActiveSpotTiles.push_back(topTile);
            }
        }

        // Right
        testTile = getGameMap()->getTile(centerTile->getX() + 2, centerTile->getY());
        if (testTile != nullptr && testTile->isWallClaimedForSeat(getSeat()))
        {
            Tile* rightTile = getGameMap()->getTile(centerTile->getX() + 1, centerTile->getY());
            if (rightTile != nullptr)
                rightWallsActiveSpotTiles.push_back(rightTile);
        }
        // Right for 4 tiles wide room
        testTile = getGameMap()->getTile(centerTile->getX() + 3, centerTile->getY());
        if (testTile != nullptr && testTile->isWallClaimedForSeat(getSeat()))
        {
            bool isFound = true;
            for(int k = 0; k < 3; ++k)
            {
                Tile* testTile2 = getGameMap()->getTile(centerTile->getX() + 2, centerTile->getY() + k - 1);
                if((testTile2 == nullptr) || (testTile2->getCoveringBuilding() != this))
                {
                    isFound = false;
                    break;
                }
            }

            if(isFound)
            {
                Tile* topTile = getGameMap()->getTile(centerTile->getX() + 2, centerTile->getY());
                rightWallsActiveSpotTiles.push_back(topTile);
            }
        }
    }

    activeSpotCheckChange(activeSpotCenter, mCentralActiveSpotTiles, centralActiveSpotTiles);
    activeSpotCheckChange(activeSpotLeft, mLeftWallsActiveSpotTiles, leftWallsActiveSpotTiles);
    activeSpotCheckChange(activeSpotRight, mRightWallsActiveSpotTiles, rightWallsActiveSpotTiles);
    activeSpotCheckChange(activeSpotTop, mTopWallsActiveSpotTiles, topWallsActiveSpotTiles);
    activeSpotCheckChange(activeSpotBottom, mBottomWallsActiveSpotTiles, bottomWallsActiveSpotTiles);

    mCentralActiveSpotTiles = centralActiveSpotTiles;
    mLeftWallsActiveSpotTiles = leftWallsActiveSpotTiles;
    mRightWallsActiveSpotTiles = rightWallsActiveSpotTiles;
    mTopWallsActiveSpotTiles = topWallsActiveSpotTiles;
    mBottomWallsActiveSpotTiles = bottomWallsActiveSpotTiles;

    // Updates the number of active spots
    mNumActiveSpots = mCentralActiveSpotTiles.size()
                      + mLeftWallsActiveSpotTiles.size() + mRightWallsActiveSpotTiles.size()
                      + mTopWallsActiveSpotTiles.size() + mBottomWallsActiveSpotTiles.size();
}

void Room::activeSpotCheckChange(ActiveSpotPlace place, const std::vector<Tile*>& originalSpotTiles,
    const std::vector<Tile*>& newSpotTiles)
{
    // We create the non existing tiles
    for(std::vector<Tile*>::const_iterator it = newSpotTiles.begin(); it != newSpotTiles.end(); ++it)
    {
        Tile* tile = *it;
        if(std::find(originalSpotTiles.begin(), originalSpotTiles.end(), tile) == originalSpotTiles.end())
        {
            // The tile do not exist
            BuildingObject* ro = notifyActiveSpotCreated(place, tile);
            if(ro != nullptr)
            {
                // The room wants to build a room onject. We add it to the gamemap
                addBuildingObject(tile, ro);
                ro->createMesh();
            }
        }
    }
    // We remove the suppressed tiles
    for(std::vector<Tile*>::const_iterator it = originalSpotTiles.begin(); it != originalSpotTiles.end(); ++it)
    {
        Tile* tile = *it;
        if(std::find(newSpotTiles.begin(), newSpotTiles.end(), tile) == newSpotTiles.end())
        {
            // The tile has been removed
            notifyActiveSpotRemoved(place, tile);
        }
    }
}

BuildingObject* Room::notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile)
{
    return nullptr;
}

void Room::notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile)
{
    removeBuildingObject(tile);
}

void Room::exportHeadersToStream(std::ostream& os) const
{
    os << getType() << "\t";
}

void Room::exportTileDataToStream(std::ostream& os, Tile* tile, TileData* tileData) const
{
    if(getGameMap()->isInEditorMode())
        return;

    os << "\t" << tileData->mHP;

    // We only save enemy seats that have vision on the building
    std::vector<Seat*> seatsToSave;
    for(Seat* seat : tileData->mSeatsVision)
    {
        if(getSeat()->isAlliedSeat(seat))
            continue;

        seatsToSave.push_back(seat);
    }
    uint32_t nbSeatsVision = seatsToSave.size();
    os << "\t" << nbSeatsVision;
    for(Seat* seat : seatsToSave)
        os << "\t" << seat->getId();
}

bool Room::importTileDataFromStream(std::istream& is, Tile* tile, TileData* tileData)
{
    if(is.eof())
    {
        // Default initialization
        tileData->mHP = DEFAULT_TILE_HP;
        mCoveredTiles.push_back(tile);
        tile->setCoveringBuilding(this);
        return true;
    }

    // We read saved state
    if(!(is >> tileData->mHP))
        return false;

    if(tileData->mHP > 0.0)
    {
        mCoveredTiles.push_back(tile);
        tile->setCoveringBuilding(this);
    }
    else
    {
        mCoveredTilesDestroyed.push_back(tile);
    }

    GameMap* gameMap = getGameMap();
    uint32_t nbSeatsVision;
    if(!(is >> nbSeatsVision))
        return false;
    while(nbSeatsVision > 0)
    {
        --nbSeatsVision;
        int seatId;
        if(!(is >> seatId))
            return false;

        Seat* seat = gameMap->getSeatById(seatId);
        if(seat == nullptr)
        {
            OD_LOG_ERR("room=" + getName() + ", seatId=" + Helper::toString(seatId));
            continue;
        }

        tileData->mSeatsVision.push_back(seat);
    }

    return true;
}

void Room::restoreInitialEntityState()
{
    // We restore the vision if we need to
    std::map<Seat*, std::vector<Tile*>> tiles;
    for(std::pair<Tile* const, TileData*>& p : mTileData)
    {
        if(p.second->mSeatsVision.empty())
            continue;
        for(Seat* seat : p.second->mSeatsVision)
        {
            seat->setVisibleBuildingOnTile(this, p.first);
            tiles[seat].push_back(p.first);
        }
    }

    // We notify the clients
    for(std::pair<Seat* const, std::vector<Tile*>>& p : tiles)
    {
        if(p.first->getPlayer() == nullptr)
            continue;
        if(!p.first->getPlayer()->getIsHuman())
            continue;

        ServerNotification *serverNotification = new ServerNotification(
            ServerNotificationType::refreshTiles, p.first->getPlayer());
        std::vector<Tile*>& tilesRefresh = p.second;
        uint32_t nbTiles = tilesRefresh.size();
        serverNotification->mPacket << nbTiles;
        for(Tile* tile : tilesRefresh)
        {
            getGameMap()->tileToPacket(serverNotification->mPacket, tile);
            tile->exportToPacketForUpdate(serverNotification->mPacket, p.first);
        }
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
}

bool Room::canBeRepaired() const
{
    switch(getType())
    {
        case RoomType::dungeonTemple:
        case RoomType::portal:
            return false;
        default:
            break;
    }

    for(Tile* tile : mCoveredTilesDestroyed)
    {
        // We check if the tiles can be repaired
        if(!tile->isBuildableUpon(getSeat()))
            continue;

        return true;
    }

    return false;
}

int Room::getCostRepair()
{
    if(!canBeRepaired())
        return 0;

    int nbTiles = 0;
    for(Tile* tile : mCoveredTilesDestroyed)
    {
        // We check if the tiles can be repaired
        if(!tile->isBuildableUpon(getSeat()))
            continue;

        ++nbTiles;
    }
    return nbTiles * RoomManager::costPerTile(getType());
}

void Room::repairRoom()
{
    if(!canBeRepaired())
        return;

    while(!mCoveredTilesDestroyed.empty())
    {
        Tile* tile = mCoveredTilesDestroyed.back();
        mCoveredTilesDestroyed.pop_back();

        // We check if the tiles can be repaired
        if(!tile->isBuildableUpon(getSeat()))
            continue;

        OD_LOG_INF("Repairing room=" + getName() + ", tile=" + Tile::displayAsString(tile));

        mCoveredTiles.push_back(tile);
        TileData* tileData;
        auto it = mTileData.find(tile);
        if(it != mTileData.end())
            tileData = it->second;
        else
        {
            tileData = createTileData(tile);
            mTileData[tile] = tileData;
        }
        tileData->mHP = DEFAULT_TILE_HP;

        tile->setCoveringBuilding(this);
    }

    updateActiveSpots();
}

bool Room::sortForMapSave(Room* r1, Room* r2)
{
    // We sort room by seat id then meshName
    int seatId1 = r1->getSeat()->getId();
    int seatId2 = r2->getSeat()->getId();
    if(seatId1 != seatId2)
        return seatId1 < seatId2;

    if(r1->getType() != r2->getType())
        return static_cast<uint32_t>(r1->getType()) < static_cast<uint32_t>(r2->getType());

    return r1->getName().compare(r2->getName()) < 0;
}

void Room::fireRoomSound(Tile& tile, const std::string& soundFamily)
{
    std::string sound = "Rooms/" + soundFamily;
    for(Seat* seat : tile.getSeatsWithVision())
    {
        if(seat->getPlayer() == nullptr)
            continue;
        if(!seat->getPlayer()->getIsHuman())
            continue;

        ServerNotification *serverNotification = new ServerNotification(
            ServerNotificationType::playSpatialSound, seat->getPlayer());
        serverNotification->mPacket << sound << tile.getX() << tile.getY();
        ODServer::getSingleton().queueServerNotification(serverNotification);
    }
}

bool Room::importRoomFromStream(Room& room, std::istream& is)
{
    return room.importFromStream(is);
}

void Room::creatureDropped(Creature& creature)
{
    creature.pushAction(Utils::make_unique<CreatureActionSearchJob>(creature, true));
}

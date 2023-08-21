/*!
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

#include "ai/KeeperAI.h"

#include "creatureaction/CreatureAction.h"
#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "entities/Tile.h"
#include "game/Player.h"
#include "game/SkillManager.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "rooms/Room.h"
#include "rooms/RoomManager.h"
#include "rooms/RoomType.h"
#include "spells/SpellSummonWorker.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/Random.h"

#include <vector>

// Contains the rooms the AI will try to build. It will try to build them in the given order
static const std::vector<RoomType> wantedBuildings = {
    RoomType::dormitory,
    RoomType::treasury,
    RoomType::hatchery,
    RoomType::library,
    RoomType::trainingHall,
    RoomType::workshop,
    RoomType::arena,
    RoomType::crypt
};


KeeperAI::KeeperAI(GameMap& gameMap, Player& player, int cooldownDefenseMin, int cooldownDefenseMax,
             int cooldownSaveWoundedCreaturesMin, int cooldownSaveWoundedCreaturesMax,
             int cooldownLookingForRoomsMin, int cooldownLookingForRoomsMax):
    BaseAI(gameMap, player),
    mCooldownCheckTreasury(0),
    mCooldownLookingForRooms(0),
    mCooldownLookingForRoomsMin(cooldownLookingForRoomsMin),
    mCooldownLookingForRoomsMax(cooldownLookingForRoomsMax),
    mRoomPosX(-1),
    mRoomPosY(-1),
    mRoomSize(-1),
    mNoMoreReachableGold(false),
    mCooldownLookingForGold(0),
    mCooldownDefense(0),
    mCooldownDefenseMin(cooldownDefenseMin),
    mCooldownDefenseMax(cooldownDefenseMax),
    mCooldownWorkers(0),
    mCooldownRepairRooms(0),
    mCooldownSaveWoundedCreatures(0),
    mCooldownSaveWoundedCreaturesMin(cooldownSaveWoundedCreaturesMin),
    mCooldownSaveWoundedCreaturesMax(cooldownSaveWoundedCreaturesMax),
    mIsFirstUpkeepDone(false)
{
}

bool KeeperAI::doTurn(double timeSinceLastTurn)
{
    // If we have no dungeon temple, we are dead
    if(getDungeonTemple() == nullptr)
        return false;

    if(!mIsFirstUpkeepDone)
    {
        mIsFirstUpkeepDone = true;
        handleFirstTurn();
    }

    saveWoundedCreatures();

    handleDefense();

    if (handleWorkers())
        return true;

    if (checkTreasury())
        return true;

    if (handleRooms())
        return true;

    if (lookForGold())
        return true;

    if (repairRooms())
        return true;

    if(handleTiredCreatures())
        return true;

    if(handleHungryCreatures())
        return true;

    return true;
}

bool KeeperAI::checkTreasury()
{
    // If the treasury gets destroyed, we don't want the AI to build each turn the
    // free treasury
    if(mCooldownCheckTreasury > 0)
    {
        --mCooldownCheckTreasury;
        return false;
    }
    mCooldownCheckTreasury = Random::Int(10,30);

    int totalGold = 0;
    int totalStorage = 0;
    for(Room* room : mGameMap.getRooms())
    {
        if(room->getSeat() != mPlayer.getSeat())
            continue;

        totalGold += room->getTotalGoldStored();
        totalStorage += room->getTotalGoldStorage();
    }

    // We want at least to be allowed to store 3000 gold
    if(totalStorage >= 3000)
        return false;

    // The treasury is too small, we try to increase it
    // A treasury can be built if we have none (in this case, it is free). Otherwise,
    // we check if we have enough gold
    if((totalStorage > 0) && (totalGold < RoomManager::costPerTile(RoomType::treasury)))
        return false;

    Tile* central = getDungeonTemple()->getCentralTile();

    Creature* worker = mGameMap.getWorkerForPathFinding(mPlayer.getSeat());
    if (worker == nullptr)
        return false;

    // We try in priority to gold next to an existing treasury
    std::vector<Room*> treasuriesOwned = mGameMap.getRoomsByTypeAndSeat(RoomType::treasury, mPlayer.getSeat());
    for(Room* treasury : treasuriesOwned)
    {
        for(Tile* tile : treasury->getCoveredTiles())
        {
            for(Tile* neigh : tile->getAllNeighbors())
            {
                if(neigh->isBuildableUpon(mPlayer.getSeat()) &&
                   mGameMap.pathExists(worker, central, neigh))
                {
                    std::vector<Tile*> tiles;
                    tiles.push_back(neigh);

                    if(!RoomManager::buildRoomOnTiles(&mGameMap, RoomType::treasury, &mPlayer, tiles))
                        return false;

                    return true;
                }
            }
        }
    }

    int widerSide = mGameMap.getMapSizeX() > mGameMap.getMapSizeY() ?
        mGameMap.getMapSizeX() : mGameMap.getMapSizeY();

    // If we have found no tile available to an existing treasury, we search for the closest
    // buildable claimed tile available
    Tile* firstAvailableTile = nullptr;
    for(int32_t distance = 1; distance < widerSide; ++distance)
    {
        for(int k = 0; k <= distance; ++k)
        {
            Tile* t;
            // North-East
            t = mGameMap.getTile(central->getX() + k, central->getY() + distance);
            if(t != nullptr && t->isBuildableUpon(mPlayer.getSeat()) &&
               mGameMap.pathExists(worker, central, t))
            {
                firstAvailableTile = t;
                break;
            }
            // North-West
            if(k > 0)
            {
                t = mGameMap.getTile(central->getX() - k, central->getY() + distance);
                if(t != nullptr && t->isBuildableUpon(mPlayer.getSeat()) &&
                   mGameMap.pathExists(worker, central, t))
                {
                    firstAvailableTile = t;
                    break;
                }
            }
            // South-East
            t = mGameMap.getTile(central->getX() + k, central->getY() - distance);
            if(t != nullptr && t->isBuildableUpon(mPlayer.getSeat()) &&
               mGameMap.pathExists(worker, central, t))
            {
                firstAvailableTile = t;
                break;
            }
            // South-West
            if(k > 0)
            {
                t = mGameMap.getTile(central->getX() - k, central->getY() - distance);
                if(t != nullptr && t->isBuildableUpon(mPlayer.getSeat()) &&
                   mGameMap.pathExists(worker, central, t))
                {
                    firstAvailableTile = t;
                    break;
                }
            }
            // East-North
            t = mGameMap.getTile(central->getX() + distance, central->getY() + k);
            if(t != nullptr && t->isBuildableUpon(mPlayer.getSeat()) &&
               mGameMap.pathExists(worker, central, t))
            {
                firstAvailableTile = t;
                break;
            }
            // East-South
            if(k > 0)
            {
                t = mGameMap.getTile(central->getX() + distance, central->getY() - k);
                if(t != nullptr && t->isBuildableUpon(mPlayer.getSeat()) &&
                   mGameMap.pathExists(worker, central, t))
                {
                    firstAvailableTile = t;
                    break;
                }
            }
            // West-North
            t = mGameMap.getTile(central->getX() - distance, central->getY() + k);
            if(t != nullptr && t->isBuildableUpon(mPlayer.getSeat()) &&
               mGameMap.pathExists(worker, central, t))
            {
                firstAvailableTile = t;
                break;
            }
            // West-South
            if(k > 0)
            {
                t = mGameMap.getTile(central->getX() - distance, central->getY() - k);
                if(t != nullptr && t->isBuildableUpon(mPlayer.getSeat()) &&
                   mGameMap.pathExists(worker, central, t))
                {
                    firstAvailableTile = t;
                    break;
                }
            }
        }

        if(firstAvailableTile != nullptr)
            break;
    }

    // We couldn't find any available tile T_T
    // We return true to avoid doing something else to let workers claim
    if(firstAvailableTile == nullptr)
        return true;

    std::vector<Tile*> tiles;
    tiles.push_back(firstAvailableTile);

    if(!RoomManager::buildRoomOnTiles(&mGameMap, RoomType::treasury, &mPlayer, tiles))
        return false;

    return true;
}

bool KeeperAI::handleRooms()
{
    if(mCooldownLookingForRooms > 0)
    {
        --mCooldownLookingForRooms;
        return false;
    }

    mCooldownLookingForRooms = Random::Int(mCooldownLookingForRoomsMin, mCooldownLookingForRoomsMax);

    // We check if the last built room is done
    if(mRoomSize != -1)
    {
        Tile* tile = mGameMap.getTile(mRoomPosX, mRoomPosY);
        if(tile == nullptr)
        {
            OD_LOG_ERR("mRoomPosX=" + Helper::toString(mRoomPosX) + ", mRoomPosY=" + Helper::toString(mRoomPosY));
            mRoomSize = -1;
            return false;
        }
        int32_t points;
        if(!computePointsForRoom(tile, mPlayer.getSeat(), mRoomSize, true, false, points))
        {
            // The room is not valid anymore (may be claimed or built by somebody else). We redo
            mRoomSize = -1;
            return false;
        }

        if(buildMostNeededRoom())
        {
            mRoomSize = -1;
            return true;
        }

        return false;
    }

    Tile* central = getDungeonTemple()->getCentralTile();
    int32_t bestX = 0;
    int32_t bestY = 0;
    if(!findBestPlaceForRoom(central, mPlayer.getSeat(), 5, true, bestX, bestY))
        return false;

    mRoomSize = 5;
    mRoomPosX = bestX;
    mRoomPosY = bestY;

    Tile* tileDest = mGameMap.getTile(mRoomPosX, mRoomPosY);
    if(tileDest == nullptr)
    {
        OD_LOG_ERR("tileDest=" + Tile::displayAsString(tileDest) + ", mRoomPosX=" + Helper::toString(mRoomPosX) + ", mRoomPosY=" + Helper::toString(mRoomPosY));
        return false;
    }
    if(!digWayToTile(central, tileDest))
        return false;

    for(int xx = 0; xx < mRoomSize; ++xx)
    {
        for(int yy = 0; yy < mRoomSize; ++yy)
        {
            Tile* tile = mGameMap.getTile(mRoomPosX + xx, mRoomPosY + yy);
            if(tile == nullptr)
            {
                OD_LOG_ERR("xx=" + Helper::toString(mRoomPosX + xx) + ", yy=" + Helper::toString(mRoomPosY + yy));
                continue;
            }

            tile->setMarkedForDigging(true, &mPlayer);
        }
    }

    return true;
}

bool KeeperAI::lookForGold()
{
    if (mNoMoreReachableGold)
        return false;

    if(mCooldownLookingForGold > 0)
    {
        --mCooldownLookingForGold;
        return false;
    }

    mCooldownLookingForGold = Random::Int(70,120);

    // Do we need gold ?
    int emptyStorage = 0;
    for(Room* room : mGameMap.getRooms())
    {
        if(room->getSeat() != mPlayer.getSeat())
            continue;

        emptyStorage += (room->getTotalGoldStorage() - room->getTotalGoldStored());
    }

    // No need to search for gold
    if(emptyStorage < 100)
        return false;

    Tile* central = getDungeonTemple()->getCentralTile();
    int widerSide = mGameMap.getMapSizeX() > mGameMap.getMapSizeY() ?
        mGameMap.getMapSizeX() : mGameMap.getMapSizeY();

    // We search for the closest gold tile
    Tile* firstGoldTile = nullptr;
    for(int32_t distance = 1; distance < widerSide; ++distance)
    {
        for(int k = 0; k <= distance; ++k)
        {
            Tile* t;
            // North-East
            t = mGameMap.getTile(central->getX() + k, central->getY() + distance);
            if(t != nullptr && t->getType() == TileType::gold && t->getFullness() > 0.0)
            {
                // If we already have a tile at same distance, we randomly change to
                // try to not be too predictable
                if((firstGoldTile == nullptr) || (Random::Uint(1,2) == 1))
                    firstGoldTile = t;
            }
            // North-West
            if(k > 0)
            {
                t = mGameMap.getTile(central->getX() - k, central->getY() + distance);
                if(t != nullptr && t->getType() == TileType::gold && t->getFullness() > 0.0)
                {
                    if((firstGoldTile == nullptr) || (Random::Uint(1,2) == 1))
                        firstGoldTile = t;
                }
            }
            // South-East
            t = mGameMap.getTile(central->getX() + k, central->getY() - distance);
            if(t != nullptr && t->getType() == TileType::gold && t->getFullness() > 0.0)
            {
                if((firstGoldTile == nullptr) || (Random::Uint(1,2) == 1))
                    firstGoldTile = t;
            }
            // South-West
            if(k > 0)
            {
                t = mGameMap.getTile(central->getX() - k, central->getY() - distance);
                if(t != nullptr && t->getType() == TileType::gold && t->getFullness() > 0.0)
                {
                    if((firstGoldTile == nullptr) || (Random::Uint(1,2) == 1))
                        firstGoldTile = t;
                }
            }
            // East-North
            t = mGameMap.getTile(central->getX() + distance, central->getY() + k);
            if(t != nullptr && t->getType() == TileType::gold && t->getFullness() > 0.0)
            {
                if((firstGoldTile == nullptr) || (Random::Uint(1,2) == 1))
                    firstGoldTile = t;
            }
            // East-South
            if(k > 0)
            {
                t = mGameMap.getTile(central->getX() + distance, central->getY() - k);
                if(t != nullptr && t->getType() == TileType::gold && t->getFullness() > 0.0)
                {
                    if((firstGoldTile == nullptr) || (Random::Uint(1,2) == 1))
                        firstGoldTile = t;
                }
            }
            // West-North
            t = mGameMap.getTile(central->getX() - distance, central->getY() + k);
            if(t != nullptr && t->getType() == TileType::gold && t->getFullness() > 0.0)
            {
                if((firstGoldTile == nullptr) || (Random::Uint(1,2) == 1))
                    firstGoldTile = t;
            }
            // West-South
            if(k > 0)
            {
                t = mGameMap.getTile(central->getX() - distance, central->getY() - k);
                if(t != nullptr && t->getType() == TileType::gold && t->getFullness() > 0.0)
                {
                    if((firstGoldTile == nullptr) || (Random::Uint(1,2) == 1))
                        firstGoldTile = t;
                }
            }

            if(firstGoldTile != nullptr)
                break;
        }

        // If we found a tile, no need to continue
        if(firstGoldTile != nullptr)
            break;
    }

    // No more gold
    if (firstGoldTile == nullptr)
    {
        mNoMoreReachableGold = true;
        return false;
    }

    if(!digWayToTile(central, firstGoldTile))
    {
        mNoMoreReachableGold = true;
        return false;
    }

    // If the neighbors are gold, we dig them
    const int levelTilesDig = 2;
    std::set<Tile*> tilesDig;
    // Because we can't insert Tiles in tilesDig while iterating, we use a copy: tilesDigTmp
    std::set<Tile*> tilesDigTmp;
    tilesDig.insert(firstGoldTile);

    for(int i = 0; i < levelTilesDig; ++i)
    {
        tilesDigTmp = tilesDig;
        for(Tile* tile : tilesDigTmp)
        {
            for(Tile* neigh : tile->getAllNeighbors())
            {
                if(neigh->getType() == TileType::gold && neigh->getFullness() > 0.0)
                    tilesDig.insert(neigh);
            }
        }
    }

    for(Tile* tile : tilesDig)
        tile->setMarkedForDigging(true, &mPlayer);

    return true;
}

bool KeeperAI::buildMostNeededRoom()
{
    for(RoomType roomType: wantedBuildings)
    {
        if(!checkNeedRoom(roomType))
            continue;

        if(!SkillManager::isRoomAvailable(roomType, mPlayer.getSeat()))
            continue;

        if(buildRoom(roomType, mRoomPosX, mRoomPosY, mRoomSize))
            return true;

    }
    return false;
}

bool KeeperAI::buildRoom(RoomType roomType, int x, int y, int roomSize)
{
    std::vector<Tile*> tiles = mGameMap.getBuildableTilesForPlayerInArea(x, y, x + roomSize - 1,
        y + roomSize - 1, &mPlayer);
    if (tiles.size() < static_cast<uint32_t>(roomSize * roomSize))
        return false;

    if(!RoomManager::buildRoomOnTiles(&mGameMap, roomType, &mPlayer, tiles))
        return false;

    OD_LOG_INF("AI seatId=" + Helper::toString(mPlayer.getSeat()->getId())+ " builds room type=" + Helper::toString(static_cast<uint32_t>(roomType)) + ", x=" + Helper::toString(x) + ", y=" + Helper::toString(y) + ", roomSize=" + Helper::toString(roomSize));
    return true;
}

bool KeeperAI::checkNeedRoom(RoomType roomType)
{
    // We should try to build one room of each except special rooms (like bridges)
    switch(roomType)
    {
        case RoomType::dormitory:
        {
            // If we have no dormitory, we try to build 1. If we already have one, we try to build another
            // if nothing else can be built
            if(mPlayer.getSeat()->getNbRooms(roomType) <= 0)
                return true;

            // If too many dormitories, no need to build more
            if(mPlayer.getSeat()->getNbRooms(roomType) > 1)
                return false;

            for(RoomType roomType: wantedBuildings)
            {
                if(roomType == RoomType::dormitory)
                    continue;

                if(!checkNeedRoom(roomType))
                    continue;

                if(!SkillManager::isRoomAvailable(roomType, mPlayer.getSeat()))
                    continue;

                // Another room is needed
                return false;
            }

            return true;
        }
        case RoomType::treasury:
        {
            int totalStorage = mPlayer.getSeat()->getGoldMax();
            int totalGold = mPlayer.getSeat()->getGold();

            if(((totalStorage - totalGold) < 200) && (totalGold < 20000))
                return true;

            return false;
        }
        case RoomType::bridgeWooden:
        case RoomType::bridgeStone:
            return false;

        default:
        {
            if(mPlayer.getSeat()->getNbRooms(roomType) <= 0)
                return true;

            return false;
        }
    }
}

void KeeperAI::saveWoundedCreatures()
{
    if(mCooldownSaveWoundedCreatures > 0)
    {
        --mCooldownSaveWoundedCreatures;
        return;
    }
    mCooldownSaveWoundedCreatures = Random::Int(mCooldownSaveWoundedCreaturesMin, mCooldownSaveWoundedCreaturesMax);

    Tile* dungeonTempleTile = getDungeonTemple()->getCentralTile();
    if(dungeonTempleTile == nullptr)
    {
        OD_LOG_ERR("keeperAi=" + mPlayer.getNick());
        return;
    }

    Seat* seat = mPlayer.getSeat();
    std::vector<Creature*> creatures = mGameMap.getCreaturesBySeat(seat);
    for(Creature* creature : creatures)
    {
        // We take away fleeing creatures not too near our dungeon heart
        if(!creature->isActionInList(CreatureActionType::flee))
            continue;
        Tile* tile = creature->getPositionTile();
        if(tile == nullptr)
            continue;

        if((std::abs(dungeonTempleTile->getX() - tile->getX()) <= 5) &&
           (std::abs(dungeonTempleTile->getY() - tile->getY()) <= 5))
        {
            // We are too close from our dungeon heart to be picked up
            continue;
        }

        if(!creature->tryPickup(seat))
            continue;

        mPlayer.pickUpEntity(creature);

        mPlayer.dropHand(dungeonTempleTile);
    }
}

void KeeperAI::handleDefense()
{
    if(mCooldownDefense > 0)
    {
        --mCooldownDefense;
        return;
    }
    mCooldownDefense = Random::Int(mCooldownDefenseMin, mCooldownDefenseMax);

    Seat* seat = mPlayer.getSeat();
    // We drop creatures nearby owned or allied attacked creatures
    std::vector<Creature*> creatures = mGameMap.getCreaturesByAlliedSeat(seat);
    for(Creature* creature : creatures)
    {
        // We check if a creature is fighting near a claimed tile. If yes, we drop a creature nearby
        if(!creature->isActionInList(CreatureActionType::fight))
            continue;

        Tile* tile = creature->getPositionTile();
        if(tile == nullptr)
            continue;

        // We look for a healthy creature not already fighting
        Creature* creatureToDrop = nullptr;
        for(Creature* creature2 : mGameMap.getCreaturesBySeat(seat))
        {
            if(creature2->getDefinition()->isWorker())
                continue;
            if(creature2->getHP() < (creature2->getMaxHp() * 0.5))
                continue;
            if(creature2->isActionInList(CreatureActionType::fight))
                continue;
            if((creatureToDrop != nullptr) && (creatureToDrop->getLevel() >= creature2->getLevel()))
                continue;

            creatureToDrop = creature2;
        }
        if(creatureToDrop == nullptr)
            continue;

        if(!creatureToDrop->tryPickup(seat))
            continue;

        for(Tile* neigh : tile->getAllNeighbors())
        {
            if(creatureToDrop->tryDrop(seat, neigh))
            {
                mPlayer.pickUpEntity(creatureToDrop);
                mPlayer.dropHand(neigh);
                return;
            }
        }
    }
}

bool KeeperAI::handleWorkers()
{
    if(mCooldownWorkers > 0)
    {
        --mCooldownWorkers;
        return false;
    }

    mCooldownWorkers = Random::Int(3,10);

    // We want to use the first covered tile because the central might be destroyed and enemy claimed
    // and, if it is the case, we will not be able to spawn a worker.
    int mana = static_cast<int>(mPlayer.getSeat()->getMana());
    int summonCost = SpellSummonWorker::getNextWorkerPriceForPlayer(&mGameMap, &mPlayer);
    if(mana < summonCost)
        return false;


    // If we have less than 4 workers or we have the chance, we summon
    int nbWorkers = mPlayer.getSeat()->getNumCreaturesWorkers();
    if((nbWorkers < 4) ||
       (Random::Int(0, nbWorkers * 3) == 0))
    {
        Tile* tile = getDungeonTemple()->getCoveredTile(0);
        std::vector<Tile*> tiles;
        tiles.push_back(tile);
        if(!SpellSummonWorker::summonWorkersOnTiles(&mGameMap, &mPlayer, tiles))
            return false;

        return true;
    }

    return false;
}

bool KeeperAI::repairRooms()
{
    if(mCooldownRepairRooms > 0)
    {
        --mCooldownRepairRooms;
        return false;
    }

    mCooldownRepairRooms = Random::Int(20,60);

    Seat* seat = mPlayer.getSeat();
    for(Room* room : mGameMap.getRooms())
    {
        if(room->getSeat() != seat)
            continue;

        if(!room->canBeRepaired())
            continue;

        int goldRequired = room->getCostRepair();

        if(!mGameMap.withdrawFromTreasuries(goldRequired, mPlayer.getSeat()))
            return false;

        room->repairRoom();
        // We only repair one room at a time. Note that if we want to repair more than one room at a time, we should pay
        // attention to not modify the room list (if room absorbed in RoomManager::buildRoom) while iterating it
        break;
    }

    return false;
}

bool KeeperAI::handleTiredCreatures()
{
    // Handle tired creatures if we have a dormitory
    if(mPlayer.getSeat()->getNbRooms(RoomType::dormitory) <= 0)
        return false;

    std::vector<Creature*> creatures = mGameMap.getCreaturesBySeat(mPlayer.getSeat());
    for(Creature* creature : creatures)
    {
        // We do not take creatures fighting
        if(creature->isActionInList(CreatureActionType::fight))
            continue;

        if(!creature->isTired())
            continue;

        Tile* dropTile = nullptr;
        // If the creature has a bed, we drop it in its dormitory
        Tile* homeTile = creature->getHomeTile();
        if(homeTile != nullptr)
        {
            // We check if it is already in the dormitory. If yes, do nothing
            Tile* posTile = creature->getPositionTile();
            if(posTile == nullptr)
            {
                OD_LOG_ERR("null position tile for creature=" + creature->getName() + ", pos=" + Helper::toString(creature->getPosition()));
                continue;
            }
            if(homeTile->getCoveringRoom() == posTile->getCoveringRoom())
                continue;

            // It is not on its dormitory, we try to drop it there
            dropTile = homeTile;
        }
        else
        {
            // The creature do not have a home tile. We check it can reach the
            // dungeon temple. If not, we drop it there
            Tile* posTile = creature->getPositionTile();
            if(posTile == nullptr)
            {
                OD_LOG_ERR("null position tile for creature=" + creature->getName() + ", pos=" + Helper::toString(creature->getPosition()));
                continue;
            }

            Tile* dungeonTempleTile = getDungeonTemple()->getCentralTile();
            if(mGameMap.pathExists(creature, posTile, dungeonTempleTile))
                continue;

            // The creature cannot reach its dungeon temple. We drop it there
            dropTile = dungeonTempleTile;
        }

        if(dropTile == nullptr)
            continue;

        if(!creature->tryPickup(mPlayer.getSeat()))
            continue;

        mPlayer.pickUpEntity(creature);

        mPlayer.dropHand(dropTile);

        // We help only 1 creature per turn
        return true;
    }

    return false;
}

bool KeeperAI::handleHungryCreatures()
{
    // Handle hungry creatures if we have a hatchery
    if(mPlayer.getSeat()->getNbRooms(RoomType::hatchery) <= 0)
        return false;

    std::vector<Creature*> creatures = mGameMap.getCreaturesBySeat(mPlayer.getSeat());
    for(Creature* creature : creatures)
    {
        // We do not take creatures fighting
        if(creature->isActionInList(CreatureActionType::fight))
            continue;

        if(!creature->isHungry())
            continue;

        // We check it can reach the dungeon temple. If not, we drop it there
        Tile* posTile = creature->getPositionTile();
        if(posTile == nullptr)
        {
            OD_LOG_ERR("null position tile for creature=" + creature->getName() + ", pos=" + Helper::toString(creature->getPosition()));
            continue;
        }

        Tile* dungeonTempleTile = getDungeonTemple()->getCentralTile();
        if(mGameMap.pathExists(creature, posTile, dungeonTempleTile))
            continue;

        // The creature cannot reach its dungeon temple. We drop it there
        if(!creature->tryPickup(mPlayer.getSeat()))
            continue;

        mPlayer.pickUpEntity(creature);

        mPlayer.dropHand(dungeonTempleTile);

        // We help only 1 creature per turn
        return true;
    }

    return false;
}

void KeeperAI::handleFirstTurn()
{
    Seat* seat = mPlayer.getSeat();
    // We set the skills to research. We start with pending skills to not modify research
    // order if it was already set in the level
    std::vector<SkillType> skills = seat->getSkillPending();
    SkillManager::buildRandomPendingSkillsForSeat(skills, seat);
    seat->setSkillTree(skills);
}

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

#include "rooms/RoomTreasury.h"

#include "entities/BuildingObject.h"
#include "entities/GameEntityType.h"
#include "entities/Tile.h"
#include "entities/TreasuryObject.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "modes/InputCommand.h"
#include "modes/InputManager.h"
#include "network/ODClient.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "rooms/RoomManager.h"
#include "sound/SoundEffectsManager.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/Random.h"

#include <string>

const std::string RoomTreasuryName = "Treasury";
const std::string RoomTreasuryNameDisplay = "Treasury room";
const RoomType RoomTreasury::mRoomType = RoomType::treasury;

namespace
{
class RoomTreasuryFactory : public RoomFactory
{
    RoomType getRoomType() const override
    { return RoomTreasury::mRoomType; }

    const std::string& getName() const override
    { return RoomTreasuryName; }

    const std::string& getNameReadable() const override
    { return RoomTreasuryNameDisplay; }

    int getCostPerTile() const override
    { return ConfigManager::getSingleton().getRoomConfigInt32("TreasuryCostPerTile"); }

    void checkBuildRoom(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const override
    {
        Player* player = gameMap->getLocalPlayer();
        int nbTreasuries = player->getSeat()->getNbRooms(RoomTreasury::mRoomType);
        int32_t pricePerTarget = RoomManager::costPerTile(RoomTreasury::mRoomType);
        int32_t playerGold = static_cast<int32_t>(player->getSeat()->getGold());
        if(inputManager.mCommandState == InputCommandState::infoOnly)
        {
            // First treasury tile is free
            if(nbTreasuries <= 0)
                pricePerTarget = 0;

            if(playerGold < pricePerTarget)
            {
                std::string txt = formatBuildRoom(RoomTreasury::mRoomType, pricePerTarget);
                inputCommand.displayText(Ogre::ColourValue::Red, txt);
            }
            else
            {
                std::string txt = formatBuildRoom(RoomTreasury::mRoomType, pricePerTarget);
                inputCommand.displayText(Ogre::ColourValue::White, txt);
            }
            inputCommand.selectSquaredTiles(inputManager.mXPos, inputManager.mYPos, inputManager.mXPos,
                inputManager.mYPos);
            return;
        }

        std::vector<Tile*> buildableTiles = gameMap->getBuildableTilesForPlayerInArea(inputManager.mXPos,
            inputManager.mYPos, inputManager.mLStartDragX, inputManager.mLStartDragY, player);

        inputCommand.selectTiles(buildableTiles);

        if(buildableTiles.empty())
        {
            std::string txt = formatBuildRoom(RoomTreasury::mRoomType, 0);
            inputCommand.displayText(Ogre::ColourValue::White, txt);
            return;
        }

        int32_t priceTotal = static_cast<int32_t>(buildableTiles.size()) * pricePerTarget;
        // First treasury tile is free
        if(nbTreasuries <= 0)
            priceTotal -= pricePerTarget;

        if(playerGold < priceTotal)
        {
            std::string txt = formatBuildRoom(RoomTreasury::mRoomType, priceTotal);
            inputCommand.displayText(Ogre::ColourValue::Red, txt);
            return;
        }

        std::string txt = formatBuildRoom(RoomTreasury::mRoomType, priceTotal);
        inputCommand.displayText(Ogre::ColourValue::White, txt);

        if(inputManager.mCommandState != InputCommandState::validated)
            return;

        ClientNotification *clientNotification = RoomManager::createRoomClientNotification(RoomTreasury::mRoomType);
        uint32_t nbTiles = buildableTiles.size();
        clientNotification->mPacket << nbTiles;
        for(Tile* tile : buildableTiles)
            gameMap->tileToPacket(clientNotification->mPacket, tile);

        ODClient::getSingleton().queueClientNotification(clientNotification);
    }

    bool buildRoom(GameMap* gameMap, Player* player, ODPacket& packet) const override
    {
        std::vector<Tile*> tiles;
        if(!getRoomTilesDefault(tiles, gameMap, player, packet))
            return false;

        int32_t price = getRoomCostForPlayer(gameMap, player, tiles);
        if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
            return false;

        RoomTreasury* room = new RoomTreasury(gameMap);
        room->setupRoom(gameMap->nextUniqueNameRoom(room->getType()), player->getSeat(), tiles);
        room->addToGameMap();
        room->createMesh();

        if((player != nullptr) &&
           (player->getIsHuman()))
        {
            // We notify the clients with vision of the changed tiles. Note that we need
            // to calculate per seat since they could have vision on different parts of the building
            std::map<Seat*,std::vector<Tile*>> tilesPerSeat;
            const std::vector<Seat*>& seats = gameMap->getSeats();
            for(Seat* tmpSeat : seats)
            {
                if(tmpSeat->getPlayer() == nullptr)
                    continue;
                if(!tmpSeat->getPlayer()->getIsHuman())
                    continue;

                for(Tile* tile : tiles)
                {
                    if(!tmpSeat->hasVisionOnTile(tile))
                        continue;

                    tile->changeNotifiedForSeat(tmpSeat);
                    tilesPerSeat[tmpSeat].push_back(tile);
                }
            }

            for(const std::pair<Seat* const,std::vector<Tile*>>& p : tilesPerSeat)
            {
                uint32_t nbTiles = p.second.size();
                ServerNotification serverNotification(
                    ServerNotificationType::refreshTiles, p.first->getPlayer());
                serverNotification.mPacket << nbTiles;
                for(Tile* tile : p.second)
                {
                    gameMap->tileToPacket(serverNotification.mPacket, tile);
                    p.first->updateTileStateForSeat(tile, false);
                    tile->exportToPacketForUpdate(serverNotification.mPacket, p.first);
                }
                ODServer::getSingleton().sendAsyncMsg(serverNotification);
            }
        }

        room->checkForRoomAbsorbtion();
        room->updateActiveSpots();

        return true;
    }

    void checkBuildRoomEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const override
    {
        checkBuildRoomDefaultEditor(gameMap, RoomTreasury::mRoomType, inputManager, inputCommand);
    }

    bool buildRoomEditor(GameMap* gameMap, ODPacket& packet) const override
    {
        RoomTreasury* room = new RoomTreasury(gameMap);
        return buildRoomDefaultEditor(gameMap, room, packet);
    }

    Room* getRoomFromStream(GameMap* gameMap, std::istream& is) const override
    {
        RoomTreasury* room = new RoomTreasury(gameMap);
        if(!Room::importRoomFromStream(*room, is))
        {
            OD_LOG_ERR("Error while building a room from the stream");
        }
        return room;
    }

    bool buildRoomOnTiles(GameMap* gameMap, Player* player, const std::vector<Tile*>& tiles) const override
    {
        int32_t price = getRoomCostForPlayer(gameMap, player, tiles);
        if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
            return false;

        RoomTreasury* room = new RoomTreasury(gameMap);
        return buildRoomDefault(gameMap, room, player->getSeat(), tiles);
    }

    int32_t getRoomCostForPlayer(GameMap* gameMap, Player* player, const std::vector<Tile*>& tiles) const
    {
        int32_t pricePerTarget = RoomManager::costPerTile(RoomTreasury::mRoomType);
        int32_t price = static_cast<int32_t>(tiles.size()) * pricePerTarget;
        // First treasury tile is free
        std::vector<Room*> treasuriesOwned = gameMap->getRoomsByTypeAndSeat(RoomTreasury::mRoomType, player->getSeat());
        if(treasuriesOwned.empty())
            price -= pricePerTarget;

        return price;
    }
};

// Register the factory
static RoomRegister reg(new RoomTreasuryFactory);
}

static const int maxGoldinTile = 1000;

RoomTreasury::RoomTreasury(GameMap* gameMap) :
    Room(gameMap),
    mGoldChanged(false)
{
    setMeshName("Treasury");
}

void RoomTreasury::doUpkeep()
{
    Room::doUpkeep();

    if (mCoveredTiles.empty())
        return;

    if(mGoldChanged)
    {
        for (std::pair<Tile* const, TileData*>& p : mTileData)
        {
            RoomTreasuryTileData* roomTreasuryTileData = static_cast<RoomTreasuryTileData*>(p.second);
            updateMeshesForTile(p.first, roomTreasuryTileData);
        }
        mGoldChanged = false;
    }
}

bool RoomTreasury::removeCoveredTile(Tile* t)
{
    // if the mesh has gold, we erase the mesh
    RoomTreasuryTileData* roomTreasuryTileData = static_cast<RoomTreasuryTileData*>(mTileData[t]);

    if(!roomTreasuryTileData->mMeshOfTile.empty())
        removeBuildingObject(t);

    if(roomTreasuryTileData->mGoldInTile > 0)
    {
        int value = roomTreasuryTileData->mGoldInTile;
        if(value > 0)
        {
            OD_LOG_INF("Room " + getName()
                + ", tile=" + Tile::displayAsString(t) + " releases gold amount = "
                + Helper::toString(value));
            TreasuryObject* obj = new TreasuryObject(getGameMap(), value);
            obj->addToGameMap();
            Ogre::Vector3 spawnPosition(static_cast<Ogre::Real>(t->getX()),
                                        static_cast<Ogre::Real>(t->getY()), 0.0f);
            obj->createMesh();
            obj->setPosition(spawnPosition);
        }
    }

    roomTreasuryTileData->mMeshOfTile.clear();
    roomTreasuryTileData->mGoldInTile = 0;
    return Room::removeCoveredTile(t);
}

int RoomTreasury::getTotalGoldStorage() const
{
    return numCoveredTiles() * maxGoldinTile;
}

int RoomTreasury::getTotalGoldStored() const
{
    int totalGold = 0;

    for (const std::pair<Tile* const, TileData*>& p : mTileData)
    {
        RoomTreasuryTileData* roomTreasuryTileData = static_cast<RoomTreasuryTileData*>(p.second);
        totalGold += roomTreasuryTileData->mGoldInTile;
    }

    return totalGold;
}

int RoomTreasury::depositGold(int gold, Tile *tile)
{
    int goldDeposited, goldToDeposit = gold, emptySpace;

    // Start by trying to deposit the gold in the requested tile.
    RoomTreasuryTileData* roomTreasuryTileData = static_cast<RoomTreasuryTileData*>(mTileData[tile]);
    emptySpace = maxGoldinTile - roomTreasuryTileData->mGoldInTile;
    goldDeposited = std::min(emptySpace, goldToDeposit);
    roomTreasuryTileData->mGoldInTile += goldDeposited;
    goldToDeposit -= goldDeposited;

    // If there is still gold left to deposit after the first tile, loop over all of the tiles and see if we can put the gold in another tile.
    for (std::pair<Tile* const, TileData*>& p : mTileData)
    {
        if(goldToDeposit <= 0)
            break;

        if(p.second->mHP <= 0)
            continue;

        // Store as much gold as we can in this tile.
        RoomTreasuryTileData* roomTreasuryTileData = static_cast<RoomTreasuryTileData*>(p.second);
        emptySpace = maxGoldinTile - roomTreasuryTileData->mGoldInTile;
        goldDeposited = std::min(emptySpace, goldToDeposit);
        roomTreasuryTileData->mGoldInTile += goldDeposited;
        goldToDeposit -= goldDeposited;
    }

    // Return the amount we were actually able to deposit
    // (i.e. the amount we wanted to deposit minus the amount we were unable to deposit).
    int wasDeposited = gold - goldToDeposit;
    // If we couldn't deposit anything, we do not notify
    if(wasDeposited == 0)
        return wasDeposited;

    mGoldChanged = true;

    // Tells the client to play a deposit gold sound. For now, we only send it to the players
    // with vision on tile
    fireRoomSound(*tile, "Treasury/DepositGold");

    return wasDeposited;
}

int RoomTreasury::withdrawGold(int gold)
{
    mGoldChanged = true;

    int withdrawlAmount = 0;
    for (std::pair<Tile* const, TileData*>& p : mTileData)
    {
        RoomTreasuryTileData* roomTreasuryTileData = static_cast<RoomTreasuryTileData*>(p.second);
        // Check to see if the current room tile has enough gold in it to fill the amount we still need to pick up.
        int goldStillNeeded = gold - withdrawlAmount;
        if (roomTreasuryTileData->mGoldInTile >= goldStillNeeded)
        {
            // There is enough to satisfy the request so we do so and exit the loop.
            withdrawlAmount += goldStillNeeded;
            roomTreasuryTileData->mGoldInTile -= goldStillNeeded;
            break;
        }
        else
        {
            // There is not enough to satisfy the request so take everything there is and move on to the next tile.
            withdrawlAmount += roomTreasuryTileData->mGoldInTile;
            roomTreasuryTileData->mGoldInTile = 0;
        }
    }

    return withdrawlAmount;
}

void RoomTreasury::updateMeshesForTile(Tile* tile, RoomTreasuryTileData* roomTreasuryTileData)
{
    int gold = roomTreasuryTileData->mGoldInTile;
    OD_ASSERT_TRUE_MSG(gold <= maxGoldinTile, "room=" + getName() + ", gold=" + Helper::toString(gold));

    // If the tile was and is empty, nothing to do
    if(roomTreasuryTileData->mMeshOfTile.empty() && (gold == 0))
        return;

    // If the tile was not empty but is now, we remove it
    if(gold == 0)
    {
        roomTreasuryTileData->mMeshOfTile.clear();
        removeBuildingObject(tile);
        return;
    }

    // If the mesh has not changed we do not need to do anything.
    std::string newMeshName = TreasuryObject::getMeshNameForGold(gold);
    if (roomTreasuryTileData->mMeshOfTile.compare(newMeshName) == 0)
        return;

    // If the mesh has changed we need to destroy the existing treasury if there was one
    if (!roomTreasuryTileData->mMeshOfTile.empty())
        removeBuildingObject(tile);

    if (gold > 0)
    {
        const double offset = 0.2;
        double posX = static_cast<double>(tile->getX());
        double posY = static_cast<double>(tile->getY());
        double posZ = 0;
        posX += Random::Double(-offset, offset);
        posY += Random::Double(-offset, offset);
        double angle = Random::Double(0.0, 360);
        BuildingObject* ro = new BuildingObject(getGameMap(), *this, newMeshName, tile, posX, posY, posZ, angle, false);
        addBuildingObject(tile, ro);
    }

    roomTreasuryTileData->mMeshOfTile = newMeshName;
}

bool RoomTreasury::hasCarryEntitySpot(GameEntity* carriedEntity)
{
    // We might accept more gold than empty space (for example, if there are 100 gold left
    // and 2 different workers want to bring a treasury) but we don't care
    if(carriedEntity->getObjectType() != GameEntityType::treasuryObject)
        return false;

    if(getTotalGoldStored() >= getTotalGoldStorage())
        return false;

    return true;
}

Tile* RoomTreasury::askSpotForCarriedEntity(GameEntity* carriedEntity)
{
    if(!hasCarryEntitySpot(carriedEntity))
        return nullptr;

    return mCoveredTiles[0];
}

void RoomTreasury::notifyCarryingStateChanged(Creature* carrier, GameEntity* carriedEntity)
{
    // If a treasury is deposited on the treasury, no need to handle it here.
    // It will handle himself alone
}

RoomTreasuryTileData* RoomTreasury::createTileData(Tile* tile)
{
    return new RoomTreasuryTileData;
}

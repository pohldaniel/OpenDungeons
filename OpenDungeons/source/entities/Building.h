/*!
 * \file   Building.h
 * \date:  22 March 2011
 * \author StefanP.MUC
 * \brief  Provides common methods and members for buildable objects, like rooms and traps
 *
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

#ifndef BUILDING_H_
#define BUILDING_H_

#include "entities/GameEntity.h"

class BuildingObject;
class GameMap;
class Tile;
class Room;
class Seat;
class Trap;

class TileData
{
public:
    TileData() :
        mHP(0)
    {}

    TileData(const TileData* tileData) :
        mHP(tileData->mHP)
    {}

    virtual ~TileData()
    {}

    virtual TileData* cloneTileData() const
    { return new TileData(this); }

    double mHP;

    //! Seats with vision on the corresponding tile. Note that seats with vision are not copied when cloning a TileData
    std::vector<Seat*> mSeatsVision;
};

/*! \class Building
 *  \brief This class holds elements that are common to Building like Rooms or Traps
 *
 * Functions and properties that are common to every buildable object like rooms and traps
 * should be placed into this class and initialised with a good default value in the default
 * constructor.
 */
class Building : public GameEntity
{
public:
    //! \brief Default constructor with default values. Buildings are used only on server map
    Building(GameMap* gameMap) :
        GameEntity(gameMap)
    {}

    virtual ~Building();

    const static double DEFAULT_TILE_HP;

    virtual void doUpkeep() override;

    virtual bool displayTileMesh() const
    { return false; }

    virtual bool colorCustomMesh() const
    { return false; }

    //! \brief Updates the active spot lists. Active spots are places where objects can be added
    virtual void updateActiveSpots() = 0;

    //! \brief  Do nothing since Buildings do not have exp.
    void receiveExp(double /*experience*/)
    {}

    //! \brief Checks if the building objects allow the room to be deleted
    bool canBuildingBeRemoved();

    void removeAllBuildingObjects();
    Tile* getCentralTile();

    virtual bool isClaimable(Seat* seat) const
    { return false; }

    virtual void claimForSeat(Seat* seat, Tile* tile, double danceRate)
    {}

    virtual bool canSeatSellBuilding(Seat* seat) const;

    virtual bool isAttackable(Tile* tile, Seat* seat) const override;
    virtual bool removeCoveredTile(Tile* t);
    std::vector<Tile*> getCoveredTiles() override;
    Tile* getCoveredTile(int index) override;
    uint32_t numCoveredTiles() const override
    { return mCoveredTiles.size(); }

    virtual double getPhysicalDefense() const
    { return 0.0; }
    virtual double getMagicalDefense() const
    { return 0.0; }
    virtual double getElementDefense() const
    { return 0.0; }

    virtual void clearCoveredTiles();
    double getHP(Tile *tile) const override;
    double takeDamage(GameEntity* attacker, double absoluteDamage, double physicalDamage, double magicalDamage, double elementDamage,
        Tile *tileTakingDamage, bool ko) override;
    std::string getNameTile(Tile* tile);

    //! \brief Tells whether the ground tile below the building tile should be displayed.
    virtual bool shouldDisplayGroundTile() const
    { return false; }

    //! \brief Tells whether the building wants the given entity to be brought
    virtual bool hasCarryEntitySpot(GameEntity* carriedEntity)
    { return false; }

    //! \brief Tells where the building wants the given entity to be brought
    //! returns the Tile carriedEntity should be brought to or nullptr if
    //! the carriedEntity is not wanted anymore (if no free spot for example).
    //! If askSpotForCarriedEntity returns a valid tile, a spot may be booked and in
    //! any case, notifyCarryingStateChanged should be called to release it
    virtual Tile* askSpotForCarriedEntity(GameEntity* carriedEntity)
    { return nullptr; }

    //! \brief Tells whether the carrying state changed. One should check the carrier
    //! tile position to check if it is in the requested Tile. If yes, the carriedEntity
    //! is at the wanted place. If not, it means that the carrier stopped carrying (for
    //! example, if it was killed or picked up during process)
    virtual void notifyCarryingStateChanged(Creature* carrier, GameEntity* carriedEntity)
    {}

    //! Tells if the covering tile should be set to dirty when the building is added on the tile
    virtual bool shouldSetCoveringTileDirty(Seat* seat, Tile* tile)
    { return true; }

    inline const std::vector<Tile*>& getCoveredTilesDestroyed() const
    { return mCoveredTilesDestroyed; }

    virtual bool isTileVisibleForSeat(Tile* tile, Seat* seat) const
    { return true; }

    virtual void notifySeatVision(Tile* tile, Seat* seat);

    virtual double getCreatureSpeed(const Creature* creature, Tile* tile) const;

    virtual bool permitsVision(Tile* tile)
    { return true; }

    //! \brief Allows to customize creature behaviour when dropped on the given building
    virtual void creatureDropped(Creature& creature)
    {}

    virtual void exportToStream(std::ostream& os) const override;
    virtual bool importFromStream(std::istream& is) override;

protected:
    //! \brief Allows to export/import specific data for child classes. Note that every tile
    //! should be exported on 1 line (thus, no line ending should be added here). Moreover
    //! the building will only export the tile coords. Exporting other relevant data is
    //! up to the subclass
    virtual void exportTileDataToStream(std::ostream& os, Tile* tile, TileData* tileData) const
    {}

    //! \brief importTileDataFromStream should add the tile to covered or destroyed tiles vector and,
    //! if added to covered tile vectors, set covering room
    //! Returns true if everything OK and false if there is a problem
    virtual bool importTileDataFromStream(std::istream& is, Tile* tile, TileData* tileData)
    { return true; }

    //! \brief This will be called when tiles will be added to the building. By overriding it,
    //! child classes can expand TileData and add the data they need
    virtual TileData* createTileData(Tile* tile);

    void addBuildingObject(Tile* targetTile, BuildingObject* obj);
    void removeBuildingObject(Tile* tile);
    void removeBuildingObject(BuildingObject* obj);
    BuildingObject* getBuildingObjectFromTile(Tile* tile);
    //! \brief Buildings are handled by the tile, they don't fire add/remove events
    void fireAddEntity(Seat* seat, bool async) override
    {}
    void fireRemoveEntity(Seat* seat) override
    {}

    std::map<Tile*, BuildingObject*> mBuildingObjects;
    std::vector<Tile*> mCoveredTiles;
    std::vector<Tile*> mCoveredTilesDestroyed;
    std::map<Tile*, TileData*> mTileData;
};

#endif // BUILDING_H_

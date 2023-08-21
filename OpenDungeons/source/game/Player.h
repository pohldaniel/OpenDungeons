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

#ifndef PLAYER_H
#define PLAYER_H

#include <OgrePrerequisites.h>

#include <string>
#include <vector>
#include <cstdint>

class Creature;
class GameMap;
class GameEntity;
class ODPacket;
class Skill;
class Seat;
class Tile;

enum class CreatureActionType;
enum class SpellType;

enum class PlayerEventType
{
    nullType,
    fight
};

ODPacket& operator<<(ODPacket& os, const PlayerEventType& type);
ODPacket& operator>>(ODPacket& is, PlayerEventType& type);

//! Class that represents the events where the player can zoom
class PlayerEvent
{
public:
    PlayerEvent() :
        mType(PlayerEventType::nullType),
        mTile(nullptr),
        mTimeRemain(0.0)
    {
    }

    PlayerEvent(PlayerEventType type, Tile* tile, Ogre::Real timeRemain) :
        mType(type),
        mTile(tile),
        mTimeRemain(timeRemain)
    {
    }

    inline PlayerEventType getType() const
    { return mType; }

    inline Tile* getTile() const
    { return mTile; }

    inline float getTimeRemain() const
    { return mTimeRemain; }

    inline void setTimeRemain(float timeRemain)
    { mTimeRemain = timeRemain; }

    static PlayerEvent* getPlayerEventFromPacket(GameMap* gameMap, ODPacket& is);
    static void exportPlayerEventToPacket(PlayerEvent* event, GameMap* gameMap, ODPacket& is);

protected:
    void exportToPacket(GameMap* gameMap, ODPacket& os);
    void importFromPacket(GameMap* gameMap, ODPacket& is);

private:
    PlayerEventType mType;
    Tile* mTile;
    float mTimeRemain;
};

class PlayerSpellData
{
public:
    PlayerSpellData(uint32_t cooldownNbTurnPending, float cooldownTimePendingTurn) :
        mCooldownNbTurnPending(cooldownNbTurnPending),
        mCooldownTimePendingTurn(cooldownTimePendingTurn)
    {}

    uint32_t mCooldownNbTurnPending;
    float mCooldownTimePendingTurn;
};

/*! \brief The player class contains information about a human, or computer, player in the game.
 *
 * When a new player joins a game being hosted on a server the server will
 * allocate a new Player structure and fill it in with the appropriate values.
 * Its relevant information will then be sent to the other players in the game
 * so they are aware of its presence.
 */
class Player
{
    friend class Seat;
public:
    enum class Direction
    {
        left = -1,
        right = 1
    };

    Player(GameMap* gameMap, int32_t id);

    inline int32_t getId() const
    { return mId; }

    const std::string& getNick() const
    { return mNickname; }

    Seat* getSeat()
    { return mSeat; }

    const Seat* getSeat() const
    { return mSeat; }

    void setNick (const std::string& nick)
    { mNickname = nick; }

    //! \brief A simple accessor function to return the number of creatures
    //! this player is holding in his/her hand that belongs to seat seat.
    //! If seat is nullptr, then returns the total number of creatures
    unsigned int numCreaturesInHand(const Seat* seat = nullptr) const;
    unsigned int numObjectsInHand() const;

    /*! \brief Check to see if it is the user or another player picking up the creature and act accordingly.
     *
     * This function takes care of all of the operations required for a player to
     * pick up an object (creature, treasury, ...).  If the player is the user we need to move the creature
     * oncreen to the "hand" as well as add the creature to the list of creatures
     * in our own hand, this is done by setting moveToHand to true.  If move to
     * hand is false we just hide the creature (and stop its AI, etc.), rather than
     * making it follow the cursor.
     */
    void pickUpEntity(GameEntity *entity);

    //! \brief Check to see the first object in hand can be dropped on Tile t and do so if possible.
    bool isDropHandPossible(Tile *t, unsigned int index = 0);

    //! \brief Drops the creature on tile t. Returns the dropped creature
    void dropHand(Tile *t, unsigned int index = 0);

    void rotateHand(Direction d);

    //! \brief Clears all creatures that a player might have in his hand
    void clearObjectsInHand();

    //! \brief Clears all creatures that a player might have in his hand
    void notifyNoMoreDungeonTemple();

    inline bool getIsHuman() const
    { return mIsHuman; }

    inline void setIsHuman(bool isHuman)
    { mIsHuman = isHuman; }

    inline const std::vector<GameEntity*>& getObjectsInHand()
    { return mObjectsInHand; }

    inline bool getHasLost() const
    { return mHasLost; }

    //! \brief Notify the player is fighting
    //! Should be called on the server game map for human players only. tile represents
    //! the place where the fight is happening and player is the Player actually fighting
    void notifyTeamFighting(Player* player, Tile* tile);

    //! \brief Notify the player that they are not skill anything
    //! Should be called on the server game map for human players only
    void notifyNoSkillInQueue();

    //! \brief Notify the player that he has no worker
    void notifyNoWorker();

    //! \brief Notify the player that no treasury tiles are available for gold storage
    //! Should be called on the server game map for human players only
    void notifyNoTreasuryAvailable();

    //! \brief Notify the player that a creature cannot find a bed
    void notifyCreatureCannotFindBed(Creature& creature);

    //! \brief Notify the player that a creature cannot find a bed
    void notifyCreatureCannotFindFood(Creature& creature);

    void fireEvents();

    //! \brief Called on client side to update the current list of events. Note that
    //! the time remaining of PlayerEvent class is not relevant on client side
    void updateEvents(const std::vector<PlayerEvent*>& events);

    //! \brief Get the next event. After index. If index > events size, the first one is
    //! sent. When the function returns, index is set to the index of the returned event
    //! and it returns the PlayerEvent if any (nullptr is none).
    //! Note that on client side, PlayerEvent::mTimeRemain is not relevant
    const PlayerEvent* getNextEvent(uint32_t& index) const;

    //! \brief Marks the tiles for digging and send the refresh event to concerned player if human
    void markTilesForDigging(bool marked, const std::vector<Tile*>& tiles, bool asyncMsg);

    //! \brief Gets the spell cooldown in turns for the given spell
    uint32_t getSpellCooldownTurns(SpellType spellType) const;

    //! \brief Gets the spell cooldown as a smooth value 0 being no cooldown and 1
    //! max cooldown for the given spell
    float getSpellCooldownSmooth(SpellType spellType) const;
    float getSpellCooldownSmoothTime(SpellType spellType) const;

    void setSpellCooldownTurns(SpellType spellType, uint32_t cooldown);

    //! \brief Called each turn, it should handle Player upkeep on server side
    void upkeepPlayer(double timeSinceLastUpkeep);

    //! \brief Decreases cooldown for all spells. Used on both server and client sides
    void decreaseSpellCooldowns();

    //! \brief Called each time a frame is displayed. Called on client side
    void frameStarted(float timeSinceLastFrame);

    //! \brief Called when a worker picks up a new action
    void notifyWorkerAction(Creature& worker, CreatureActionType actionType);
    void notifyWorkerStopsAction(Creature& worker, CreatureActionType actionType);

    //! \brief Returns how many workers are doing the given action
    uint32_t getNbWorkersDoing(CreatureActionType actionType) const;

    //! \brief Returns a list of the actions the worker should do based on what the other workers
    //! of this seat are doing. The worker should try the actions on the given order
    std::vector<CreatureActionType> getWorkerPreferredActions(Creature& worker) const;

private:
    //! \brief Player ID is only used during seat configuration phase
    //! During the game, one should use the seat ID to identify a player because
    //! every AI player has an id = 0.
    //! ID is unique only for human players
    int32_t mId;

    GameMap* mGameMap;
    Seat *mSeat;

    //! \brief The nickname used in chat, etc.
    std::string mNickname;

    //! \brief The creature the player has got in hand.
    std::vector<GameEntity*> mObjectsInHand;

    //! True: player is human. False: player is a computer/inactive.
    bool mIsHuman;

    //! \brief This counter tells for how much time is left before considering
    //! the player should be notified again that he has not queued a skill.
    float mNoSkillInQueueTime;

    //! \brief This counter tells for how much time is left before considering
    //! the player should be notified again that he has no worker.
    float mNoWorkerTime;

    //! \brief This counter tells for how much time is left before considering
    //! the player should be notified again that he has no free space to store gold.
    float mNoTreasuryAvailableTime;

    //! \brief This counter tells how much time is left before considering
    //! the player should be notified again that a creature cannot find place in a dormitory.
    float mCreatureCannotFindBed;

    //! \brief This counter tells how much time is left before considering
    //! the player should be notified again that a creature cannot find place in a hatchery.
    float mCreatureCannotFindFood;

    bool mHasLost;

    //! \brief List of tiles there is an event on. Used on client and server
    std::vector<PlayerEvent*> mEvents;

    //! \brief Cooldown for every spells. Used on client and server.
    //! the first value is the cooldown in turn. The second is the smooth time (set to
    //! 1 turn duration at each refresh)
    //! Note that the second value is used on client side only
    std::vector<PlayerSpellData> mSpellsCooldown;

    //! \brief Used to know what the workers are doing. That will help to change
    //! probability to choose the action to do
    std::vector<uint32_t> mWorkersActions;

    //! \brief A simple mutator function to put the given entity into the player's hand,
    //! note this should NOT be called directly for creatures on the map,
    //! for that you should use the correct function like pickUpEntity() instead.
    void addEntityToHand(GameEntity *entity);
};

#endif // PLAYER_H

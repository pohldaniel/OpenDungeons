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

#include "spells/SpellCreatureExplosion.h"

#include "creatureeffect/CreatureEffectExplosion.h"
#include "entities/Creature.h"
#include "entities/GameEntityType.h"
#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "modes/InputCommand.h"
#include "modes/InputManager.h"
#include "network/ODClient.h"
#include "spells/SpellType.h"
#include "spells/SpellManager.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

const std::string SpellCreatureExplosionName = "creatureExplosion";
const std::string SpellCreatureExplosionNameDisplay = "Creature explosion";
const std::string SpellCreatureExplosionCooldownKey = "CreatureExplosionCooldown";
const SpellType SpellCreatureExplosion::mSpellType = SpellType::creatureExplosion;

namespace
{
class SpellCreatureExplosionFactory : public SpellFactory
{
    SpellType getSpellType() const override
    { return SpellCreatureExplosion::mSpellType; }

    const std::string& getName() const override
    { return SpellCreatureExplosionName; }

    const std::string& getCooldownKey() const override
    { return SpellCreatureExplosionCooldownKey; }

    const std::string& getNameReadable() const override
    { return SpellCreatureExplosionNameDisplay; }

    virtual void checkSpellCast(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const override
    { SpellCreatureExplosion::checkSpellCast(gameMap, inputManager, inputCommand); }

    virtual bool castSpell(GameMap* gameMap, Player* player, ODPacket& packet) const override
    { return SpellCreatureExplosion::castSpell(gameMap, player, packet); }

    Spell* getSpellFromStream(GameMap* gameMap, std::istream &is) const override
    { return SpellCreatureExplosion::getSpellFromStream(gameMap, is); }

    Spell* getSpellFromPacket(GameMap* gameMap, ODPacket &is) const override
    { return SpellCreatureExplosion::getSpellFromPacket(gameMap, is); }
};

// Register the factory
static SpellRegister reg(new SpellCreatureExplosionFactory);
}

void SpellCreatureExplosion::checkSpellCast(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand)
{
    Player* player = gameMap->getLocalPlayer();
    int32_t priceTotal = 0;
    int32_t pricePerTarget = ConfigManager::getSingleton().getSpellConfigInt32("CreatureExplosionPrice");
    int32_t playerMana = static_cast<int32_t>(player->getSeat()->getMana());
    if(inputManager.mCommandState == InputCommandState::infoOnly)
    {
        if(playerMana < pricePerTarget)
        {
            std::string txt = formatCastSpell(SpellType::creatureExplosion, pricePerTarget);
            inputCommand.displayText(Ogre::ColourValue::Red, txt);
        }
        else
        {
            std::string txt = formatCastSpell(SpellType::creatureExplosion, pricePerTarget);
            inputCommand.displayText(Ogre::ColourValue::White, txt);
        }
        inputCommand.selectSquaredTiles(inputManager.mXPos, inputManager.mYPos, inputManager.mXPos,
            inputManager.mYPos);
        return;
    }

    if(inputManager.mCommandState == InputCommandState::building)
    {
        inputCommand.selectSquaredTiles(inputManager.mXPos, inputManager.mYPos, inputManager.mLStartDragX,
            inputManager.mLStartDragY);
    }

    std::vector<GameEntity*> targets;
    gameMap->playerSelects(targets, inputManager.mXPos, inputManager.mYPos, inputManager.mLStartDragX, inputManager.mLStartDragY,
        SelectionTileAllowed::groundClaimedAllied, SelectionEntityWanted::creatureAliveEnemy, player);

    if(targets.empty())
    {
        std::string txt = formatCastSpell(SpellType::creatureExplosion, 0);
        inputCommand.displayText(Ogre::ColourValue::White, txt);
        return;
    }

    std::random_shuffle(targets.begin(), targets.end());
    std::vector<Creature*> creatures;
    for(GameEntity* target : targets)
    {
        if(playerMana < pricePerTarget)
            break;

        if(target->getObjectType() != GameEntityType::creature)
        {
            static bool logMsg = false;
            if(!logMsg)
            {
                logMsg = true;
                OD_LOG_ERR("Wrong target name=" + target->getName() + ", type=" + Helper::toString(static_cast<int32_t>(target->getObjectType())));
            }
            continue;
        }

        Creature* creature = static_cast<Creature*>(target);
        creatures.push_back(creature);

        priceTotal += pricePerTarget;
        playerMana -= pricePerTarget;
    }

    std::string txt = formatCastSpell(SpellType::creatureExplosion, priceTotal);
    inputCommand.displayText(Ogre::ColourValue::White, txt);

    if(inputManager.mCommandState != InputCommandState::validated)
        return;

    inputCommand.unselectAllTiles();

    ClientNotification *clientNotification = SpellManager::createSpellClientNotification(SpellType::creatureExplosion);
    uint32_t nbCreatures = creatures.size();
    clientNotification->mPacket << nbCreatures;
    for(Creature* creature : creatures)
        clientNotification->mPacket << creature->getName();

    ODClient::getSingleton().queueClientNotification(clientNotification);
}

bool SpellCreatureExplosion::castSpell(GameMap* gameMap, Player* player, ODPacket& packet)
{
    uint32_t nbCreatures;
    OD_ASSERT_TRUE(packet >> nbCreatures);
    std::vector<Creature*> creatures;
    while(nbCreatures > 0)
    {
        --nbCreatures;
        std::string creatureName;
        OD_ASSERT_TRUE(packet >> creatureName);

        // We check that the creatures are valid targets
        Creature* creature = gameMap->getCreature(creatureName);
        if(creature == nullptr)
        {
            OD_LOG_ERR("creatureName=" + creatureName);
            continue;
        }

        if(creature->getSeat()->isAlliedSeat(player->getSeat()))
        {
            OD_LOG_WRN("creatureName=" + creatureName);
            continue;
        }

        Tile* pos = creature->getPositionTile();
        if(pos == nullptr)
        {
            OD_LOG_WRN("creatureName=" + creatureName);
            continue;
        }

        if(!creature->isAlive())
        {
            // This can happen if the creature was alive on client side but is not since we received the message
            OD_LOG_WRN("creatureName=" + creatureName);
            continue;
        }

        // That can happen if the creature is not in perfect synchronization and is not on a claimed tile on the server gamemap
        if(!pos->isClaimedForSeat(player->getSeat()))
        {
            OD_LOG_INF("WARNING : " + creatureName + ", tile=" + Tile::displayAsString(pos));
            continue;
        }

        creatures.push_back(creature);
    }

    if(creatures.empty())
        return false;

    int32_t pricePerTarget = ConfigManager::getSingleton().getSpellConfigInt32("CreatureExplosionPrice");
    int32_t playerMana = static_cast<int32_t>(player->getSeat()->getMana());
    uint32_t nbTargets = std::min(static_cast<uint32_t>(playerMana / pricePerTarget), static_cast<uint32_t>(creatures.size()));
    int32_t priceTotal = nbTargets * pricePerTarget;

    if(creatures.size() > nbTargets)
        creatures.resize(nbTargets);

    if(!player->getSeat()->takeMana(priceTotal))
        return false;

    uint32_t duration = ConfigManager::getSingleton().getSpellConfigUInt32("CreatureExplosionDuration");
    double value = ConfigManager::getSingleton().getSpellConfigDouble("CreatureExplosionValue");
    for(Creature* creature : creatures)
    {
        CreatureEffectExplosion* effect = new CreatureEffectExplosion(duration, value, "SpellCreatureExplosion");
        creature->addCreatureEffect(effect);
    }

    return true;
}

Spell* SpellCreatureExplosion::getSpellFromStream(GameMap* gameMap, std::istream &is)
{
    OD_LOG_ERR("SpellCreatureExplosion cannot be read from stream");
    return nullptr;
}

Spell* SpellCreatureExplosion::getSpellFromPacket(GameMap* gameMap, ODPacket &is)
{
    OD_LOG_ERR("SpellCreatureExplosion cannot be read from packet");
    return nullptr;
}

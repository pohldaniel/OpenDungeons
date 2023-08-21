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

#ifndef CREATUREOVERLAYSTATUS_H
#define CREATUREOVERLAYSTATUS_H

#include <OgrePrerequisites.h>

#include <cstdint>

class Creature;
class MovableTextOverlay;
class Seat;

namespace Ogre
{
    class Entity;
    class Camera;
}

class CreatureOverlayStatus
{
public:
    CreatureOverlayStatus(Creature* creature, Ogre::Entity* ent,
        Ogre::Camera* cam);
    ~CreatureOverlayStatus();

    void displayHealthOverlay(Ogre::Real timeToDisplay);
    void update(Ogre::Real timeSincelastFrame);

private:
    void updateHealth();
    void updateStatus(Ogre::Real timeSincelastFrame);

    Creature* mCreature;
    Seat* mSeat;
    MovableTextOverlay* mMovableTextOverlay;
    uint32_t mHealthValue;
    unsigned int mLevel;
    Ogre::Real mTimeDisplayStatus;
    uint32_t mStatus;
    std::vector<uint32_t> mOverlayIds;
};

#endif // CREATUREOVERLAYSTATUS_H

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

#include "creatureeffect/CreatureEffect.h"

#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <istream>

bool CreatureEffect::upkeepEffect(Creature& creature)
{
    if(mNbTurnsEffect <= 0)
        return false;

    --mNbTurnsEffect;
    applyEffect(creature);
    return true;
}

void CreatureEffect::exportToStream(std::ostream& os) const
{
    os << "\t" << mNbTurnsEffect << "\t";
    if(mParticleEffectScript.empty())
        os << "none";
    else
        os << mParticleEffectScript;
}

bool CreatureEffect::importFromStream(std::istream& is)
{
    if(!(is >> mNbTurnsEffect))
        return false;
    if(!(is >> mParticleEffectScript))
        return false;

    if(mParticleEffectScript == "none")
        mParticleEffectScript.clear();

    return true;
}

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

#ifndef RENDERSCENEADDENTITY_H
#define RENDERSCENEADDENTITY_H

#include "renderscene/RenderScene.h"

class RenderSceneAddEntity : public RenderScene
{
public:
    RenderSceneAddEntity() :
        mEntity(nullptr)
    {}

    virtual ~RenderSceneAddEntity()
    {}

    const std::string& getModifierName() const override;

    bool activate(CameraManager& cameraManager, RenderManager& renderManager) override;

    void freeRessources(CameraManager& cameraManager, RenderManager& renderManager) override;

    bool importFromStream(std::istream& is) override;

private:
    std::string mName;
    std::string mMesh;
    Ogre::Vector3 mScale;
    Ogre::Vector3 mPosition;

    // Temporary ressources
    Ogre::Entity* mEntity;
};

#endif // RENDERSCENEADDENTITY_H

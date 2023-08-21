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

#ifndef EDITORMODE_H
#define EDITORMODE_H

#include "GameEditorModeBase.h"

#include "modes/InputCommand.h"
#include "modes/SettingsWindow.h"

class GameMap;
class Gui; // Used to change the Current tile type

enum class TileVisual;

class EditorMode final: public GameEditorModeBase, public InputCommand
{
public:
    EditorMode(ModeManager* modeManager);

    virtual ~EditorMode()
    {}

    virtual bool mouseMoved     (const OIS::MouseEvent &arg) override;
    virtual bool mousePressed   (const OIS::MouseEvent &arg, OIS::MouseButtonID id) override;
    virtual bool mouseReleased  (const OIS::MouseEvent &arg, OIS::MouseButtonID id) override;
    virtual bool keyPressed     (const OIS::KeyEvent &arg) override;
    virtual bool keyReleased    (const OIS::KeyEvent &arg) override;
    virtual void handleHotkeys  (OIS::KeyCode keycode) override;

    void onFrameStarted(const Ogre::FrameEvent& evt) override;
    void onFrameEnded(const Ogre::FrameEvent& evt) override;

    //! \brief Called when the game mode is activated
    //! Used to call the corresponding Gui Sheet.
    void activate() override;

    // ! Specific functions
    inline GameMap* getGameMap() const
    { return mGameMap; }

    virtual void notifyGuiAction(GuiAction guiAction) override;

    //! \brief Options window functions
    bool toggleOptionsWindow(const CEGUI::EventArgs& arg = {});
    bool showSettingsFromOptions(const CEGUI::EventArgs& e = {});
    bool showQuitMenuFromOptions(const CEGUI::EventArgs& arg = {});
    bool onSaveButtonClickFromOptions(const CEGUI::EventArgs& arg = {});

    //! \brief Quit menu functions
    bool showQuitMenu(const CEGUI::EventArgs& arg = {});
    bool hideQuitMenu(const CEGUI::EventArgs& arg = {});
    bool onClickYesQuitMenu(const CEGUI::EventArgs& arg = {});
    void setTileVisual(TileVisual tileVisual)
    { mCurrentTileVisual = tileVisual; }

    void selectSquaredTiles(int tileX1, int tileY1, int tileX2, int tileY2) override;
    void selectTiles(const std::vector<Tile*> tiles) override;
    void unselectAllTiles() override;

    void displayText(const Ogre::ColourValue& txtColour, const std::string& txt) override;

private:
    void connectTileSelect(const std::string& buttonName, TileVisual tileVisual);

    //! \brief Tile type (Dirt, Lava, Claimed, ...)
    TileVisual mCurrentTileVisual;

    //! \brief how of the wall type is there (0 - 100.0)
    //! < 1.0 means no walls.
    double mCurrentFullness;

    //! \brief Current selected creature to spawn
    uint32_t mCurrentCreatureIndex;

    //! \brief The creature node name being dragged by the mouse
    std::string mDraggedCreature;

    //! \brief The map light node name being dragged by the mouse
    std::string mDraggedMapLight;

    //! \brief Stores the lastest mouse cursor and light positions.
    int mMouseX;
    int mMouseY;

    //! \brief The settings window.
    SettingsWindow mSettings;

    //! \brief A sub-function called by mouseMoved()
    //! It will handle the potential mouse wheel logic
    void handleMouseWheel(const OIS::MouseEvent& arg);

    //! \brief Updates the text seen next to the cursor position.
    //! This text gives the tile position, and the current left-click action
    void updateCursorText();

    //! \brief Refreshes the gui buttons. It will be called at level loading only since
    //! it shouldn't change in the editor
    void refreshGuiSkill();

    //! \brief Updates the flag icon color when switching seats.
    void updateFlagColor();

    //! \brief Called when there is a mouse input change
    void checkInputCommand();
    void handlePlayerActionNone();
    void handlePlayerActionChangeTile();
};

#endif // EDITORMODE_H

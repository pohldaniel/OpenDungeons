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

#include "GameEditorModeBase.h"

#include "GameEditorModeConsole.h"
#include "camera/CullingManager.h"
#include "game/SkillManager.h"
#include "gamemap/GameMap.h"
#include "gamemap/MiniMap.h"
#include "network/ChatEventMessage.h"
#include "network/ODClient.h"
#include "render/Gui.h"
#include "render/ODFrameListener.h"
#include "rooms/RoomType.h"
#include "traps/TrapType.h"
#include "utils/Helper.h"
#include "utils/MakeUnique.h"

#include <OgreVector2.h>

#include <CEGUI/widgets/PushButton.h>
#include <CEGUI/widgets/Scrollbar.h>

static const Ogre::Real CHAT_TIME_DISPLAY = 30;

namespace {
    class ActionSelector
    {
    public:
        bool operator()(const CEGUI::EventArgs& e)
        {
            playerSelection.setCurrentAction(action);
            return true;
        }
        SelectedAction action;
        PlayerSelection& playerSelection;
    };
    //! \brief Functor for calling notifyGuiAction
    // only used in game and editor currently.
    class GuiNotifier
    {
    public:
        bool operator()(const CEGUI::EventArgs&)
        {
            mode.notifyGuiAction(action);
            return true;
        }
        AbstractApplicationMode::GuiAction action;
        GameEditorModeBase& mode;
    };
}

bool RoomSelector::operator()(const CEGUI::EventArgs& e)
{
    mPlayerSelection.setCurrentAction(SelectedAction::buildRoom);
    mPlayerSelection.setNewRoomType(mRoomType);
    return true;
}

bool TrapSelector::operator()(const CEGUI::EventArgs& e)
{
    mPlayerSelection.setCurrentAction(SelectedAction::buildTrap);
    mPlayerSelection.setNewTrapType(mTrapType);
    return true;
}

bool SpellSelector::operator()(const CEGUI::EventArgs& e)
{
    mPlayerSelection.setCurrentAction(SelectedAction::castSpell);
    mPlayerSelection.setNewSpellType(mSpellType);
    return true;
}

GameEditorModeBase::GameEditorModeBase(ModeManager* modeManager, ModeManager::ModeType modeType, CEGUI::Window* rootWindow) :
    AbstractApplicationMode(modeManager, modeType),
    mCurrentInputMode(InputModeNormal),
    mRootWindow(rootWindow),
    mGameMap(ODFrameListener::getSingletonPtr()->getClientGameMap()),
    mChatMessageDisplayTime(0),
    mChatMessageBoxDisplay(ChatMessageBoxDisplay::hide),
    mMiniMap(MiniMap::createMiniMap(rootWindow->getChild(Gui::MINIMAP))),
    mMainCullingManager(new CullingManager(mGameMap, CullingType::SHOW_MAIN_WINDOW)),
    mKeepReplayAtDisconnect(false),
    mConsole(Utils::make_unique<GameEditorModeConsole>(modeManager)),
    mCameraTilesIntersections(std::vector<Ogre::Vector3>(4, Ogre::Vector3::ZERO))
{
    addEventConnection(
        rootWindow->getChild(Gui::MINIMAP)->subscribeEvent(
            CEGUI::Window::EventMouseClick,
            CEGUI::Event::Subscriber(&GameEditorModeBase::onMinimapClick, this)
    ));

    //Rooms
    addEventConnection(
        rootWindow->getChild(Gui::BUTTON_DESTROY_ROOM)->subscribeEvent(
            CEGUI::Window::EventMouseClick,
            CEGUI::Event::Subscriber(ActionSelector{SelectedAction::destroyRoom, mPlayerSelection})
        )
    );

    // Traps
    addEventConnection(
        rootWindow->getChild(Gui::BUTTON_DESTROY_TRAP)->subscribeEvent(
            CEGUI::Window::EventMouseClick,
            CEGUI::Event::Subscriber(ActionSelector{SelectedAction::destroyTrap, mPlayerSelection})
        )
    );

    // Connect gui buttons (Rooms, traps, spells)
    SkillManager::connectGuiButtons(this, mRootWindow, mPlayerSelection);

    // Creature buttons
    connectGuiAction(Gui::BUTTON_CREATURE_WORKER, AbstractApplicationMode::GuiAction::ButtonPressedCreatureWorker);
    connectGuiAction(Gui::BUTTON_CREATURE_FIGHTER, AbstractApplicationMode::GuiAction::ButtonPressedCreatureFighter);

    // Clear up any events and chat messages
    CEGUI::Window* gameChatText = mRootWindow->getChild("GameChatWindow/GameChatText");
    gameChatText->setText("");
    gameChatText->hide();
    mRootWindow->getChild("GameEventText")->setText("");

    Ogre::Camera* cam = ODFrameListener::getSingleton().getCameraManager()->getActiveCamera();
    mMainCullingManager->computeIntersectionPoints(cam, mCameraTilesIntersections);
    mMainCullingManager->startTileCulling(cam, mCameraTilesIntersections);
}

GameEditorModeBase::~GameEditorModeBase()
{
    // Note that we don't stop culling because the gamemap has already been
    // cleared when we are here
    delete mMainCullingManager;
    delete mMiniMap;

    // Delete the potential pending event messages
    for (EventMessage* message : mEventMessages)
        delete message;

    if(ODClient::getSingleton().isConnected())
        ODClient::getSingleton().disconnect(mKeepReplayAtDisconnect);
    if(ODServer::getSingleton().isConnected())
        ODServer::getSingleton().stopServer();

    // Now that the server is stopped, we can clear the client game map
    mGameMap->clearAll();
}

void GameEditorModeBase::deactivate()
{
    // Delete the potential pending event messages
    for (EventMessage* message : mEventMessages)
        delete message;
    mEventMessages.clear();
}

void GameEditorModeBase::connectGuiAction(const std::string& buttonName, AbstractApplicationMode::GuiAction action)
{
    addEventConnection(
        mRootWindow->getChild(buttonName)->subscribeEvent(
          CEGUI::PushButton::EventClicked,
          CEGUI::Event::Subscriber(GuiNotifier{action, *this})
        )
    );
}

bool GameEditorModeBase::onMinimapClick(const CEGUI::EventArgs& arg)
{
    const CEGUI::MouseEventArgs& mouseEvt = static_cast<const CEGUI::MouseEventArgs&>(arg);

    ODFrameListener& frameListener = ODFrameListener::getSingleton();

    Ogre::Vector2 cc = mMiniMap->camera_2dPositionFromClick(static_cast<int>(mouseEvt.position.d_x),
        static_cast<int>(mouseEvt.position.d_y));
    frameListener.getCameraManager()->onMiniMapClick(cc);

    return true;
}

void GameEditorModeBase::onFrameStarted(const Ogre::FrameEvent& evt)
{
    updateMessages(evt.timeSinceLastFrame);

    Ogre::Camera* cam = ODFrameListener::getSingleton().getCameraManager()->getActiveCamera();
    mMainCullingManager->computeIntersectionPoints(cam, mCameraTilesIntersections);
    mMainCullingManager->update(cam, mCameraTilesIntersections);

    mMiniMap->update(evt.timeSinceLastFrame, mCameraTilesIntersections);
}

void GameEditorModeBase::receiveChat(const ChatMessage& chat)
{
    // Adds the message right away
    CEGUI::Window* chatTextBox = mRootWindow->getChild("GameChatWindow/GameChatText");
    chatTextBox->appendText(reinterpret_cast<const CEGUI::utf8*>(chat.getMessageAsString().c_str()));
    mChatMessageDisplayTime = CHAT_TIME_DISPLAY;
    mChatMessageBoxDisplay |= ChatMessageBoxDisplay::showMessageReceived;
    refreshChatDisplay();

    // Ensure the latest text is shown
    CEGUI::Scrollbar* scrollBar = reinterpret_cast<CEGUI::Scrollbar*>(chatTextBox->getChild("__auto_vscrollbar__"));
    scrollBar->setScrollPosition(scrollBar->getDocumentSize());
}

void GameEditorModeBase::receiveEventShortNotice(EventMessage* event)
{
    mEventMessages.emplace_back(event);

    // Adds the message right away
    CEGUI::Window* shortNoticeText = mRootWindow->getChild("GameEventText");
    shortNoticeText->appendText(reinterpret_cast<const CEGUI::utf8*>(event->getMessageAsString().c_str()));

    // Ensure the latest text is shown
    CEGUI::Scrollbar* scrollBar = reinterpret_cast<CEGUI::Scrollbar*>(shortNoticeText->getChild("__auto_vscrollbar__"));
    scrollBar->setScrollPosition(scrollBar->getDocumentSize());
}

void GameEditorModeBase::updateMessages(Ogre::Real update_time)
{
    float maxChatTimeDisplay = ODFrameListener::getSingleton().getEventMaxTimeDisplay();

    // Update the event message seen if necessary.
    CEGUI::Window* shortNoticeText = mRootWindow->getChild("GameEventText");
    CEGUI::Scrollbar* scrollBar = static_cast<CEGUI::Scrollbar*>(shortNoticeText->getChild("__auto_vscrollbar__"));
    float scrollPosition = scrollBar->getScrollPosition();

    // Update the chat message seen if necessary.
    bool messageDisplayUpdate = false;
    CEGUI::String ceguiStr;
    for (auto it = mEventMessages.begin(); it != mEventMessages.end();)
    {
        EventMessage* event = *it;
        if (event->isMessageTooOld(maxChatTimeDisplay))
        {
            delete event;
            it = mEventMessages.erase(it);
            messageDisplayUpdate = true;
        }
        else
        {
            ceguiStr += reinterpret_cast<const CEGUI::utf8*>(event->getMessageAsString().c_str());
            ++it;
        }
    }

    if (messageDisplayUpdate)
    {
        shortNoticeText->setText(ceguiStr);
        scrollBar->setScrollPosition(scrollPosition);
    }

    if(mChatMessageDisplayTime > 0)
    {
        // The chat box is displayed
        if(update_time < mChatMessageDisplayTime)
        {
            mChatMessageDisplayTime -= update_time;
        }
        else
        {
            mChatMessageDisplayTime = 0;
            mChatMessageBoxDisplay &= ~ChatMessageBoxDisplay::showMessageReceived;
            refreshChatDisplay();
        }
    }
}

void GameEditorModeBase::refreshChatDisplay()
{
    CEGUI::Window* chatTextBox = mRootWindow->getChild("GameChatWindow/GameChatText");
    if(mChatMessageBoxDisplay == ChatMessageBoxDisplay::hide)
        chatTextBox->hide();
    else
        chatTextBox->show();
}

void GameEditorModeBase::syncTabButtonTooltips(const CEGUI::String& tabControlName)
{
    // For each pane, we setup the corresponding tab button name.
    CEGUI::Window* tabControl = nullptr;
    try {
        tabControl = mRootWindow->getChild(tabControlName + "/__auto_TabPane__");
    }
    catch (std::exception& e)
    {
    }
    if (tabControl == nullptr)
        return;

    for(size_t i = 0; i < tabControl->getChildCount(); ++i)
    {
        CEGUI::Window* paneWin = tabControl->getChildAtIdx(i);

        // Try to get the tabButton corresponding widget.
        CEGUI::Window* tabButton = nullptr;
        CEGUI::String buttonName = tabControlName + "/__auto_TabPane__Buttons/__auto_btn" + paneWin->getName();
        try {
            tabButton = mRootWindow->getChild(buttonName);
        }
        catch (std::exception& e)
        {
        }
        if (tabButton == nullptr)
            continue;

        CEGUI::String propTooltip = paneWin->getProperty("TooltipText");
        tabButton->setTooltipText(propTooltip);
    }
}

void GameEditorModeBase::enterConsole()
{
    // We use a unique console instance.
    mCurrentInputMode = InputModeConsole;
    mConsole->activate();
}

void GameEditorModeBase::leaveConsole()
{
    // We're no more in console mode.
    mCurrentInputMode = InputModeNormal;
    activate();
}

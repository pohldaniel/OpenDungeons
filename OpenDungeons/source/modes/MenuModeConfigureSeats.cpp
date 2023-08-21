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

#include "gamemap/GameMap.h"

#include "ai/KeeperAIType.h"
#include "game/Seat.h"
#include "modes/MenuModeConfigureSeats.h"
#include "modes/ModeManager.h"
#include "network/ChatEventMessage.h"
#include "network/ODServer.h"
#include "network/ODClient.h"
#include "render/Gui.h"
#include "render/ODFrameListener.h"
#include "sound/MusicPlayer.h"
#include "utils/ConfigManager.h"
#include "utils/LogManager.h"
#include "utils/Helper.h"

#include <CEGUI/CEGUI.h>

#include <boost/filesystem.hpp>

const std::string TEXT_SEAT_ID_PREFIX = "TextSeat";
const std::string COMBOBOX_TEAM_ID_PREFIX = "ComboTeam";
const std::string COMBOBOX_PLAYER_FACTION_PREFIX = "ComboPlayerFactionSeat";
const std::string COMBOBOX_PLAYER_PREFIX = "ComboPlayerSeat";

MenuModeConfigureSeats::MenuModeConfigureSeats(ModeManager* modeManager):
    AbstractApplicationMode(modeManager, ModeManager::MENU_CONFIGURE_SEATS),
    mIsActivePlayerConfig(false)
{
    CEGUI::Window* window = modeManager->getGui().getGuiSheet(Gui::guiSheet::configureSeats);
    addEventConnection(
        window->getChild("ListPlayers/LaunchGameButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeConfigureSeats::launchSelectedButtonPressed, this)
        )
    );
    addEventConnection(
        window->getChild("ListPlayers/BackButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeConfigureSeats::goBack, this)
        )
    );

    addEventConnection(
        window->getChild("ListPlayers")->subscribeEvent(
            CEGUI::FrameWindow::EventCloseClicked,
            CEGUI::Event::Subscriber(&MenuModeConfigureSeats::goBack, this)
        )
    );

    addEventConnection(
        window->getChild("ListPlayers/GameChatEditBox")->subscribeEvent(
            CEGUI::Editbox::EventTextAccepted,
            CEGUI::Event::Subscriber(&MenuModeConfigureSeats::chatText, this)
        )
    );
}

MenuModeConfigureSeats::~MenuModeConfigureSeats()
{
    CEGUI::Window* tmpWin = getModeManager().getGui().getGuiSheet(Gui::guiSheet::configureSeats)->getChild("ListPlayers");
    for(int seatId : mSeatIds)
    {
        std::string name;
        name = TEXT_SEAT_ID_PREFIX + Helper::toString(seatId);
        tmpWin->destroyChild(name);
        name = COMBOBOX_PLAYER_FACTION_PREFIX + Helper::toString(seatId);
        tmpWin->destroyChild(name);
        name = COMBOBOX_PLAYER_PREFIX + Helper::toString(seatId);
        tmpWin->destroyChild(name);
        name = COMBOBOX_TEAM_ID_PREFIX + Helper::toString(seatId);
        tmpWin->destroyChild(name);
    }
}

void MenuModeConfigureSeats::activate()
{
    // Loads the corresponding Gui sheet.
    getModeManager().getGui().loadGuiSheet(Gui::guiSheet::configureSeats);

    giveFocus();

    // Play the main menu music
    MusicPlayer::getSingleton().play(ConfigManager::getSingleton().getMainMenuMusic());

    // We use the client game map to allow everybody to see how the server is configuring seats
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->setGamePaused(true);

    CEGUI::WindowManager& winMgr = CEGUI::WindowManager::getSingleton();
    CEGUI::Window* tmpWin = getModeManager().getGui().getGuiSheet(Gui::guiSheet::configureSeats)->getChild("ListPlayers");
    CEGUI::Window* msgWin = getModeManager().getGui().getGuiSheet(Gui::guiSheet::configureSeats)->getChild("LoadingText");
    msgWin->setText("Loading...");
    msgWin->setVisible(false);

    tmpWin->setText(reinterpret_cast<const CEGUI::utf8*>(std::string("Configure map : " + gameMap->getLevelName()).c_str()));

    // Reset the chat
    CEGUI::Window* chatWin = tmpWin->getChild("GameChatText");
    chatWin->setText("");
    CEGUI::Window* chatEdit = tmpWin->getChild("GameChatEditBox");
    chatEdit->setText("");

    const std::vector<std::string>& factions = ConfigManager::getSingleton().getFactions();
    const CEGUI::Image* selImg = &CEGUI::ImageManager::getSingleton().get("OpenDungeonsSkin/SelectionBrush");
    const std::vector<Seat*>& seats = gameMap->getSeats();

    int offset = 0;
    bool enabled = false;

    for(Seat* seat : seats)
    {
        // We do not add the rogue creatures seat
        if(seat->isRogueSeat())
            continue;

        mSeatIds.push_back(seat->getId());
        std::string name;
        CEGUI::Combobox* combo;

        name = TEXT_SEAT_ID_PREFIX + Helper::toString(seat->getId());
        CEGUI::DefaultWindow* textSeatId = static_cast<CEGUI::DefaultWindow*>(winMgr.createWindow("OD/StaticText", name));
        tmpWin->addChild(textSeatId);
        Ogre::ColourValue seatColor = seat->getColorValue();
        seatColor.a = 1.0f; // Restore the color opacity
        textSeatId->setArea(CEGUI::UDim(0.3,10), CEGUI::UDim(0,65 + offset), CEGUI::UDim(0,60), CEGUI::UDim(0,30));
        textSeatId->setText("[colour='" + Helper::getCEGUIColorFromOgreColourValue(seatColor) + "']Seat "  + Helper::toString(seat->getId()));
        textSeatId->setProperty("FrameEnabled", "False");
        textSeatId->setProperty("BackgroundEnabled", "False");

        name = COMBOBOX_PLAYER_FACTION_PREFIX + Helper::toString(seat->getId());
        combo = static_cast<CEGUI::Combobox*>(winMgr.createWindow("OD/Combobox", name));
        tmpWin->addChild(combo);
        combo->setArea(CEGUI::UDim(0.3,80), CEGUI::UDim(0,70 + offset), CEGUI::UDim(0.2,0), CEGUI::UDim(0,200));
        combo->setReadOnly(true);
        combo->setEnabled(enabled);
        combo->setSortingEnabled(true);
        if(seat->getFaction().compare(Seat::PLAYER_FACTION_CHOICE) == 0)
        {
            uint32_t cptFaction = 0;
            for(const std::string& faction : factions)
            {
                CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(faction, cptFaction);
                item->setSelectionBrushImage(selImg);
                combo->addItem(item);
                // We set nothing in the combos. They will be refreshed by the server
                ++cptFaction;
            }
        }
        else
        {
            uint32_t cptFaction = 0;
            for(const std::string& faction : factions)
            {
                if(seat->getFaction().compare(faction) == 0)
                    break;

                ++cptFaction;
            }
            // If the faction is not found, we set it to the first defined
            if(cptFaction >= factions.size())
                cptFaction = 0;

            CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(factions[cptFaction], cptFaction);
            item->setSelectionBrushImage(selImg);
            combo->addItem(item);
            combo->setText(item->getText());
            combo->setEnabled(false);
        }
        combo->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::SubscriberSlot(&MenuModeConfigureSeats::comboChanged, this));

        name = COMBOBOX_PLAYER_PREFIX + Helper::toString(seat->getId());
        combo = static_cast<CEGUI::Combobox*>(winMgr.createWindow("OD/Combobox", name));
        tmpWin->addChild(combo);
        combo->setArea(CEGUI::UDim(0.7,-90), CEGUI::UDim(0,70 + offset), CEGUI::UDim(0.3,0), CEGUI::UDim(0,200));
        combo->setReadOnly(true);
        combo->setEnabled(enabled);
        combo->setSortingEnabled(true);
        if(seat->getPlayerType().compare(Seat::PLAYER_TYPE_INACTIVE) == 0)
        {
            CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(Seat::PLAYER_TYPE_INACTIVE, Seat::PLAYER_TYPE_INACTIVE_ID);
            item->setSelectionBrushImage(selImg);
            combo->addItem(item);
            combo->setText(item->getText());
            combo->setEnabled(false);
        }
        else if(seat->getPlayerType().compare(Seat::PLAYER_TYPE_AI) == 0)
        {
            for(uint32_t i = 0; i < static_cast<uint32_t>(KeeperAIType::nbAI); ++i)
            {
                KeeperAIType type = static_cast<KeeperAIType>(i);
                int32_t id = Seat::aITypeToPlayerId(type);
                CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(KeeperAITypes::toDisplayableString(type), id);
                item->setSelectionBrushImage(selImg);
                combo->addItem(item);
            }
        }
        else if(seat->getPlayerType().compare(Seat::PLAYER_TYPE_CHOICE) == 0)
        {
            for(uint32_t i = 0; i < static_cast<uint32_t>(KeeperAIType::nbAI); ++i)
            {
                KeeperAIType type = static_cast<KeeperAIType>(i);
                int32_t id = Seat::aITypeToPlayerId(type);
                CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(KeeperAITypes::toDisplayableString(type), id);
                item->setSelectionBrushImage(selImg);
                combo->addItem(item);
            }
        }
        combo->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::SubscriberSlot(&MenuModeConfigureSeats::comboChanged, this));

        name = COMBOBOX_TEAM_ID_PREFIX + Helper::toString(seat->getId());
        combo = static_cast<CEGUI::Combobox*>(winMgr.createWindow("OD/Combobox", name));
        tmpWin->addChild(combo);
        combo->setArea(CEGUI::UDim(1,-80), CEGUI::UDim(0,70 + offset), CEGUI::UDim(0,60), CEGUI::UDim(0,200));
        combo->setReadOnly(true);
        combo->setEnabled(enabled);
        combo->setSortingEnabled(true);
        const std::vector<int>& availableTeamIds = seat->getAvailableTeamIds();
        OD_ASSERT_TRUE_MSG(!availableTeamIds.empty(), "Empty availableTeamIds for seat id="
            + Helper::toString(seat->getId()));
        if(availableTeamIds.size() > 1)
        {
            for(int teamId : availableTeamIds)
            {
                CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(Helper::toString(teamId), teamId);
                item->setSelectionBrushImage(selImg);
                combo->addItem(item);
                // We do not select anything by default. The server will refresh what needs to be
            }
        }
        else if(!availableTeamIds.empty())
        {
            int teamId = availableTeamIds[0];
            CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(Helper::toString(teamId), teamId);
            item->setSelectionBrushImage(selImg);
            combo->addItem(item);
            combo->setText(item->getText());
            combo->setEnabled(false);
        }
        combo->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::SubscriberSlot(&MenuModeConfigureSeats::comboChanged, this));

        offset += 30;
    }

    tmpWin = getModeManager().getGui().getGuiSheet(Gui::guiSheet::configureSeats)->getChild("ListPlayers/LaunchGameButton");
    tmpWin->setEnabled(enabled);

    // We notify the server we are ready to receive players and configure them
    ODClient::getSingleton().queueClientNotification(ClientNotificationType::readyForSeatConfiguration);
}

bool MenuModeConfigureSeats::launchSelectedButtonPressed(const CEGUI::EventArgs&)
{
    // We send to the server the associations faction/seat/player
    // It will be responsible to disconnect the unselected players
    if(!mIsActivePlayerConfig)
        return true;

    ClientNotification* notif = new ClientNotification(ClientNotificationType::seatConfigurationSet);
    ODClient::getSingleton().queueClientNotification(notif);
    return true;
}

bool MenuModeConfigureSeats::goBack(const CEGUI::EventArgs&)
{
    // We disconnect client and, if we are server, the server
    ODClient::getSingleton().disconnect();
    if(ODServer::getSingleton().isConnected())
    {
        ODServer::getSingleton().stopServer();
    }

    getModeManager().requestPreviousMode();
    return true;
}

bool MenuModeConfigureSeats::comboChanged(const CEGUI::EventArgs& ea)
{
    // If the combo changed is a player and he was already in another combo, we remove him from the combo
    CEGUI::Combobox* comboSel = static_cast<CEGUI::Combobox*>(static_cast<const CEGUI::WindowEventArgs&>(ea).window);
    CEGUI::ListboxItem* selItem = comboSel->getSelectedItem();
    if((selItem != nullptr) &&
       (comboSel->getName().compare(0, COMBOBOX_PLAYER_PREFIX.length(), COMBOBOX_PLAYER_PREFIX) == 0) &&
       (static_cast<int32_t>(selItem->getID()) >= Seat::PLAYER_ID_HUMAN_MIN) && // Can be several AI players
       (static_cast<int32_t>(selItem->getID()) != Seat::PLAYER_TYPE_INACTIVE_ID)) // Can be several inactive players
    {
        GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
        CEGUI::Window* playersWin = getModeManager().getGui().getGuiSheet(Gui::guiSheet::configureSeats)->getChild("ListPlayers");
        for(int seatId : mSeatIds)
        {
            Seat* seat = gameMap->getSeatById(seatId);
            if(seat->getPlayerType().compare(Seat::PLAYER_TYPE_INACTIVE) == 0)
                continue;

            std::string name = COMBOBOX_PLAYER_PREFIX + Helper::toString(seatId);
            CEGUI::Combobox* combo = static_cast<CEGUI::Combobox*>(playersWin->getChild(name));
            if(combo == comboSel)
                continue;

            CEGUI::ListboxItem* item = combo->getSelectedItem();
            if(item == nullptr)
                continue;

            if(item->getID() != selItem->getID())
                continue;

            combo->setText("");
            combo->setItemSelectState(item, false);
        }
    }

    fireSeatConfigurationToServer();
    return true;
}

void MenuModeConfigureSeats::addPlayer(const std::string& nick, int32_t id)
{
    const CEGUI::Image* selImg = &CEGUI::ImageManager::getSingleton().get("OpenDungeonsSkin/SelectionBrush");
    CEGUI::Window* playersWin = getModeManager().getGui().getGuiSheet(Gui::guiSheet::configureSeats)->getChild("ListPlayers");

    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    mPlayers.push_back(std::pair<std::string, int32_t>(nick, id));
    for(int seatId : mSeatIds)
    {
        Seat* seat = gameMap->getSeatById(seatId);
        if(seat->getPlayerType().compare(Seat::PLAYER_TYPE_INACTIVE) == 0)
            continue;

        if(seat->getPlayerType().compare(Seat::PLAYER_TYPE_AI) == 0)
            continue;

        std::string name = COMBOBOX_PLAYER_PREFIX + Helper::toString(seatId);
        CEGUI::Combobox* combo = static_cast<CEGUI::Combobox*>(playersWin->getChild(name));
        CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(reinterpret_cast<const CEGUI::utf8*>(nick.c_str()), id);
        item->setSelectionBrushImage(selImg);
        combo->addItem(item);
    }
}

void MenuModeConfigureSeats::removePlayer(int32_t id)
{
    CEGUI::Window* playersWin = getModeManager().getGui().getGuiSheet(Gui::guiSheet::configureSeats)->getChild("ListPlayers");
    for(std::vector<std::pair<std::string, int32_t> >::iterator it = mPlayers.begin(); it != mPlayers.end();)
    {
        std::pair<std::string, int32_t>& player = *it;
        if(player.second != id)
        {
            ++it;
            continue;
        }

        mPlayers.erase(it);
        for(int seatId : mSeatIds)
        {
            std::string name = COMBOBOX_PLAYER_PREFIX + Helper::toString(seatId);
            CEGUI::Combobox* combo = static_cast<CEGUI::Combobox*>(playersWin->getChild(name));
            for(uint32_t i = 0; i < combo->getItemCount();)
            {
                CEGUI::ListboxItem* selItem = combo->getListboxItemFromIndex(i);
                if(selItem->getID() == static_cast<CEGUI::uint>(id))
                {
                    if(selItem->isSelected())
                        combo->setText("");

                    combo->removeItem(selItem);
                }
                else
                    ++i;
            }
        }
        break;
    }
}

void MenuModeConfigureSeats::fireSeatConfigurationToServer()
{
    CEGUI::Window* playersWin = getModeManager().getGui().getGuiSheet(Gui::guiSheet::configureSeats)->getChild("ListPlayers");

    ClientNotification* notif = new ClientNotification(ClientNotificationType::seatConfigurationRefresh);

    for(int seatId : mSeatIds)
    {
        CEGUI::Combobox* combo;
        CEGUI::ListboxItem* selItem;
        std::string name;
        notif->mPacket << seatId;
        name = COMBOBOX_PLAYER_FACTION_PREFIX + Helper::toString(seatId);
        combo = static_cast<CEGUI::Combobox*>(playersWin->getChild(name));
        selItem = combo->getSelectedItem();
        if(selItem != nullptr)
        {
            int32_t factionIndex = static_cast<int32_t>(selItem->getID());
            notif->mPacket << true << factionIndex;
        }
        else
        {
            notif->mPacket << false;
        }

        name = COMBOBOX_PLAYER_PREFIX + Helper::toString(seatId);
        combo = static_cast<CEGUI::Combobox*>(playersWin->getChild(name));
        selItem = combo->getSelectedItem();
        if(selItem != nullptr)
        {
            int32_t playerId = selItem->getID();
            notif->mPacket << true << playerId;
        }
        else
        {
            notif->mPacket << false;
        }

        name = COMBOBOX_TEAM_ID_PREFIX + Helper::toString(seatId);
        combo = static_cast<CEGUI::Combobox*>(playersWin->getChild(name));
        selItem = combo->getSelectedItem();
        if(selItem != nullptr)
        {
            int32_t teamId = selItem->getID();
            notif->mPacket << true << teamId;
        }
        else
        {
            notif->mPacket << false;
        }
    }
    ODClient::getSingleton().queueClientNotification(notif);
}

void MenuModeConfigureSeats::activatePlayerConfig()
{
    mIsActivePlayerConfig = true;
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    CEGUI::Window* listPlayersWindow = getModeManager().getGui().getGuiSheet(Gui::guiSheet::configureSeats)->getChild("ListPlayers");
    listPlayersWindow->setText("Please configure map : " + gameMap->getLevelName());
    bool enabled = true;

    for(int seatId : mSeatIds)
    {
        Seat* seat = gameMap->getSeatById(seatId);

        std::string name;
        CEGUI::Combobox* combo;

        name = COMBOBOX_PLAYER_FACTION_PREFIX + Helper::toString(seatId);
        combo = static_cast<CEGUI::Combobox*>(listPlayersWindow->getChild(name));
        if(combo->getItemCount() > 1)
            combo->setEnabled(enabled);

        name = COMBOBOX_PLAYER_PREFIX + Helper::toString(seatId);
        combo = static_cast<CEGUI::Combobox*>(listPlayersWindow->getChild(name));
        if(enabled)
        {
            if(seat->getPlayerType().compare(Seat::PLAYER_TYPE_INACTIVE) != 0)
                combo->setEnabled(enabled);
        }
        else
            combo->setEnabled(enabled);

        name = COMBOBOX_TEAM_ID_PREFIX + Helper::toString(seatId);
        combo = static_cast<CEGUI::Combobox*>(listPlayersWindow->getChild(name));
        if(combo->getItemCount() > 1)
            combo->setEnabled(enabled);
    }

    CEGUI::Window* startButton = getModeManager().getGui().getGuiSheet(Gui::guiSheet::configureSeats)->getChild("ListPlayers/LaunchGameButton");
    startButton->setEnabled(enabled);
}

void MenuModeConfigureSeats::refreshSeatConfiguration(ODPacket& packet)
{
    CEGUI::Window* playersWin = getModeManager().getGui().getGuiSheet(Gui::guiSheet::configureSeats)->getChild("ListPlayers");
    for(int seatId : mSeatIds)
    {
        CEGUI::Combobox* combo;
        bool isSelected;
        int seatIdPacket;
        CEGUI::ListboxItem* selItem = nullptr;
        OD_ASSERT_TRUE(packet >> seatIdPacket);
        OD_ASSERT_TRUE_MSG(seatId == seatIdPacket, "seatId=" + Helper::toString(seatId) + ", seatIdPacket=" + Helper::toString(seatIdPacket));
        std::string name;
        name = COMBOBOX_PLAYER_FACTION_PREFIX + Helper::toString(seatId);
        combo = static_cast<CEGUI::Combobox*>(playersWin->getChild(name));
        OD_ASSERT_TRUE(packet >> isSelected);
        if(isSelected)
        {
            int32_t factionIndex = -1;
            OD_ASSERT_TRUE(packet >> factionIndex);
            uint32_t id = static_cast<uint32_t>(factionIndex);
            selItem = nullptr;
            for(uint32_t i = 0; i < combo->getItemCount(); ++i)
            {
                CEGUI::ListboxItem* item = combo->getListboxItemFromIndex(i);
                if(isSelected && item->getID() == id)
                    selItem = item;

                combo->setItemSelectState(item, false);
            }
            if(selItem != nullptr)
            {
                combo->setText(selItem->getText());
                combo->setItemSelectState(selItem, true);
            }
        }

        name = COMBOBOX_PLAYER_PREFIX + Helper::toString(seatId);
        combo = static_cast<CEGUI::Combobox*>(playersWin->getChild(name));
        OD_ASSERT_TRUE(packet >> isSelected);
        int32_t playerId = 0;
        if(isSelected)
        {
            OD_ASSERT_TRUE(packet >> playerId);
        }

        // Because of a bug in CEGUI::Combobox, the text is unset if we call setItemSelectState
        // on an unselected item that has the same text as the currently selected one. For
        // this reason, we start by unselecting everything and we will select after if
        // there is something to select
        selItem = nullptr;
        for(uint32_t i = 0; i < combo->getItemCount(); ++i)
        {
            CEGUI::ListboxItem* item = combo->getListboxItemFromIndex(i);
            if(isSelected && item->getID() == static_cast<uint32_t>(playerId))
                selItem = item;

            combo->setItemSelectState(item, false);
        }
        if(selItem != nullptr)
        {
            combo->setText(selItem->getText());
            combo->setItemSelectState(selItem, true);
        }

        name = COMBOBOX_TEAM_ID_PREFIX + Helper::toString(seatId);
        combo = static_cast<CEGUI::Combobox*>(playersWin->getChild(name));
        OD_ASSERT_TRUE(packet >> isSelected);
        int32_t teamId = 0;
        if(isSelected)
        {
            OD_ASSERT_TRUE(packet >> teamId);
        }
        selItem = nullptr;
        for(uint32_t i = 0; i < combo->getItemCount(); ++i)
        {
            CEGUI::ListboxItem* item = combo->getListboxItemFromIndex(i);
            if(isSelected && item->getID() == static_cast<uint32_t>(teamId))
                selItem = item;

            combo->setItemSelectState(item, false);
        }
        if(selItem != nullptr)
        {
            combo->setText(selItem->getText());
            combo->setItemSelectState(selItem, true);
        }
    }
}

void MenuModeConfigureSeats::receiveChat(const ChatMessage& chat)
{
    CEGUI::Window* playersWin = getModeManager().getGui().getGuiSheet(Gui::guiSheet::configureSeats)->getChild("ListPlayers");
    // Adds the message right away
    CEGUI::Window* chatWin = playersWin->getChild("GameChatText");
    chatWin->appendText(reinterpret_cast<const CEGUI::utf8*>(chat.getMessageAsString().c_str()));

    // Ensure the latest text is shown
    CEGUI::Scrollbar* scrollBar = reinterpret_cast<CEGUI::Scrollbar*>(chatWin->getChild("__auto_vscrollbar__"));
    scrollBar->setScrollPosition(scrollBar->getDocumentSize());
}

bool MenuModeConfigureSeats::chatText(const CEGUI::EventArgs& e)
{
    CEGUI::Window* playersWin = getModeManager().getGui().getGuiSheet(Gui::guiSheet::configureSeats)->getChild("ListPlayers");
    CEGUI::Editbox* chatEdit = static_cast<CEGUI::Editbox*>(playersWin->getChild("GameChatEditBox"));
    const std::string txt = chatEdit->getText().c_str();

    ODClient::getSingleton().queueClientNotification(ClientNotificationType::chat, txt);

    chatEdit->setText("");

    return true;
}

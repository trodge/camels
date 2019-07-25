/*
 * This file is part of Camels.
 *
 * Camels is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Camels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Camels.  If not, see <https://www.gnu.org/licenses/>.
 *
 * © Tom Rodgers notaraptor@gmail.com 2017-2019
 */

#include "player.hpp"

Player::Player(Game &g) : game(g), printer(g.getPrinter()) {
    SDL_Rect sR = Settings::getScreenRect();
    SDL_Color uiFgr = Settings::getUIForeground(), uiBgr = Settings::getUIBackground(), uiHgl = Settings::getUIHighlight();
    int bBB = Settings::getBigBoxBorder(), bBR = Settings::getBigBoxRadius(), bBFS = Settings::getBigBoxFontSize(),
        sBB = Settings::getSmallBoxBorder(), sBR = Settings::getSmallBoxRadius(), sBFS = Settings::getSmallBoxFontSize();
    auto bigBox = [&uiFgr, &uiBgr, &uiHgl, bBB, bBR, bBFS](const SDL_Rect &rt, const std::vector<std::string> &tx) {
        return BoxInfo{rt, tx, uiFgr, uiBgr, uiHgl, 0u, false, bBB, bBR, bBFS, {}};
    };
    auto bigButton = [&uiFgr, &uiBgr, &uiHgl, bBB, bBR, bBFS](const SDL_Rect &rt, const std::vector<std::string> &tx,
                                                              SDL_Keycode ky, std::function<void(MenuButton *)> fn) {
        return BoxInfo{rt, tx, uiFgr, uiBgr, uiHgl, 0u, false, bBB, bBR, bBFS, {}, ky, fn};
    };
    auto bigSelectButton = [&uiFgr, &uiBgr, &uiHgl, bBB, bBR, bBFS](const SDL_Rect &rt, const std::vector<std::string> &tx,
                                                                    SDL_Keycode ky, std::function<void(MenuButton *)> fn) {
        return BoxInfo{rt, tx, uiFgr, uiBgr, uiHgl, 0u, false, bBB, bBR, bBFS, {}, ky, fn};
    };
    auto smallBox = [&uiFgr, &uiBgr, &uiHgl, sBB, sBR, sBFS](const SDL_Rect &rt, const std::vector<std::string> &tx) {
        return BoxInfo{rt, tx, uiFgr, uiBgr, uiHgl, 0u, false, sBB, sBR, sBFS, {}};
    };
    auto smallButton = [&uiFgr, &uiBgr, &uiHgl, sBB, sBR, sBFS](const SDL_Rect &rt, const std::vector<std::string> &tx,
                                                                SDL_Keycode ky, std::function<void(MenuButton *)> fn) {
        return BoxInfo{rt, tx, uiFgr, uiBgr, uiHgl, 0u, false, sBB, sBR, sBFS, {}, ky, fn};
    };
    uiStates = {
        {UiState::starting,
         {{bigBox({sR.w / 2, sR.h / 15, 0, 0}, {"Camels and Silk"}),
           bigButton({sR.w / 7, sR.h / 3, 0, 0}, {"(N)ew Game"}, SDLK_n,
                     [this](MenuButton *) {
                         game.newGame();
                         setState(UiState::beginning);
                     }),
           bigButton({sR.w / 7, sR.h * 2 / 3, 0, 0}, {"(L}oad Game"}, SDLK_l,
                     [this](MenuButton *) { setState(UiState::loading); })}}},
        {UiState::beginning,
         {{bigBox({sR.w / 2, sR.h / 7, 0, 0}, {"Name", ""})},
          [this] {
              // Allow typing in name box.
              pagers[0u].toggleLock(0u);
          }}},
        {UiState::quitting,
         {{bigButton({sR.w / 2, sR.h / 4, 0, 0}, {"Continue"}, SDLK_ESCAPE,
                     [this](MenuButton *) {
                         pause = storedPause;
                         setState(storedState);
                     }),
           traveler ? bigButton({sR.w / 2, sR.h * 3 / 4, 0, 0}, {"Save and Quit"}, SDLK_q,
                                [this](MenuButton *) {
                                    game.saveGame();
                                    stop = true;
                                })
                    : bigButton({sR.w / 2, sR.h * 3 / 4, 0, 0}, {"Quit"}, SDLK_q, [this](MenuButton *) { stop = true; })},
          [this] {
              storedState = state;
              storedPause = pause;
          }}},
        {UiState::loading,
         {{bigButton({sR.w / 7, sR.h / 7, 0, 0}, {"(B)ack"}, SDLK_b,
                     [this](MenuButton *) { setState(UiState::starting); })}}},
        {UiState::traveling,
         {{smallButton({sR.w / 9, sR.h * 14 / 15, 0, 0}, {"(G)o"}, SDLK_g,
                       [this](MenuButton *btn) {
                           if (focusTown > -1)
                               game.pickTown(traveler, static_cast<size_t>(focusTown));
                           show = true;
                           btn->setClicked(false);
                       }),
           smallButton({sR.w * 2 / 9, sR.h * 14 / 15, 0, 0}, {"(T)rade"}, SDLK_t,
                       [this](MenuButton *) { setState(UiState::trading); }),
           smallButton({sR.w / 3, sR.h * 14 / 15, 0, 0}, {"(S)tore"}, SDLK_s,
                       [this](MenuButton *) { setState(UiState::trading); }),
           smallButton({sR.w * 4 / 9, sR.h * 14 / 15, 0, 0}, {"(M)anage"}, SDLK_m,
                       [this](MenuButton *) { setState(UiState::managing); }),
           smallButton({sR.w * 5 / 9, sR.h * 14 / 15, 0, 0}, {"(E)quip"}, SDLK_e,
                       [this](MenuButton *) { setState(UiState::equipping); }),
           smallButton({sR.w * 2 / 3, sR.h * 14 / 15, 0, 0}, {"(H)ire"}, SDLK_h,
                       [this](MenuButton *) { setState(UiState::hiring); }),
           smallButton({sR.w * 7 / 9, sR.h * 14 / 15, 0, 0}, {"(A)ttack"}, SDLK_a,
                       [this](MenuButton *) { setState(UiState::attacking); }),
           smallButton({sR.w * 8 / 9, sR.h * 14 / 15, 0, 0}, {"(L)og"}, SDLK_l,
                       [this](MenuButton *) { setState(UiState::logging); })},
          [this] { pause = false; }}},
        {UiState::trading,
         {{smallButton({sR.w * 2 / 9, sR.h * 14 / 15, 0, 0}, {"Stop (T)rading"}, SDLK_t,
                       [this](MenuButton *) { setState(UiState::traveling); }),
           smallButton({sR.w * 2 / 9, sR.h * 13 / 15, 0, 0}, {"(C)omplete Trade"}, SDLK_c,
                       [this](MenuButton *) {
                           traveler->makeTrade();
                           setState(UiState::trading);
                       }),
           smallBox({sR.w * 4 / 9, sR.h * 13 / 15, 0, 0}, {}),
           smallButton({sR.w * 4 / 9, sR.h * 14 / 15, 0, 0}, {"(S)et Portion"}, SDLK_s,
                       [this](MenuButton *btn) {
                           double p;
                           std::stringstream{pagers[0].getVisible(kPortionBoxIndex)->getText(0u)} >> p;
                           traveler->setPortion(p);
                           updatePortionBox();
                           btn->setClicked(false);
                       })},
          [this] { traveler->createTradeButtons(pagers, printer); },
          3u}},
        {UiState::storing,
         {{smallButton({sR.w / 3, sR.h * 14 / 15, 0, 0}, {"Stop (S)toring"}, SDLK_s,
                       [this](MenuButton *) { setState(UiState::traveling); })},
          [this] { traveler->createStorageButtons(pagers, focusBox, printer); },
          3u}},
        {UiState::managing,
         {{smallButton({sR.w * 4 / 9, sR.h * 14 / 15, 0, 0}, {"Stop (M)anaging"}, SDLK_m,
                       [this](MenuButton *) { setState(UiState::traveling); })},
          [this] { traveler->createManageButtons(pagers, printer); },
          3u}},
        {UiState::equipping,
         {{smallButton({sR.w * 5 / 9, sR.h * 14 / 15, 0, 0}, {"Stop (E)quipping"}, SDLK_e,
                       [this](MenuButton *) { setState(UiState::traveling); })},
          [this] { traveler->createEquipButtons(pagers, focusBox, printer); }}},
        {UiState::hiring,
         {{smallButton({sR.w * 2 / 3, sR.h * 14 / 15, 0, 0}, {"Stop (H)iring"}, SDLK_h,
                       [this](MenuButton *) { setState(UiState::traveling); })}}},
        {UiState::attacking,
         {{smallButton({sR.w * 7 / 9, sR.h * 14 / 15, 0, 0}, {"Cancel (A)ttack"}, SDLK_a,
                       [this](MenuButton *) { setState(UiState::traveling); })},
          [this] {
              traveler->createAttackButton(
                  pagers[0u], [this] { setState(UiState::fighting); }, printer);
          }}},
        {UiState::logging,
         {{smallButton({sR.w * 8 / 9, sR.h * 14 / 15, 0, 0}, {"Close (L)og"}, SDLK_l,
                       [this](MenuButton *) { setState(UiState::traveling); })},
          [this, sR, bBB, bBR, sBFS] {
              // Create log box.
              pagers[0].addBox(
                  std::make_unique<ScrollBox>(BoxInfo{{sR.w / 15, sR.h * 2 / 15, sR.w * 13 / 15, sR.h * 11 / 15},
                                                      traveler->getLogText(),
                                                      traveler->getNation()->getForeground(),
                                                      traveler->getNation()->getBackground(),
                                                      traveler->getNation()->getHighlight(),
                                                      0u,
                                                      false,
                                                      bBB,
                                                      bBR,
                                                      sBFS,
                                                      {},
                                                      SDLK_UNKNOWN,
                                                      nullptr,
                                                      true},
                                              printer));
          }}}};
    auto &nations = game.getNations();
    auto &nationBoxes = uiStates.at(UiState::beginning).boxesInfo;
    for (auto &nt : nations)
        // Create a button for each nation to start in that nation.
        nationBoxes.push_back({{sR.w * (static_cast<int>(nt.getId() - 1) % 3 * 2 + 1) / 6,
                                sR.h * (static_cast<int>(nt.getId() - 1) / 3 + 2) / 7, 0, 0},
                               nt.getNames(),
                               nt.getForeground(),
                               nt.getBackground(),
                               {},
                               nt.getId(),
                               true,
                               bBB,
                               bBR,
                               bBFS,
                               {},
                               SDLK_UNKNOWN,
                               [this, nt](MenuButton *) {
                                   unsigned int nId = nt.getId(); // nation id
                                   std::string name = pagers[0u].getVisible(0u)->getText(1u);
                                   if (name.empty())
                                       name = nt.randomName();
                                   // Create traveler object for player
                                   traveler = game.createPlayerTraveler(nId, name);
                                   show = true;
                                   focusBox = -1;
                                   setState(UiState::traveling);
                               }});
    fs::path path{"save"};
    std::vector<std::string> saves;
    for (auto &file : fs::directory_iterator{path})
        saves.push_back(file.path().stem().string());
    uiStates.at(UiState::loading)
        .boxesInfo.push_back(
            bigSelectButton({sR.w / 5, sR.h / 7, sR.w * 3 / 5, sR.h * 5 / 7}, saves, SDLK_l, [this, &path](MenuButton *btn) {
                game.loadGame((path / btn->getItem()).replace_extension("sav"));
                show = true;
                setState(UiState::traveling);
            }));
}

void Player::loadTraveler(const Save::Traveler *t, std::vector<Town> &ts) {
    // Load the traveler for the player from save file.
    traveler = std::make_shared<Traveler>(t, ts, game.getNations(), game.getData());
}

void Player::prepFocus(FocusGroup g, int &i, int &s, int &d, std::vector<TextBox *> &fcbls) {
    std::vector<Town *> neighbors;
    switch (g) {
    case box:
        i = focusBox;
        for (auto &pg : pagers) {
            // Put all visible boxes from all pages in focusables vector.
            auto bxs = pg.getVisible();
            fcbls.insert(fcbls.end(), bxs.begin(), bxs.end());
        }
        s = static_cast<int>(fcbls.size());
        d = kBoxDBS;
        break;
    case neighbor:
        neighbors = traveler->getTown()->getNeighbors();
        i = static_cast<int>(
            std::find_if(neighbors.begin(), neighbors.end(),
                         [this](Town *t) { return t->getId() == static_cast<unsigned int>(focusTown + 1); }) -
            neighbors.begin());
        s = static_cast<int>(neighbors.size());
        d = kTownDBS;
        if (i == s) {
            // Currently focused town is last neighbor.
            auto &ts = game.getTowns();
            size_t fT = focusTown;
            if (fT < ts.size())
                ts[fT].getBox()->changeBorder(-kTownDBS);
            i = -1;
        }
        fcbls.reserve(neighbors.size());
        for (auto &ng : neighbors)
            fcbls.push_back(ng->getBox());
        break;
    case town:
        i = focusTown;
        fcbls = game.getTownBoxes();
        s = static_cast<int>(fcbls.size());
        d = kTownDBS;
        break;
    }
}

void Player::finishFocus(int f, FocusGroup g, const std::vector<TextBox *> &fcbls) {
    switch (g) {
    case box:
        focusBox = f;
        break;
    case neighbor:
        if (f > -1)
            focusTown = static_cast<int>(fcbls[static_cast<size_t>(f)]->getId() - 1);
        else
            focusTown = -1;
        break;
    case town:
        focusTown = f;
        break;
    }
}

void Player::focus(int f, FocusGroup g) {
    // Focus item f from group g.
    int i, s, d;
    std::vector<TextBox *> fcbls;
    prepFocus(g, i, s, d, fcbls);
    if (i > -1 && i < s)
        fcbls[static_cast<size_t>(i)]->changeBorder(-d);
    if (f > -1 && f < s)
        fcbls[static_cast<size_t>(f)]->changeBorder(d);
    finishFocus(f, g, fcbls);
}

void Player::focusPrev(FocusGroup g) {
    // Focus previous item from group g.
    int i, s, d;
    std::vector<TextBox *> fcbls;
    prepFocus(g, i, s, d, fcbls);
    if (i == -1) {
        i = s;
        while (i--)
            if (fcbls[static_cast<size_t>(i)]->getCanFocus())
                break;
    } else {
        fcbls[static_cast<size_t>(i)]->changeBorder(-d);
        int j = i;
        while (--i != j)
            if (i < 0)
                i = s;
            else if (fcbls[static_cast<size_t>(i)]->getCanFocus())
                break;
    }
    if (i > -1) {
        fcbls[static_cast<size_t>(i)]->changeBorder(d);
        finishFocus(i, g, fcbls);
    }
}

void Player::focusNext(FocusGroup g) {
    // Focus next item from group g.
    int i, s, d;
    std::vector<TextBox *> fcbls;
    prepFocus(g, i, s, d, fcbls);
    if (i == -1) {
        // Nothing was focused, find first focusable item.
        while (++i < s)
            if (fcbls[static_cast<size_t>(i)]->getCanFocus())
                break;
        // No focusable item was found in group.
        return;
    } else {
        fcbls[static_cast<size_t>(i)]->changeBorder(-d);
        int j = i;
        while (++i != j)
            // Loop until we come back to the same item.
            if (i >= s)
                i = -1;
            else if (fcbls[static_cast<size_t>(i)]->getCanFocus())
                break;
    }
    fcbls[static_cast<size_t>(i)]->changeBorder(d);
    finishFocus(i, g, fcbls);
}

void Player::setState(UiState::State s) {
    // Change the UI state to s.
    UiState state = uiStates.at(s);
    pagers.clear();
    pagers.resize(state.pagerCount);
    fs::path path;
    std::vector<std::string> saves;
    // Create boxes for new state.
    for (auto &bI : state.boxesInfo)
        if (bI.onClick && bI.scrolls)
            pagers[0].addBox(std::make_unique<SelectButton>(bI));
        else if (bI.onClick)
            pagers[0].addBox(std::make_unique<MenuButton>(bI));
        else if (bI.scrolls)
            pagers[0].addBox(std::make_unique<ScrollBox>(bI));
        else
            pagers[0].addBox(std::make_unique<TextBox>(bI));
    switch (s) {
    case trading:
        break;
    case storing:
        storageButtonIndex = boxes.size();

        break;
    case managing:
        traveler->createManageButtons(pagers, printer);
        break;
    case equipping:
        equipButtonIndex = boxes.size();
        traveler->createEquipButtons(boxes, focusBox, equipButtonIndex, printer);
        break;
    case hiring:
        break;
    case attacking:
        traveler->createAttackButton(
            boxes, [this] { setState(fighting); }, printer);
        pause = true;
        break;
    case logging:

        break;
    default:
        break;
    }
    break;
case fighting:
    traveler->createFightBoxes(boxes, pause, printer);
    break;
case looting:
    rt = {screenRect.w / 15, screenRect.h * 14 / 15, 0, 0};
    tx.back() = "(D)one Looting";
    boxes.push_back(std::make_unique<MenuButton>(rt, tx, uIFgr, uIBgr, sBB, sBR, sBFS, printer, [this] {
        traveler->loseTarget();
        setState(traveling);
    }));
    boxes.back()->setKey(SDLK_d);
    rt.x += screenRect.w / 5;
    tx.back() = "(L)oot All";
    boxes.push_back(std::make_unique<MenuButton>(rt, tx, uIFgr, uIBgr, sBB, sBR, sBFS, printer,
                                                 [this, &tgt = *traveler->getTarget().lock()] {
                                                     traveler->loot(tgt);
                                                     traveler->loseTarget();
                                                     setState(traveling);
                                                 }));
    boxes.back()->setKey(SDLK_l);
    lootButtonIndex = boxes.size();
    traveler->createLootButtons(boxes, focusBox, lootButtonIndex, printer);
    pause = true;
    break;
case dying:
    rt = {screenRect.w / 2, screenRect.h / 2, 0, 0};
    tx = {traveler->getLogText().back(), "You have died."};
    boxes.push_back(std::make_unique<TextBox>(rt, tx, traveler->getNation()->getForeground(),
                                              traveler->getNation()->getBackground(), bBB, bBR, fFS, printer));
    break;
}
focusBox = pagers[0].getBox(0);
state = s;
}

void Player::handleKey(const SDL_KeyboardEvent &k) {
    SDL_Keymod mod = SDL_GetModState();
    if (mod & (KMOD_CTRL | KMOD_SHIFT | KMOD_ALT))
        modMultiplier = 10000.;
    else if (mod & (KMOD_CTRL | KMOD_ALT))
        modMultiplier = 0.001;
    else if (mod & (KMOD_SHIFT | KMOD_ALT))
        modMultiplier = 0.01;
    else if (mod & (KMOD_CTRL | KMOD_SHIFT))
        modMultiplier = 1000.;
    else if (mod & KMOD_ALT)
        modMultiplier = 0.1;
    else if (mod & KMOD_SHIFT)
        modMultiplier = 10.;
    else if (mod & KMOD_CTRL)
        modMultiplier = 100.;
    else
        modMultiplier = 1.;
    if (k.state == SDL_PRESSED) {
        // Event is SDL_KEYDOWN
        if (!boxes.empty()) {
            auto keyedBox = std::find_if(boxes.begin(), boxes.end(),
                                         [&k](const std::unique_ptr<TextBox> &t) { return k.keysym.sym == t->getKey(); });
            if (keyedBox != boxes.end()) {
                // A box's key was pressed.
                focus(static_cast<int>(std::distance(boxes.begin(), keyedBox)), box);
                (*keyedBox)->handleKey(k);
            }
        }
        if (focusBox < 0 || !boxes[static_cast<size_t>(focusBox)]->keyCaptured(k)) {
            if (state != quitting) {
                if (traveler) {
                    switch (state) {
                    case traveling:
                        switch (k.keysym.sym) {
                        case SDLK_LEFT:
                        case SDLK_RIGHT:
                        case SDLK_UP:
                        case SDLK_DOWN:
                            scrollKeys.insert(k.keysym.sym);
                            show = false;
                            break;
                        case SDLK_n:
                            if (mod & KMOD_SHIFT)
                                focusPrev(neighbor);
                            else
                                focusNext(neighbor);
                            break;
                        }
                        break;
                    case trading:
                    case storing:
                    case managing:
                        switch (k.keysym.sym) {
                        case SDLK_LEFT:
                            focusPrev(box);
                            break;
                        case SDLK_RIGHT:
                            focusNext(box);
                            break;
                        case SDLK_UP:
                            for (int i = 0; i < 7; ++i)
                                focusPrev(box);
                            break;
                        case SDLK_DOWN:
                            for (int i = 0; i < 7; ++i)
                                focusNext(box);
                            break;
                        case SDLK_COMMA:
                            traveler->changePortion(-0.1);
                            updatePortionBox();
                            break;
                        case SDLK_PERIOD:
                            traveler->changePortion(0.1);
                            updatePortionBox();
                            break;
                        }
                        break;
                    default:
                        break;
                    }
                }
            }
            switch (k.keysym.sym) {
            case SDLK_ESCAPE:
                if (state == quitting)
                    setState(storedState);
                else {
                    storedState = state;
                    setState(quitting);
                }
                break;
            case SDLK_TAB:
                if (mod & KMOD_SHIFT)
                    focusPrev(box);
                else
                    focusNext(box);
                break;
            }
        } else
            boxes[static_cast<size_t>(focusBox)]->handleKey(k);
    } else {
        // Event is SDL_KEYUP.
        if (!boxes.empty()) {
            // Find a box which handles this key.
            auto keyedBox = std::find_if(boxes.begin(), boxes.end(),
                                         [&k](const std::unique_ptr<TextBox> &t) { return k.keysym.sym == t->getKey(); });
            if (keyedBox != boxes.end())
                // A box's key was released.
                (*keyedBox)->handleKey(k);
            if (!(focusBox > -1 && boxes[static_cast<size_t>(focusBox)]->keyCaptured(k)))
                // Key is not used by currently focused box.
                switch (k.keysym.sym) {
                case SDLK_LEFT:
                case SDLK_RIGHT:
                case SDLK_UP:
                case SDLK_DOWN:
                    scrollKeys.erase(k.keysym.sym);
                    break;
                }
            else
                boxes[static_cast<size_t>(focusBox)]->handleKey(k);
        }
    }
}

void Player::handleTextInput(const SDL_TextInputEvent &t) {
    if (focusBox > -1)
        boxes[static_cast<size_t>(focusBox)]->handleTextInput(t);
}

void Player::handleClick(const SDL_MouseButtonEvent &b) {
    if (state == traveling) {
        std::vector<TextBox *> fcbls = game.getTownBoxes();
        auto clickedTown = std::find_if(fcbls.begin(), fcbls.end(), [&b](const TextBox *t) { return t->clickCaptured(b); });
        if (clickedTown != fcbls.end())
            focus(static_cast<int>(std::distance(fcbls.begin(), clickedTown)), town);
    }
    if (!boxes.empty()) {
        auto clickedBox = std::find_if(boxes.begin(), boxes.end(), [&b](const std::unique_ptr<TextBox> &t) {
            return t->getCanFocus() && t->clickCaptured(b);
        });
        if (clickedBox == boxes.end()) {
            // None of the boxes was clicked, check pagers.
            focus(-1, box);
        } else {
            // One of the boxes was clicked.
            int cI = static_cast<int>(clickedBox - boxes.begin());
            if (cI != focusBox)
                focus(cI, box);
            (*clickedBox)->handleClick(b);
        }
    }
}

void Player::handleEvent(const SDL_Event &e) {
    switch (e.type) {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        handleKey(e.key);
        break;
    case SDL_TEXTINPUT:
        handleTextInput(e.text);
        break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        handleClick(e.button);
        break;
    case SDL_QUIT:
        stop = true;
        break;
    }
}

void Player::update(unsigned int e) {
    // Update the UI to reflect current state.
    auto t = traveler.get();
    bool target = t && t->getTarget().lock();
    switch (state) {
    case traveling:
        if (target) {
            pause = true;
            setState(fighting);
        }
        break;
    case trading:
        traveler->updateTradeButtons(boxes, offerButtonIndex, requestButtonIndex);
        break;
    case fighting:
        if (!traveler->alive()) {
            setState(dying);
            break;
        } else if (!target) {
            setState(logging);
            break;
        } else if (traveler->fightWon()) {
            setState(looting);
            break;
        }
        traveler->updateFightBoxes(boxes);
        break;
    default:
        break;
    }
    int scrollX = 0, scrollY = 0, sS = Settings::getScroll();
    if (show) {
        const SDL_Rect &mR = game.getMapRect();
        if (traveler->getPX() < int(mR.w * kShowPlayerPadding))
            scrollX = -sS;
        if (traveler->getPY() < int(mR.h * kShowPlayerPadding))
            scrollY = -sS;
        if (traveler->getPX() > int(mR.w * (1 - kShowPlayerPadding)))
            scrollX = sS;
        if (traveler->getPY() > int(mR.h * (1 - kShowPlayerPadding)))
            scrollY = sS;
    }
    if (!(scrollKeys.empty())) {
        auto upIt = scrollKeys.find(SDLK_UP), dnIt = scrollKeys.find(SDLK_DOWN), lfIt = scrollKeys.find(SDLK_LEFT),
             rtIt = scrollKeys.find(SDLK_RIGHT);
        scrollX -= (lfIt != scrollKeys.end()) * static_cast<int>(static_cast<double>(sS) * modMultiplier);
        scrollX += (rtIt != scrollKeys.end()) * static_cast<int>(static_cast<double>(sS) * modMultiplier);
        scrollY -= (upIt != scrollKeys.end()) * static_cast<int>(static_cast<double>(sS) * modMultiplier);
        scrollY += (dnIt != scrollKeys.end()) * static_cast<int>(static_cast<double>(sS) * modMultiplier);
    }
    if (scrollX || scrollY)
        game.moveView(scrollX, scrollY);
    if (traveler.get() && !pause)
        traveler->update(e);
}

void Player::updatePortionBox() { boxes[portionBoxIndex]->setText(0, traveler->portionString()); }

void Player::draw(SDL_Renderer *s) {
    if (traveler.get())
        traveler->draw(s);
    for (auto &bx : boxes)
        bx->draw(s);
    for (auto &pg : pagers)
        pg.draw(s);
}

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

Player::Player(Game &g) : game(g), screenRect(Settings::getScreenRect()), printer(g.getPrinter()) {
    printer.setSize(Settings::boxSize(BoxSizeType::small));
    smallBoxFontHeight = printer.getFontHeight();
    int bBB = Settings::boxSize(BoxSizeType::big).border;
    uIStates[static_cast<size_t>(State::starting)] = {
        {Settings::boxInfo({screenRect.w / 2, screenRect.h / 15, 0, 0}, {"Camels and Silk"}, BoxSizeType::big),
         Settings::boxInfo({screenRect.w / 7, screenRect.h / 3, 0, 0}, {"(N)ew Game"}, BoxSizeType::big, SDLK_n,
                           [this](MenuButton *) {
                               // Load data to create a new game.
                               auto &nations = game.newGame();
                               auto &beginningBoxes = uIStates[static_cast<size_t>(State::beginning)].boxesInfo;
                               beginningBoxes.reserve(beginningBoxes.size() + nations.size());
                               for (auto &nt : nations)
                                   // Create a button for each nation to start in that nation.
                                   beginningBoxes.push_back(Settings::boxInfo(
                                       {screenRect.w * (static_cast<int>(nt.getId() - 1) % 3 * 2 + 1) / 6,
                                        screenRect.h * (static_cast<int>(nt.getId() - 1) / 3 + 2) / 7, 0, 0},
                                       nt.getNames(), nt.getColors(), {nt.getId(), true}, BoxSizeType::big,
                                       SDLK_UNKNOWN, [this](MenuButton *btn) {
                                           unsigned int nId = btn->getId(); // nation id
                                           std::string name = pagers[0].getVisible(0)->getText(1);
                                           // Create traveler object for player
                                           traveler = game.createPlayerTraveler(nId, name);
                                           show = true;
                                           focusBox = -1;
                                           setState(State::traveling);
                                       }));
                               setState(State::beginning);
                           }),
         Settings::boxInfo({screenRect.w / 7, screenRect.h * 2 / 3, 0, 0}, {"(L)oad Game"}, BoxSizeType::big,
                           SDLK_l, [this](MenuButton *) { setState(State::loading); })}};
    uIStates[static_cast<size_t>(State::beginning)] = {
        {Settings::boxInfo({screenRect.w / 2, screenRect.h / 7, 0, 0}, {"Name", ""}, BoxSizeType::big)}};
    uIStates[static_cast<size_t>(State::quitting)] = {
        {Settings::boxInfo({screenRect.w / 2, screenRect.h * 3 / 4, 0, 0}, {""}, BoxSizeType::big, SDLK_q,
                           [this](MenuButton *) {
                               if (traveler) game.saveGame();
                               stop = true;
                           }),
         Settings::boxInfo({screenRect.w / 2, screenRect.h / 4, 0, 0}, {"Continue"}, BoxSizeType::big, SDLK_ESCAPE,
                           [this](MenuButton *) {
                               pause = storedPause;
                               setState(storedState);
                           })},
        [this] {
            storedState = state;
            storedPause = pause;
            pagers[0].getVisible(0)->setText(0, traveler ? "Save and Quit" : "Quit");
        }};
    uIStates[static_cast<size_t>(State::loading)] = {
        {Settings::boxInfo({screenRect.w / 7, screenRect.h / 7, 0, 0}, {"(B)ack"}, BoxSizeType::big, SDLK_b,
                           [this](MenuButton *) { setState(State::starting); })}};
    uIStates[static_cast<size_t>(State::traveling)] = {
        {Settings::boxInfo({screenRect.w / 9, screenRect.h - smallBoxFontHeight, 0, 0}, {"(G)o"}, BoxSizeType::small, SDLK_g,
                           [this](MenuButton *btn) {
                               if (focusTown > -1)
                                   game.pickTown(traveler.get(), static_cast<size_t>(focusTown));
                               show = true;
                               btn->setClicked(false);
                           }),
         Settings::boxInfo({screenRect.w * 2 / 9, screenRect.h - smallBoxFontHeight, 0, 0}, {"(T)rade"},
                           BoxSizeType::small, SDLK_t, [this](MenuButton *) { setState(State::trading); }),
         Settings::boxInfo({screenRect.w / 3, screenRect.h - smallBoxFontHeight, 0, 0}, {"(S)tore"},
                           BoxSizeType::small, SDLK_s, [this](MenuButton *) { setState(State::storing); }),
         Settings::boxInfo({screenRect.w * 4 / 9, screenRect.h - smallBoxFontHeight, 0, 0}, {"(B)uild"},
                           BoxSizeType::small, SDLK_b, [this](MenuButton *) { setState(State::building); }),
         Settings::boxInfo({screenRect.w * 5 / 9, screenRect.h - smallBoxFontHeight, 0, 0}, {"(E)quip"},
                           BoxSizeType::small, SDLK_e, [this](MenuButton *) { setState(State::equipping); }),
         Settings::boxInfo({screenRect.w * 2 / 3, screenRect.h - smallBoxFontHeight, 0, 0}, {"(M)anage"},
                           BoxSizeType::small, SDLK_m, [this](MenuButton *) { setState(State::managing); }),
         Settings::boxInfo({screenRect.w * 7 / 9, screenRect.h - smallBoxFontHeight, 0, 0}, {"(A)ttack"},
                           BoxSizeType::small, SDLK_a, [this](MenuButton *) { setState(State::attacking); }),
         Settings::boxInfo({screenRect.w * 8 / 9, screenRect.h - smallBoxFontHeight, 0, 0}, {"(L)og"},
                           BoxSizeType::small, SDLK_l, [this](MenuButton *) { setState(State::logging); })},
        [this] { pause = false; }};
    uIStates[static_cast<size_t>(State::trading)] = {
        {Settings::boxInfo({screenRect.w * 2 / 9, screenRect.h - smallBoxFontHeight, 0, 0}, {"Stop (T)rading"},
                           BoxSizeType::small, SDLK_t, [this](MenuButton *) { setState(State::traveling); }),
         Settings::boxInfo({screenRect.w * 2 / 9, screenRect.h - (smallBoxFontHeight + bBB) * 2, 0, 0},
                           {"(C)omplete Trade"}, BoxSizeType::small, SDLK_c,
                           [this](MenuButton *) {
                               traveler->makeTrade();
                               setState(State::trading);
                           })

        },
        [this] { traveler->createTradeButtons(pagers, printer); },
        3};
    uIStates[static_cast<size_t>(State::storing)] = {
        {Settings::boxInfo({screenRect.w / 3, screenRect.h - smallBoxFontHeight, 0, 0}, {"Stop (S)toring"},
                           BoxSizeType::small, SDLK_s, [this](MenuButton *) { setState(State::traveling); })},
        [this] { traveler->createStorageButtons(pagers, focusBox, printer); },
        3};
    uIStates[static_cast<size_t>(State::building)] = {
        {Settings::boxInfo({screenRect.w * 4 / 9, screenRect.h - smallBoxFontHeight, 0, 0}, {"Stop (B)uilding"},
                           BoxSizeType::small, SDLK_b, [this](MenuButton *) { setState(State::traveling); })},
        [this] { traveler->createBuildButtons(pagers, focusBox, printer); },
        3};
    uIStates[static_cast<size_t>(State::equipping)] = {
        {Settings::boxInfo({screenRect.w * 5 / 9, screenRect.h - smallBoxFontHeight, 0, 0}, {"Stop (E)quipping"},
                           BoxSizeType::small, SDLK_e, [this](MenuButton *) { setState(State::traveling); })},
        [this] { traveler->createEquipButtons(pagers, focusBox, printer); },
        2};
    uIStates[static_cast<size_t>(State::managing)] = {
        {Settings::boxInfo({screenRect.w * 2 / 3, screenRect.h - smallBoxFontHeight, 0, 0}, {"Stop (M)anaging"},
                           BoxSizeType::small, SDLK_m, [this](MenuButton *) { setState(State::traveling); })}};
    uIStates[static_cast<size_t>(State::attacking)] = {
        {Settings::boxInfo({screenRect.w * 7 / 9, screenRect.h - smallBoxFontHeight, 0, 0}, {"Cancel (A)ttack"},
                           BoxSizeType::small, SDLK_a, [this](MenuButton *) { setState(State::traveling); })},
        [this] {
            pause = true;
            traveler->createAttackButton(
                pagers[0], [this] { setState(State::fighting); }, printer);
        }};
    uIStates[static_cast<size_t>(State::logging)] = {
        {Settings::boxInfo({screenRect.w * 8 / 9, screenRect.h - smallBoxFontHeight, 0, 0}, {"Close (L)og"},
                           BoxSizeType::small, SDLK_l, [this](MenuButton *) { setState(State::traveling); })},
        [this] { traveler->createLogBox(pagers[0], printer); }};
    uIStates[static_cast<size_t>(State::fighting)] = {
        {}, [this] { traveler->createFightBoxes(pagers[0], pause, printer); }};
    uIStates[static_cast<size_t>(State::looting)] = {
        {Settings::boxInfo({screenRect.w / 15, screenRect.h - smallBoxFontHeight, 0, 0}, {"Stop (L)ooting"},
                           BoxSizeType::small, SDLK_l,
                           [this](MenuButton *) {
                               traveler->loseTarget();
                               setState(State::traveling);
                           }),
         Settings::boxInfo({screenRect.w * 4 / 15, screenRect.h - smallBoxFontHeight, 0, 0}, {"Loot (A)ll"},
                           BoxSizeType::small, SDLK_a,
                           [this](MenuButton *) {
                               traveler->loot();
                               traveler->loseTarget();
                               setState(State::traveling);
                           })},
        [this] { traveler->createLootButtons(pagers, focusBox, printer); },
        3};
    uIStates[static_cast<size_t>(State::dying)] = {
        {}, [this] {
            pagers[0].addBox(std::make_unique<TextBox>(
                Settings::boxInfo({screenRect.w / 2, screenRect.h / 2, 0, 0},
                                  {traveler->getLogText().back(), "You have died."},
                                  traveler->getNation()->getColors(), BoxSizeType::big),
                printer));
        }};

    fs::path path{"save"};
    std::vector<std::string> saves;
    for (auto &file : fs::directory_iterator{path}) saves.push_back(file.path().stem().string());
    uIStates[static_cast<size_t>(State::loading)].boxesInfo.push_back(
        Settings::boxInfo({screenRect.w / 5, screenRect.h / 7, screenRect.w * 3 / 5, screenRect.h * 5 / 7},
                          saves, BoxSizeType::big, BoxBehavior::scroll, SDLK_l, [this, &path](MenuButton *btn) {
                              game.loadGame((path / btn->getItem()).replace_extension("sav"));
                              show = true;
                              setState(State::traveling);
                          }));
}

void Player::loadTraveler(const Save::Traveler *ldTvl, const std::vector<Nation> &nts, std::vector<Town> &tns,
                          const GameData &gD) {
    // Load the traveler for the player from save file.
    traveler = std::make_unique<Traveler>(ldTvl, nts, tns, gD);
}

void Player::prepFocus(FocusGroup g, int &i, int &s, std::vector<TextBox *> &fcbls) {
    std::vector<Town *> neighbors;
    switch (g) {
    case FocusGroup::box:
        i = focusBox;
        for (auto &pg : pagers) {
            // Put all visible boxes from all pages in focusables vector.
            auto bxs = pg.getVisible();
            fcbls.insert(end(fcbls), begin(bxs), end(bxs));
        }
        s = static_cast<int>(fcbls.size());
        break;
    case FocusGroup::neighbor:
        neighbors = traveler->town()->getNeighbors();
        i = static_cast<int>(
            std::find_if(begin(neighbors), end(neighbors),
                         [this](Town *t) { return t->getId() == static_cast<unsigned int>(focusTown + 1); }) -
            begin(neighbors));
        s = static_cast<int>(neighbors.size());
        if (i == s) {
            // Currently focused town is last neighbor.
            auto &ts = game.getTowns();
            size_t fT = focusTown;
            if (fT < ts.size()) ts[fT].getBox()->toggleFocus();
            i = -1;
        }
        fcbls.reserve(neighbors.size());
        for (auto &ng : neighbors) fcbls.push_back(ng->getBox());
        break;
    case FocusGroup::town:
        i = focusTown;
        fcbls = game.getTownBoxes();
        s = static_cast<int>(fcbls.size());
        break;
    }
}

void Player::finishFocus(int f, FocusGroup g, const std::vector<TextBox *> &fcbls) {
    switch (g) {
    case FocusGroup::box:
        focusBox = f;
        // Set current pager to pager that contains focus box.
        for (currentPager = begin(pagers); currentPager != end(pagers); ++currentPager) {
            int vC = currentPager->visibleCount();
            if (f >= vC)
                f -= vC;
            else
                break;
        }
        break;
    case FocusGroup::neighbor:
        if (f > -1)
            focusTown = static_cast<int>(fcbls[static_cast<size_t>(f)]->getId() - 1);
        else
            focusTown = -1;
        break;
    case FocusGroup::town:
        focusTown = f;
        break;
    }
}

void Player::focus(int f, FocusGroup g) {
    // Focus item f from group g.
    int i, s;
    std::vector<TextBox *> fcbls;
    prepFocus(g, i, s, fcbls);
    if (i == f) return;
    if (i > -1 && i < s) fcbls[static_cast<size_t>(i)]->toggleFocus();
    if (f > -1 && f < s) fcbls[static_cast<size_t>(f)]->toggleFocus();
    finishFocus(f, g, fcbls);
}

void Player::focusPrev(FocusGroup g) {
    // Focus previous item from group g.
    int i, s;
    std::vector<TextBox *> fcbls;
    prepFocus(g, i, s, fcbls);
    if (i == -1) {
        i = s;
        while (i--)
            if (fcbls[static_cast<size_t>(i)]->canFocus()) break;
        if (i < 0)
            // No focusable item found in group.
            return;
    } else {
        fcbls[static_cast<size_t>(i)]->toggleFocus();
        int j = i;
        while (--i != j)
            if (i < 0)
                i = s;
            else if (fcbls[static_cast<size_t>(i)]->canFocus())
                break;
    }
    fcbls[static_cast<size_t>(i)]->toggleFocus();
    finishFocus(i, g, fcbls);
}

void Player::focusNext(FocusGroup g) {
    // Focus next item from group g.
    int i, s;
    std::vector<TextBox *> fcbls;
    prepFocus(g, i, s, fcbls);
    if (i == -1) {
        // Nothing was focused, find first focusable item.
        while (++i < s)
            if (fcbls[static_cast<size_t>(i)]->canFocus()) break;
        if (i == s) {
            i = -1;
            // No focusable item was found in group.
            return;
        }
    } else {
        fcbls[static_cast<size_t>(i)]->toggleFocus();
        int j = i;
        while (++i != j)
            // Loop until we come back to the same item.
            if (i == s)
                i = -1;
            else if (fcbls[static_cast<size_t>(i)]->canFocus())
                break;
    }
    fcbls[static_cast<size_t>(i)]->toggleFocus();
    finishFocus(i, g, fcbls);
}

void Player::setState(State s) {
    // Change the UI state to s.
    auto framerateText = framerateBox ? framerateBox->getText() : std::vector<std::string>{"Framerate:", ""};
    UIState newState = uIStates[static_cast<size_t>(s)];
    pagers.clear();
    pagers.resize(newState.pagerCount);
    currentPager = begin(pagers);
    focusBox = -1;
    fs::path path;
    std::vector<std::string> saves;
    // Create boxes for new state.
    for (auto &bI : newState.boxesInfo)
        if (bI.onClick && bI.behavior == BoxBehavior::scroll)
            pagers[0].addBox(std::make_unique<SelectButton>(bI, printer));
        else if (bI.onClick)
            pagers[0].addBox(std::make_unique<MenuButton>(bI, printer));
        else if (bI.behavior == BoxBehavior::scroll)
            pagers[0].addBox(std::make_unique<ScrollBox>(bI, printer));
        else
            pagers[0].addBox(std::make_unique<TextBox>(bI, printer));
    if (newState.onChange) newState.onChange();
    // Create boxes shared by multiple states.
    pagers[0].addBox(std::make_unique<TextBox>(
        Settings::boxInfo({screenRect.w / 15, screenRect.h / 31, 0, 0}, framerateText, BoxSizeType::small), printer));
    if (newState.pagerCount > 2) {
        int bBB = Settings::boxSize(BoxSizeType::big).border;
        // Create portion box and set portion button.
        pagers[0].addBox(std::make_unique<TextBox>(
            Settings::boxInfo({screenRect.w / 9, screenRect.h - smallBoxFontHeight, 0, 0}, {""},
                              BoxSizeType::small, BoxBehavior::edit),
            printer));
        portionBox = pagers[0].getVisible().back();
        traveler->updatePortionBox(portionBox);
        pagers[0].addBox(std::make_unique<MenuButton>(
            Settings::boxInfo({screenRect.w / 9, screenRect.h - (smallBoxFontHeight + bBB) * 2, 0, 0},
                              {"(S)et Portion"}, BoxSizeType::small, SDLK_s,
                              [this](MenuButton *btn) {
                                  double p;
                                  std::stringstream{portionBox->getText(0)} >> p;
                                  traveler->setPortion(p);
                                  traveler->updatePortionBox(portionBox);
                                  btn->setClicked(false);
                              }),
            printer));
        // Create page switching buttons.
        std::array<SDL_Keycode, 4> keys{SDLK_MINUS, SDLK_EQUALS, SDLK_LEFTBRACKET, SDLK_RIGHTBRACKET};
        auto kyIt = keys.begin();
        for (auto pgIt = begin(pagers) + 1; pgIt != end(pagers); ++pgIt) {
            auto &bnds = pgIt->getBounds();
            // Create label.
            auto typeText = s == State::building ? "Businesses" : "Goods";
            pagers[0].addBox(std::make_unique<TextBox>(
                Settings::boxInfo(
                    {bnds.x + bnds.w / 2, bnds.y - smallBoxFontHeight, 0, 0},
                    {pgIt == begin(pagers) + 1 ?
                         traveler->getName() + "'s " + typeText :
                         s == State::storing ?
                         "Goods in " + traveler->town()->getName() :
                         (s == State::looting ? traveler->getTarget()->getName() : traveler->town()->getName()) + "'s " + typeText},
                    BoxSizeType::small),
                printer));
            if (pgIt->pageCount() > 1) {
                // Create page buttons.
                pagers[0].addBox(std::make_unique<MenuButton>(
                    Settings::boxInfo({bnds.x + bnds.w / 6, bnds.y + bnds.h + smallBoxFontHeight, 0, 0},
                                      {"Previous Page"}, BoxSizeType::small, *kyIt++,
                                      [pgIt](MenuButton *btn) {
                                          pgIt->recedePage();
                                          btn->setClicked(false);
                                      }),
                    printer));
                pagers[0].addBox(std::make_unique<MenuButton>(
                    Settings::boxInfo({bnds.x + bnds.w * 5 / 6, bnds.y + bnds.h + smallBoxFontHeight, 0, 0},
                                      {"Next Page"}, BoxSizeType::small, *kyIt++,
                                      [pgIt](MenuButton *btn) {
                                          pgIt->advancePage();
                                          btn->setClicked(false);
                                      }),
                    printer));
            } else
                kyIt += 2;
        }
    }
    focusNext(FocusGroup::box);
    state = s;
}

void Player::handleKey(const SDL_KeyboardEvent &k) {
    SDL_Keymod mod = SDL_GetModState();
    if ((mod & KMOD_CTRL) && (mod & KMOD_ALT) && (mod & KMOD_SHIFT))
        modMultiplier = 10000;
    else if ((mod & KMOD_CTRL) && (mod & KMOD_ALT))
        modMultiplier = 0.001;
    else if ((mod & KMOD_CTRL) && (mod & KMOD_SHIFT))
        modMultiplier = 1000;
    else if ((mod & KMOD_ALT) && (mod & KMOD_SHIFT))
        modMultiplier = 0.01;
    else if (mod & KMOD_CTRL)
        modMultiplier = 100;
    else if (mod & KMOD_ALT)
        modMultiplier = 0.1;
    else if (mod & KMOD_SHIFT)
        modMultiplier = 10;
    else
        modMultiplier = 1;
    int keyedIndex = -1, indexOffset = 0;
    for (auto &pgr : pagers) {
        keyedIndex = pgr.getKeyedIndex(k);
        if (keyedIndex > -1) {
            // Something on this page was keyed.
            pgr.getVisible(keyedIndex)->handleKey(k);
            break;
        }
        indexOffset += pgr.visibleCount();
    }
    if (k.state == SDL_PRESSED) {
        // Key was pressed down.
        if (keyedIndex > -1)
            // A box's key was pressed.
            focus(keyedIndex + indexOffset, FocusGroup::box);
        if (state != State::quitting) {
            if (traveler) {
                int columnCount = state == State::building ? Settings::getBusinessButtonColumns() :
                                                             Settings::getGoodButtonColumns();
                switch (state) {
                case State::beginning:
                case State::traveling:
                    switch (k.keysym.sym) {
                    case SDLK_LEFT:
                        scroll.insert(Direction::left);
                        show = false;
                        break;
                    case SDLK_RIGHT:
                        scroll.insert(Direction::right);
                        show = false;
                        break;
                    case SDLK_UP:
                        scroll.insert(Direction::up);
                        show = false;
                        break;
                    case SDLK_DOWN:
                        scroll.insert(Direction::down);
                        show = false;
                        break;
                    case SDLK_n:
                        if (mod & KMOD_SHIFT)
                            focusPrev(FocusGroup::neighbor);
                        else
                            focusNext(FocusGroup::neighbor);
                        break;
                    }
                    break;
                case State::trading:
                    if (developer) switch (k.keysym.sym) {
                        case SDLK_z:
                            traveler->resetTown();
                            break;
                        case SDLK_x:
                            traveler->toggleMaxGoods();
                            break;
                        case SDLK_v:
                            traveler->adjustDemand(pagers[2], -modMultiplier);
                            break;
                        case SDLK_b:
                            traveler->adjustDemand(pagers[2], modMultiplier);
                            break;
                        case SDLK_n:
                            traveler->adjustAreas(pagers[2], -modMultiplier);
                            break;
                        case SDLK_m:
                            traveler->adjustAreas(pagers[2], modMultiplier);
                            break;
                        case SDLK_r:
                            game.generateRoutes();
                            break;
                        case SDLK_s:
                            game.saveData();
                            break;
                        }
                case State::storing:
                case State::building:
                    switch (k.keysym.sym) {
                    case SDLK_LEFT:
                        focusPrev(FocusGroup::box);
                        break;
                    case SDLK_RIGHT:
                        focusNext(FocusGroup::box);
                        break;
                    case SDLK_UP:
                        for (int i = 0; i < columnCount; ++i) focusPrev(FocusGroup::box);
                        break;
                    case SDLK_DOWN:
                        for (int i = 0; i < columnCount; ++i) focusNext(FocusGroup::box);
                        break;
                    case SDLK_COMMA:
                        traveler->changePortion(-0.1);
                        traveler->updatePortionBox(portionBox);
                        break;
                    case SDLK_PERIOD:
                        traveler->changePortion(0.1);
                        traveler->updatePortionBox(portionBox);
                        break;
                    }
                    break;
                default:
                    break;
                }
            }
            // Keyboard shortcuts for all non-quitting states
            switch (k.keysym.sym) {
            case SDLK_ESCAPE:
                storedState = state;
                setState(State::quitting);
                break;
            }
        }
        // Keyboard shortcuts for all states.
        switch (k.keysym.sym) {
        case SDLK_BACKQUOTE:
            developer = !developer;
            break;
        case SDLK_TAB:
            if (mod & KMOD_SHIFT)
                focusPrev(FocusGroup::box);
            else
                focusNext(FocusGroup::box);
            break;
        }
    } else {
        // Key was lifted up.
        switch (k.keysym.sym) {
        case SDLK_LEFT:
            scroll.erase(Direction::left);
            break;
        case SDLK_RIGHT:
            scroll.erase(Direction::right);
            break;
        case SDLK_UP:
            scroll.erase(Direction::up);
            break;
        case SDLK_DOWN:
            scroll.erase(Direction::down);
            break;
        }
    }
}

void Player::handleTextInput(const SDL_TextInputEvent &t) {
    if (focusBox > -1) {
        int indexOffset = 0;
        for (auto &pgr : pagers) {
            int vC = pgr.visibleCount();
            if (focusBox - indexOffset < vC) {
                // Focus box is in this pager.
                pgr.getVisible(focusBox - indexOffset)->handleTextInput(t);
                break;
            }
            indexOffset += vC;
        }
    }
}

void Player::handleClick(const SDL_MouseButtonEvent &b) {
    if (state == State::traveling) {
        std::vector<TextBox *> fcbls = game.getTownBoxes();
        auto clickedTown =
            std::find_if(begin(fcbls), end(fcbls), [&b](const TextBox *t) { return t->clickCaptured(b); });
        if (clickedTown != end(fcbls))
            focus(static_cast<int>(std::distance(begin(fcbls), clickedTown)), FocusGroup::town);
    }
    int clickedIndex = -1, indexOffset = 0;
    for (auto &pgr : pagers) {
        clickedIndex = pgr.getClickedIndex(b);
        if (clickedIndex > -1) {
            pgr.getVisible(clickedIndex)->handleClick(b);
            break;
        }
        indexOffset += pgr.visibleCount();
    }
    if (b.state == SDL_PRESSED) {
        if (clickedIndex < 0)
            focus(-1, FocusGroup::box);
        else
            focus(clickedIndex + indexOffset, FocusGroup::box);
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

void Player::update(unsigned int elTm) {
    // Update the UI to reflect current state.
    auto t = traveler.get();
    bool target = t && t->getTarget();
    switch (state) {
    case State::traveling:
        if (target) {
            pause = true;
            setState(State::fighting);
        }
        break;
    case State::trading:
        traveler->updateTradeButtons(pagers);
        break;
    case State::fighting:
        if (!traveler->alive())
            setState(State::dying);
        else if (!target)
            setState(State::logging);
        else if (traveler->fightWon())
            setState(State::looting);
        else
            traveler->updateFightBoxes(pagers[0]);
        break;
    default:
        break;
    }
    SDL_Point scl{0, 0};
    double sS = Settings::getScroll() * modMultiplier;
    if (show) {
        const SDL_Rect &mR = game.getMapView();
        auto &point = traveler->getPosition().getPoint();
        if (point.x < int(mR.w * kShowPlayerPadding)) scl.x = -sS;
        if (point.y < int(mR.h * kShowPlayerPadding)) scl.y = -sS;
        if (point.x > int(mR.w * (1 - kShowPlayerPadding))) scl.x = sS;
        if (point.y > int(mR.h * (1 - kShowPlayerPadding))) scl.y = sS;
    }
    std::for_each(begin(scroll), end(scroll), [&scl, sS](Direction dir) {
        switch (dir) {
        case Direction::left:
            scl.x -= sS;
            break;
        case Direction::right:
            scl.x += sS;
            break;
        case Direction::up:
            scl.y -= sS;
            break;
        case Direction::down:
            scl.y += sS;
            break;
        }
    });
    if (scl.x || scl.y) game.moveView(scl);
    totalElapsed += elTm;
    if (++frameCount > kFramerateInterval) {
        framerateBox->setText(
            1, std::to_string(kMillisecondsPerSecond * kFramerateInterval / totalElapsed) + "fps");
        totalElapsed = 0;
        frameCount = 0;
    }
    if (traveler.get() && !pause) traveler->update(elTm);
}

void Player::draw(SDL_Renderer *s) {
    if (traveler.get()) traveler->draw(s);
    for (auto &pgr : pagers) pgr.draw(s);
}

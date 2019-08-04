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

#ifndef SETTINGS_H
#define SETTINGS_H
#include <array>
#include <chrono>
#include <random>
#include <unordered_map>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
namespace fs = boost::filesystem;
namespace pt = boost::property_tree;
#include <SDL2/SDL.h>

class MenuButton;

struct Image {
    SDL_Surface *surface;
    SDL_Rect rect;
};

struct BoxInfo {
    SDL_Rect rect{0, 0, 0, 0};
    std::vector<std::string> text;
    SDL_Color foreground{0, 0, 0, 0}, background{0, 0, 0, 0}, highlight{0, 0, 0, 0};
    unsigned int id = 0;
    bool isNation = false, canFocus = false, canEdit = false;
    int border = 0, radius = 0, fontSize = 0;
    std::vector<Image> images;
    SDL_Keycode key = SDLK_UNKNOWN;
    std::function<void(MenuButton *)> onClick = nullptr;
    bool scroll = false;
    SDL_Rect outsideRect{0, 0, 0, 0};
};

class Settings {
    static SDL_Rect screenRect;
    static SDL_Rect mapRect;
    static SDL_Color uIForeground;
    static SDL_Color uIBackground;
    static SDL_Color uIHighlight;
    static SDL_Color loadBarColor;
    static SDL_Color routeColor;
    static SDL_Color waterColor;
    static SDL_Color playerColor;
    static SDL_Color aIColor;
    static int scroll, offsetX, offsetY;
    static double scale;
    static int bigBoxBorder;
    static int bigBoxRadius;
    static int bigBoxFontSize;
    static int loadBarFontSize;
    static int smallBoxBorder;
    static int smallBoxRadius;
    static int smallBoxFontSize;
    static int fightFontSize;
    static int equipBorder;
    static int equipRadius;
    static int equipFontSize;
    static int townFontSize;
    static int tradeBorder;
    static int tradeRadius;
    static int tradeFontSize;
    static int goodButtonSizeMultiplier;
    static int goodButtonSpaceMultiplier;
    static int goodButtonXDivisor;
    static int goodButtonYDivisor;
    static int businessButtonSizeMultiplier;
    static int businessButtonSpaceMultiplier;
    static int businessButtonXDivisor;
    static int businessButtonYDivisor;
    static int dayLength;                  // length of a day in milliseconds
    static unsigned int businessHeadStart; // number of milliseconds to run before game starts on new game
    static int businessRunTime;            // time between business cycles in milliseconds
    static int travelersCheckTime;         // time between checks of dead travelers in milliseconds
    static int aIDecisionTime;             // time between AI cycles in milliseconds
    static double consumptionSpaceFactor, inputSpaceFactor, outputSpaceFactor;
    static int minPriceDivisor;
    static double townProfit;
    static size_t maxTowns;
    static size_t maxNeighbors;
    static double travelersExponent;
    static int travelersMin;
    static unsigned int statMax;
    static double attackDistSq;
    static double escapeChance;
    static std::array<double, 6> aITypeWeights;
    static int criteriaMax;
    static unsigned int aITownRange;
    static double limitFactorMin, limitFactorMax;
    static double aIAttackThreshold;
    static std::mt19937 rng;

public:
    static void load(const fs::path &p);
    static const SDL_Rect &getScreenRect() { return screenRect; }
    static const SDL_Rect &getMapRect() { return mapRect; }
    static const SDL_Color &getUiForeground() { return uIForeground; }
    static const SDL_Color &getUiBackground() { return uIBackground; }
    static const SDL_Color &getUiHighlight() { return uIHighlight; }
    static const SDL_Color &getLoadBarColor() { return loadBarColor; }
    static const SDL_Color &getRouteColor() { return routeColor; }
    static const SDL_Color &getWaterColor() { return waterColor; }
    static const SDL_Color &getPlayerColor() { return playerColor; }
    static const SDL_Color &getAIColor() { return aIColor; }
    static int getScroll() { return scroll; }
    static int getOffsetX() { return offsetX; }
    static int getOffsetY() { return offsetY; }
    static double getScale() { return scale; }
    static int getBigBoxBorder() { return bigBoxBorder; }
    static int getBigBoxRadius() { return bigBoxRadius; }
    static int getBigBoxFontSize() { return bigBoxFontSize; }
    static int getLoadBarFontSize() { return loadBarFontSize; }
    static int getSmallBoxBorder() { return smallBoxBorder; }
    static int getSmallBoxRadius() { return smallBoxRadius; }
    static int getSmallBoxFontSize() { return smallBoxFontSize; }
    static int getFightFontSize() { return fightFontSize; }
    static int getEquipBorder() { return equipBorder; }
    static int getEquipRadius() { return equipRadius; }
    static int getEquipFontSize() { return equipFontSize; }
    static int getTownFontSize() { return townFontSize; }
    static int getTradeBorder() { return tradeBorder; }
    static int getTradeRadius() { return tradeRadius; }
    static int getTradeFontSize() { return tradeFontSize; }
    static int getGoodButtonSizeMultiplier() { return goodButtonSizeMultiplier; }
    static int getGoodButtonSpaceMultiplier() { return goodButtonSpaceMultiplier; }
    static int getGoodButtonXDivisor() { return goodButtonXDivisor; }
    static int getGoodButtonYDivisor() { return goodButtonYDivisor; }
    static int getBusinessButtonSizeMultiplier() { return businessButtonSizeMultiplier; }
    static int getBusinessButtonSpaceMultiplier() { return businessButtonSpaceMultiplier; }
    static int getBusinessButtonXDivisor() { return businessButtonXDivisor; }
    static int getBusinessButtonYDivisor() { return businessButtonYDivisor; }
    static int getDayLength() { return dayLength; }
    static unsigned int getBusinessHeadStart() { return businessHeadStart; }
    static int getBusinessRunTime() { return businessRunTime; }
    static int getTravelersCheckTime() { return travelersCheckTime; }
    static int getAIDecisionTime() { return aIDecisionTime; }
    static double getConsumptionSpaceFactor() { return consumptionSpaceFactor; }
    static double getInputSpaceFactor() { return inputSpaceFactor; }
    static double getOutputSpaceFactor() { return outputSpaceFactor; }
    static int getMinPriceDivisor() { return minPriceDivisor; }
    static double getTownProfit() { return townProfit; }
    static size_t getMaxTowns() { return maxTowns; }
    static size_t getMaxNeighbors() { return maxNeighbors; }
    static double getTravelersExponent() { return travelersExponent; }
    static int getTravelersMin() { return travelersMin; }
    static unsigned int getStatMax() { return statMax; }
    static double getAttackDistSq() { return attackDistSq; }
    static double getEscapeChance() { return escapeChance; }
    static const std::array<double, 6> &getAITypeWeights() { return aITypeWeights; }
    static int getCriteriaMax() { return criteriaMax; }
    static unsigned int getAITownRange() { return aITownRange; }
    static double getLimitFactorMin() { return limitFactorMin; }
    static double getLimitFactorMax() { return limitFactorMax; }
    static double getAIAttackThreshold() { return aIAttackThreshold; }
    static std::mt19937 &getRng() { return rng; }
    static void save(const fs::path &p);
    static BoxInfo getBoxInfo(bool iBg, const SDL_Rect &rt, const std::vector<std::string> &tx, SDL_Keycode ky,
                              std::function<void(MenuButton *)> fn, bool scl);
    static BoxInfo getBoxInfo(bool iBg, const SDL_Rect &rt, const std::vector<std::string> &tx, SDL_Keycode ky,
                              std::function<void(MenuButton *)> fn) {
        return getBoxInfo(iBg, rt, tx, ky, fn, false);
    }
    static BoxInfo getBoxInfo(bool iBg, const SDL_Rect &rt, const std::vector<std::string> &tx, bool cE);
    static BoxInfo getBoxInfo(bool iBg, const SDL_Rect &rt, const std::vector<std::string> &tx) {
        return getBoxInfo(iBg, rt, tx, false);
    }
};

#endif // SETTINGS_H

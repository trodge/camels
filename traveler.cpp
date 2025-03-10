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

#include "traveler.hpp"

Traveler::Traveler(const std::string &n, Town *tn, const GameData &gD)
    : name(n), nation(tn->getNation()), destination(tn), source(tn), home(nullptr), position(tn->getPosition()),
      moving(false), portion(1), reputation(gD.nationCount), gameData(gD) {
    // Copy goods vector from nation.
    properties.emplace(std::piecewise_construct, std::forward_as_tuple(0),
                       std::forward_as_tuple(false, &tn->getNation()->getProperty()));
    // Equip fists.
    equip(Part::leftArm);
    equip(Part::rightArm);
    // Randomize stats.
    stats = Settings::travelerStats();
    // Set parts status to normal.
    parts.fill(Status::normal);
}

Traveler::Traveler(const Save::Traveler *ldTvl, const std::vector<Nation> &nts, std::vector<Town> &tns, const GameData &gD)
    : name(ldTvl->name()->str()), nation(&nts[static_cast<size_t>(ldTvl->nation() - 1)]),
      destination(&tns[static_cast<size_t>(ldTvl->toTown() - 1)]),
      source(&tns[static_cast<size_t>(ldTvl->fromTown() - 1)]), home(nullptr),
      position(ldTvl->longitude(), ldTvl->latitude()), moving(ldTvl->moving()), portion(1), gameData(gD) {
    auto ldLog = ldTvl->log();
    std::transform(ldLog->begin(), ldLog->end(), std::back_inserter(logText),
                   [](auto ldL) { return ldL->str(); });
    // Copy good image pointers from nation's goods to traveler's goods.
    auto ntPpt = &nation->getProperty();
    auto ldProperties = ldTvl->properties();
    for (auto ldPI = ldProperties->begin(); ldPI != ldProperties->end(); ++ldPI)
        properties.emplace(std::piecewise_construct, std::forward_as_tuple((*ldPI)->townId()),
                           std::forward_as_tuple(*ldPI, ntPpt));
    auto ldStats = ldTvl->stats();
    std::transform(ldStats->begin(), ldStats->end(), begin(stats), [](auto ldSt) { return ldSt; });
    auto ldParts = ldTvl->parts();
    std::transform(ldParts->begin(), ldParts->end(), begin(parts),
                   [](auto ldPt) { return static_cast<Status>(ldPt); });
    auto ldEquipment = ldTvl->equipment();
    std::transform(ldEquipment->begin(), ldEquipment->end(), std::back_inserter(equipment),
                   [](auto ldEq) { return Good(ldEq); });
}

flatbuffers::Offset<Save::Traveler> Traveler::save(flatbuffers::FlatBufferBuilder &b) const {
    // Return a flatbuffers save object for this traveler.
    auto svName = b.CreateString(name);
    auto svLog = b.CreateVectorOfStrings(logText);
    std::vector<std::pair<unsigned int, Property>> vPpts(begin(properties), end(properties));
    auto svProperties = b.CreateVector<flatbuffers::Offset<Save::Property>>(
        properties.size(), [&b, &vPpts](size_t i) { return vPpts[i].second.save(b, vPpts[i].first); });
    auto svStats = b.CreateVector(std::vector<unsigned int>(begin(stats), end(stats)));
    std::vector<unsigned int> vParts(static_cast<size_t>(Part::count));
    std::transform(begin(parts), end(parts), begin(vParts),
                   [](auto pt) { return static_cast<unsigned int>(pt); });
    auto svParts = b.CreateVector(vParts);
    auto svEquipment = b.CreateVector<flatbuffers::Offset<Save::Good>>(
        equipment.size(), [this, &b](size_t i) { return equipment[i].save(b); });
    if (aI)
        return Save::CreateTraveler(b, svName, destination->getId(), source->getId(), nation->getId(), svLog,
                                    position.getLongitude(), position.getLatitude(), svProperties, svStats,
                                    svParts, svEquipment, aI->save(b), moving);
    else
        return Save::CreateTraveler(b, svName, destination->getId(), source->getId(), nation->getId(), svLog,
                                    position.getLongitude(), position.getLatitude(), svProperties, svStats,
                                    svParts, svEquipment, 0, moving);
}

std::forward_list<Town *> Traveler::pathTo(const Town *tn) const {
    // Return forward list of towns on shortest path to tn, excluding current town.
    std::forward_list<Town *> path;
    std::unordered_map<Town *, Town *> from;
    // the town which each town can be most efficiently reached from
    std::unordered_map<Town *, int> distSqTo({{destination, 0}});
    // distance along route to each town
    std::unordered_map<Town *, int> distSqEst({{destination, destination->distSq(tn)}});
    // estimated distance along route through each town to goal
    std::unordered_set<Town *> closed;
    // set of towns already evaluated
    auto shortest = [&distSqEst](Town *m, Town *n) {
        int c = distSqEst[m], d = distSqEst[n];
        if (c == d) return m->getId() < n->getId();
        return c < d;
    };
    std::set<Town *, decltype(shortest)> open(shortest);
    // set of discovered towns not yet evaluated sorted so that closest is first
    open.insert(destination);
    while (!open.empty()) {
        // Find current town with lowest distance.
        Town *current = *begin(open);
        const std::vector<Town *> &neighbors = current->getNeighbors();
        if (current == tn) {
            // Current town is target town.
            while (from.count(current)) {
                path.push_front(current);
                current = from[current];
            }
            return path;
        }
        open.erase(begin(open));
        closed.insert(current);
        for (auto &nb : neighbors) {
            // Ignore town if already explored.
            if (closed.count(nb)) continue;
            // Find distance to nb through current.
            int dSqT = distSqTo[current] + current->distSq(nb);
            // Check if distance through current is less than previous shortest path to nb.
            if (!distSqTo.count(nb) || dSqT < distSqTo[nb]) {
                from[nb] = current;
                distSqTo[nb] = dSqT;
                distSqEst[nb] = dSqT + nb->distSq(tn);
            }
            // Add undiscovered node to open set.
            if (!open.count(nb)) open.insert(nb);
        }
    }
    // A path does not exist, return empty list.
    return path;
}

int Traveler::pathDistSq(const Town *tn) const {
    // Return the distance to tn along shortest path.
    const std::forward_list<Town *> &path = pathTo(tn);
    int distSq = 0;
    const Town *m = destination;
    for (auto &n : path) {
        distSq += m->distSq(n);
        m = n;
    }
    return distSq;
}

void Traveler::addToTown() { destination->addTraveler(this); }

bool Traveler::fightWon() const {
    // Returns true if there are enemies and none of them are alive and want to fight.
    return !enemies.empty() && std::find_if(begin(enemies), end(enemies), [](const Traveler *enm) {
                                   return enm->alive() && enm->choice == FightChoice::fight;
                               }) == end(enemies);
}

void Traveler::setPortion(double d) {
    portion = d;
    if (portion < 0)
        portion = 0;
    else if (portion > 1)
        portion = 1;
}

void Traveler::changePortion(double d) { setPortion(portion + d); }

void Traveler::create(unsigned int fId, double amt) { properties.find(0)->second.create(fId, amt); }

void Traveler::pickTown(const Town *tn) {
    // Start moving toward given town.
    if (weight() > 0 || moving) return;
    const auto &path = pathTo(tn);
    if (path.empty() || path.front() == destination) return;
    destination = path.front();
    moving = true;
    forEmployee({AIRole::guard, AIRole::thug}, [this](Traveler *epl) {
        epl->destination = destination;
        epl->moving = true;
    });
}

void Traveler::draw(SDL_Renderer *s) const {
    SDL_Color col;
    if (aI)
        col = Settings::getAIColor();
    else
        col = Settings::getPlayerColor();
    drawCircle(s, position.getPoint(), 1, col, true);
}

void Traveler::updatePortionBox(TextBox *bx) const {
    // Give parameter box a string representing portion with trailing zeroes omitted.
    std::string portionString = std::to_string(portion);
    size_t lNZI = portionString.find_last_not_of('0');
    size_t dI = portionString.find('.');
    portionString.erase(lNZI == dI ? dI + 2 : lNZI + 1, std::string::npos);
    bx->setText(0, portionString);
}

void Traveler::clearTrade() {
    offer.clear();
    request.clear();
}

void Traveler::makeTrade() {
    if (offer.empty() || request.empty()) return;
    auto &ppt = properties.find(0)->second;
    std::string logEntry = name + " trades ";
    transfer(offer, ppt, *destination, logEntry);
    logEntry += " for ";
    transfer(request, *destination, ppt, logEntry);
    logEntry += " in " + destination->getName() + ".";
    logText.push_back(logEntry);
}

void Traveler::divideExcess(double exc, double tnP) {
    // Divide excess value among amounts of offered goods.
    exc /= static_cast<double>(offer.size());
    for (auto &of : offer) {
        // Convert value to quantity of this good.
        auto tG = destination->getProperty().good(of.getFullId()); // pointer to town good
        double q = tG->quantity(exc / tnP);
        if (!tG->getSplit()) q = floor(q);
        // Reduce quantity.
        of.use(q);
    }
}

Property &Traveler::makeProperty(unsigned int tId) {
    // Returns a reference to property in the given town id, or carried property if 0.
    auto pptIt = properties.find(tId);
    if (pptIt == end(properties))
        // Property has not been created yet.
        pptIt = properties
                    .emplace(std::piecewise_construct, std::forward_as_tuple(tId),
                             std::forward_as_tuple(destination->getProperty().getCoastal(),
                                                   &destination->getNation()->getProperty()))
                    .first;
    return pptIt->second;
}

void Traveler::deposit(Good &g) {
    // Put the given good in storage in the current town.
    properties.find(0)->second.take(g);
    makeProperty(destination->getId()).put(g);
}

void Traveler::withdraw(Good &g) {
    // Take the given good from storage in the current town.
    makeProperty(destination->getId()).take(g);
    properties.find(0)->second.put(g);
}

void Traveler::build(const Business &bsn, double a) {
    // Build the given area of the given business. Check requirements before calling.
    makeProperty(destination->getId()).build(bsn, a);
}

void Traveler::demolish(const Business &bsn, double a) {
    // Demolish the given area of the given business. Check area before calling.
    makeProperty(destination->getId()).demolish(bsn, a);
}

void Traveler::unequip(Part pt) {
    // Unequip all equipment using the given part id.
    auto unused = [pt](const Good &e) {
        auto &eSs = e.getCombatStats();
        return std::find_if(begin(eSs), end(eSs), [pt](const CombatStat &s) { return s.part == pt; }) == end(eSs);
    };
    auto uEI = std::partition(begin(equipment), end(equipment), unused);
    // Put equipment back in goods.
    for (auto eI = uEI; eI != end(equipment); ++eI)
        if (eI->getAmount() > 0) properties.find(0)->second.put(*eI);
    equipment.erase(uEI, end(equipment));
}

void Traveler::equip(Good &g) {
    // Equip the given good.
    auto &ss = g.getCombatStats();
    std::vector<Part> pts; // parts used by this equipment
    pts.reserve(ss.size());
    for (auto &s : ss)
        if (pts.empty() || pts.back() != s.part) pts.push_back(s.part);
    for (auto pt : pts) unequip(pt);
    // Take good out of goods.
    properties.find(0)->second.take(g);
    // Put good in equipment container.
    equipment.push_back(g);
}

void Traveler::equip(Part pt) {
    // Equip fists if nothing is equipped in part.
    // Look for equipment in part.
    for (auto &e : equipment)
        for (auto &s : e.getCombatStats())
            if (s.part == pt) return;
    if (pt == Part::leftArm) {
        // Add left fist to equipment.
        Good fist = Good(0, "left fist", 1,
                         {{Part::leftArm, Stat::strength, 1, 0, AttackType::bash, {{1, 1, 1}}},
                          {Part::leftArm, Stat::agility, 0, 1, AttackType::bash, {{1, 1, 1}}}},
                         nullptr);
        equipment.push_back(fist);
    } else if (pt == Part::rightArm) {
        // Add right fist to equipment.
        Good fist = Good(0, "right fist", 1,
                         {{Part::rightArm, Stat::strength, 1, 0, AttackType::bash, {{1, 1, 1}}},
                          {Part::rightArm, Stat::agility, 0, 1, AttackType::bash, {{1, 1, 1}}}},
                         nullptr);
        equipment.push_back(fist);
    }
}

void Traveler::bid(double add, double wge) {
    // Add bid to current town with given addend and wage.
    contract = std::unique_ptr<Contract>(
        new Contract{this, destination->getProperty().totalValue(properties.find(0)->second) + add, wge});
    destination->addBid(*contract);
}

void Traveler::hire(size_t idx) {
    // Hire the traveler with the given bid index.
    auto bid = destination->takeBid(idx);
    auto employee = bid.party;
    employees.insert({employee->aI->getRole(), employee});
    bid.party = this;
    employee->contract = std::make_unique<Contract>(std::move(bid));
    auto &ppt = properties.find(0)->second, &eplPpt = employee->properties.find(0)->second;
    std::string logEntry = name + " hires " + employee->name + " for ";
    transfer(offer, ppt, eplPpt, logEntry);
    logEntry += ".";
    logText.push_back(logEntry);
    employee->logText.push_back(logEntry);
}

void Traveler::dismiss(Traveler *epl) {
    // Calculate value of goods employee holds.
    double totalValue = 0;
    std::unordered_map<unsigned int, const Good *> townGoods;
    epl->property().forGood([epl, &totalValue, &townGoods](const Good &gd) {
        auto flId = gd.getFullId();
        auto tnGd = epl->home->getProperty().good(flId);
        if (tnGd) {
            totalValue += tnGd->price(gd.getAmount());
            townGoods[flId] = tnGd;
        }
    });
    // Request goods with value equal to difference between total value and owed.
    double requestValue = (totalValue - epl->contract->owed) / townGoods.size();
    request.clear();
    for (auto townGood : townGoods)
        request.push_back(Good(townGood.first, townGood.second->quota(requestValue)));
    auto &ppt = properties.find(0)->second, &eplPpt = epl->properties.find(0)->second;
    std::string logEntry = name + " dismisses " + epl->name + " and collects ";
    transfer(request, eplPpt, ppt, logEntry);
    logEntry += ".";
    logText.push_back(logEntry);
    epl->logText.push_back(logEntry);
}

std::vector<Traveler *> Traveler::attackable() const {
    // Get a vector of attackable travelers.
    auto &forwardTravelers = source->getTravelers();       // travelers traveling from the same town
    auto &backwardTravelers = destination->getTravelers(); // travelers traveling from the destination town
    std::vector<Traveler *> able;
    able.insert(end(able), begin(forwardTravelers), end(forwardTravelers));
    if (source != destination) able.insert(end(able), begin(backwardTravelers), end(backwardTravelers));
    if (!able.empty()) {
        // Eliminate travelers which have reached town, are too far away, or are this traveler.
        int aDstSq = Settings::getAttackDistSq();
        able.erase(std::remove_if(begin(able), end(able),
                                  [this, aDstSq](Traveler *tg) {
                                      return tg->destination == tg->source ||
                                             position.distSq(tg->position) > aDstSq || tg == this;
                                  }),
                   end(able));
    }
    return able;
}

void Traveler::forEmployee(AIRole rl, const std::function<void(Traveler *)> &fn) {
    // Insert employees with given roles to allies set.
    auto rng = employees.equal_range(rl);
    std::for_each(rng.first, rng.second, [&fn](std::pair<AIRole, Traveler *> tvl) { fn(tvl.second); });
}

void Traveler::forEmployee(const std::vector<AIRole> &rls, const std::function<void(Traveler *)> &fn) {
    // Insert employees with given roles to allies set.
    for (auto rl : rls) { forEmployee(rl, fn); }
}

void Traveler::forAlly(const std::function<void(Traveler *)> &fn) {
    // Call given function on this traveler and all its allies.
    fn(this);
    for (auto ally : allies) fn(ally);
}

void Traveler::forCombatant(const std::function<void(Traveler *)> &fn) {
    // Call given function on all combatants.
    forAlly(fn);
    for (auto enemy : enemies) enemy->forAlly(fn);
}

void Traveler::clearCombat() {
    forAlly([](Traveler *aly) {
        aly->targeterCount = 0;
        aly->target = nullptr;
        aly->nextHit = nullptr;
    });
}

void Traveler::attack(Traveler *tgt) {
    // Attack the given traveler.
    if (tgt->enemies.empty()) /* Target is not already fighting. */ {
        // Add target's thugs and guards to target's allies set.
        tgt->forEmployee({AIRole::guard, AIRole::thug}, [tgt](Traveler *epl) { tgt->allies.insert(epl); });
        // Reset fight time.
        tgt->fightTime = 0;
        // Reset targets and targeter counts.
        tgt->clearCombat();
    }
    // Add target to enemies set.
    enemies.insert(tgt);
    // Add this to target's enemy set.
    tgt->enemies.insert(this);
    // Add thugs to allies set.
    forEmployee(AIRole::thug, [this](Traveler *epl) { allies.insert(epl); });
    // Reset fight time.
    fightTime = tgt->fightTime;
    // Reset targets and targeter counts.
    clearCombat();
    // Set choices.
    if (aI)
        choice = FightChoice::fight;
    else
        choice = FightChoice::none;
    if (tgt->aI)
        tgt->choice = tgt->aI->choice();
    else
        tgt->choice = FightChoice::none;
}

void Traveler::disengage() {
    // Leave combat.
    for (auto enemy : enemies) {
        // Remove this from enemy's enemies.
        enemy->enemies.erase(this);
        // Clear enemy's target if it is this or an ally.
        if (enemy->target == this || allies.find(enemy->target) != end(allies)) enemy->target = nullptr;
        // Remove enemy from town so that it can't be attacked again.
        enemy->source->removeTraveler(target);
        // Set dead to true if enemy is not alive.
        enemy->dead = !enemy->alive();
    }
    enemies.clear();
    allies.clear();
}

CombatHit Traveler::firstHit() {
    // Find first hit of this traveler.
    EnumArray<unsigned int, AttackType> defense;
    for (auto &e : target->equipment)
        for (auto &s : e.getCombatStats())
            for (size_t i = 0; i < defense.size(); ++i)
                defense[static_cast<AttackType>(i)] += s.defense[static_cast<AttackType>(i)] * target->stats[s.stat];
    CombatHit first = {std::numeric_limits<double>::max(), nullptr, ""};
    for (auto &e : equipment) {
        auto &ss = e.getCombatStats();
        if (ss.front().attack) {
            // e is a weapon.
            unsigned int attack = 0, speed = 0;
            AttackType type = ss.front().type;
            for (auto &s : ss) {
                attack += s.attack * stats[s.stat];
                speed += s.speed * stats[s.stat];
            }
            auto &cO = gameData.odds[type];
            // Calculate time before hit happens.
            double r = Settings::random();
            double p = static_cast<double>(attack) / cO.hitChance / static_cast<double>(defense[type]);
            double time;
            if (p < 1)
                time = (log(r) / log(1 - p) + 1) / speed;
            else
                time = 1 / speed;
            if (time < first.time) {
                first.time = time;
                first.odd = &cO;
                first.weapon = e.getFullName();
            }
        }
    }
    return first;
}

void Traveler::setTarget(Traveler *tgt) {
    --target->targeterCount;
    target = tgt;
    ++tgt->targeterCount;
}

void Traveler::setTarget(const std::unordered_set<Traveler *> &enms) {
    if (aI && (!target || !target->alive())) {
        target = aI->target(enms);
        ++target->targeterCount;
    }
}

void Traveler::setNextHit() {
    if (!nextHit) nextHit = std::make_unique<CombatHit>(firstHit());
}

void Traveler::useAmmo(double tm) {
    for (auto &e : equipment) {
        unsigned int sId = e.getShoots();
        if (sId) {
            unsigned int speed = 0;
            for (auto &s : e.getCombatStats()) speed += s.speed * stats[s.stat];
            unsigned int n = static_cast<unsigned int>(tm * speed);
            properties.find(0)->second.input(sId, n);
        }
    }
}

void Traveler::fight(unsigned int elTm) {
    // Fight target for e milliseconds.
    fightTime -= elTm;
    // Prevent fight from happening multiple times.
    for (auto enemy : enemies) enemy->fightTime += static_cast<double>(elTm);
    // Keep fighting until all enemies die, run, or yield or time runs out.
    while (alive() && choice == FightChoice::fight && fightTime < 0 &&
           std::find_if(begin(enemies), end(enemies), [](const Traveler *enm) {
               return enm->alive() && enm->choice == FightChoice::fight;
           }) != end(enemies)) {
        // Set target and next hit for all combatants and find first hit.
        double soonest = std::numeric_limits<double>::max();
        Traveler *first = nullptr;
        // Set targets of our side.
        forAlly([this](Traveler *aly) { aly->setTarget(enemies); });
        for (auto enemy : enemies)
            // Set targets of enemy and its allies.
            enemy->forAlly([enemy](Traveler *aly) { aly->setTarget(enemy->enemies); });
        forCombatant([&soonest, &first](Traveler *cbt) {
            cbt->setNextHit();
            if (cbt->nextHit->time < soonest) {
                soonest = cbt->nextHit->time;
                first = cbt;
            }
        });
        // Update time and use ammo for all combatants.
        double hitTime = first->nextHit->time; // time elapsed before first hit
        forCombatant([hitTime](Traveler *cbt) {
            cbt->useAmmo(hitTime);
            cbt->fightTime += hitTime;
        });
        // Perform first hit.
        first->hit();
        // Set choices for all AI combatants.
        forCombatant([](Traveler *cbt) {
            if (cbt->aI) cbt->choice = cbt->aI->choice();
        });
    }
}

void Traveler::hit() {
    // Apply the next combat hit to target, update both logs, and set next hit to null.
    if (!nextHit) return;
    // Pick a random part.
    auto part = Settings::part();
    // Start status at that part's status.
    auto status = target->parts[part];
    // Get random value from 0 to 1.
    auto r = Settings::random();
    // Find status such that part becomes more damaged.
    if (!nextHit->odd) return;
    auto &stChcs = nextHit->odd->statusChances;
    for (auto sCI = begin(stChcs); r > 0 && sCI != end(stChcs); ++sCI)
        // Find status such that part becomes more damaged.
        if (sCI->first > status) {
            // This part is less damaged than the current status.
            status = sCI->first;
            // Subtract chance of this status from r.
            r -= sCI->second;
        }
    target->parts[part] = status;
    if (status > Status::wounded)
        // Part is too wounded to hold equipment.
        target->unequip(part);
    std::string logEntry = name + "'s " + nextHit->weapon + " strikes " + target->name + ". " + target->name +
                           "'s " + gameData.partNames[part] + " has been " + gameData.statusNames[status] +
                           ".";
    logText.push_back(logEntry);
    target->logText.push_back(logEntry);
    nextHit = nullptr;
}

std::vector<std::string> Traveler::statusText() {
    std::vector<std::string> text(static_cast<size_t>(Part::count) + 1);
    text[0] = name + "'s Status";
    for (size_t i = 0; i < static_cast<size_t>(Part::count); ++i)
        text[i + 1] =
            gameData.partNames[static_cast<Part>(i)] + ": " + gameData.statusNames[parts[static_cast<Part>(i)]];
    return text;
}

void Traveler::loot(Good &g) {
    // Take the given good from the given traveler.
    target->properties.find(0)->second.take(g);
    properties.find(0)->second.put(g);
}

void Traveler::loot() {
    target->properties.find(0)->second.forGood([this](const Good &g) {
        Good lG(g);
        loot(lG);
    });
}

void Traveler::createAIGoods(AIRole rl) {
    for (auto &sG : Settings::getAIStartingGoods(rl)) create(sG.first, sG.second);
}

void Traveler::startAI() {
    // Initialize variables for running a new AI.
    aI = std::make_unique<AI>(*this);
}

void Traveler::startAI(const Traveler &p) {
    // Starts an AI which imitates parameter's AI.
    aI = std::make_unique<AI>(*this, *p.aI);
}

void Traveler::update(unsigned int elTm) {
    // Move traveler toward destination, update properties, and perform combat with target.
    if (aI) aI->update(elTm);
    for (auto &ppt : properties) ppt.second.update(elTm);
    if (moving) {
        moving = position.stepToward(destination->getPosition(),
                                     static_cast<double>(elTm) / static_cast<double>(Settings::getDayLength()));
        if (!moving) {
            // We reached the target town.
            source->removeTraveler(this);
            source = destination;
            destination->addTraveler(this);
            logText.push_back(
                name + " has arrived in the " +
                gameData.populationAdjectives.lower_bound(destination->getProperty().getPopulation())->second +
                " " + destination->getNation()->getAdjective() + " " +
                gameData.townTypeNames[destination->getProperty().getTownType()] + " of " +
                destination->getName() + ".");
        }
    }
    for (auto enemy : enemies) {
        std::string logEntry;
        switch (enemy->choice) {
        case FightChoice::fight:
            fight(elTm);
            break;
        case FightChoice::run:
            if (choice == FightChoice::fight) {
                // Check if target escapes.
                double escapeChance = Settings::getEscapeChance() * enemy->speed() / speed();
                if (Settings::random() > escapeChance) {
                    // Enemy is caught, fight.
                    enemy->choice = FightChoice::fight;
                    fight(elTm);
                    logEntry = name + " catches up to " + enemy->name + ".";
                    logText.push_back(logEntry);
                    enemy->logText.push_back(logEntry);
                } else {
                    // Enemy escapes.
                    logEntry = enemy->name + " has eluded " + name + ".";
                    logText.push_back(logEntry);
                    enemy->logText.push_back(logEntry);
                    enemy->disengage();
                }
            } else {
                logEntry = name + " and " + enemy->name + " have eluded each other.";
                logText.push_back(logEntry);
                enemy->logText.push_back(logEntry);
                enemy->disengage();
            }
            break;
        case FightChoice::yield:
            logEntry = enemy->name + " has yielded to " + name + ".";
            logText.push_back(logEntry);
            enemy->logText.push_back(logEntry);
            break;
        default:
            break;
        }
    }
    if (fightWon() && aI) {
        aI->loot();
        disengage();
        return;
    }
}

void Traveler::toggleMaxGoods() { destination->toggleMaxGoods(); }

void Traveler::resetTown() { destination->reset(); }

void Traveler::adjustAreas(const std::vector<TextBox *> &bxs, double mM) {
    std::vector<MenuButton *> requestButtons;
    std::transform(begin(bxs), end(bxs), std::back_inserter(requestButtons),
                   [](auto rB) { return dynamic_cast<MenuButton *>(rB); });
    destination->adjustAreas(requestButtons, mM);
}

void Traveler::adjustDemand(const std::vector<TextBox *> &bxs, double mM) {
    // Adjust demand for goods in current town and show new prices on buttons.
    std::vector<MenuButton *> requestButtons;
    std::transform(begin(bxs), end(bxs), std::back_inserter(requestButtons),
                   [](auto rB) { return dynamic_cast<MenuButton *>(rB); });
    destination->adjustDemand(requestButtons, mM);
}

template <class Source, class Destination>
void transfer(std::vector<Good> &gds, Source &src, Destination &dst, std::string &lgEt) {
    // Transfer goods from source to destination and append to log entry.
    for (auto gI = begin(gds); gI != end(gds); ++gI) {
        if (gI != begin(gds)) {
            // This is not the first good.
            if (gds.size() != 2) /* There are not exactly two goods. */
                lgEt += ", ";
            if (gI + 1 == end(gds)) /* This is the last good. */
                lgEt += " and ";
        }
        src.take(*gI);
        dst.put(*gI);
        lgEt += gI->logEntry();
    }
}

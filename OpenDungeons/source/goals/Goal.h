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

#ifndef GOAL_H
#define GOAL_H

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

class Seat;
class GameMap;

class Goal
{
public:
    // Constructors
    Goal(const std::string& nName, const std::string& nArguments);
    virtual ~Goal() {}

    // Functions which must be overridden by child classes
    virtual bool isMet(const Seat&, const GameMap&) = 0;
    virtual std::string getDescription(const Seat&) = 0;
    virtual std::string getSuccessMessage(const Seat&) = 0;
    virtual std::string getFailedMessage(const Seat&) = 0;

    // Functions which can be overridden (but do not have to be) by child classes
    virtual void doSuccessAction();
    virtual bool isVisible();
    virtual bool isUnmet(const Seat& s, const GameMap& gameMap);
    virtual bool isFailed(const Seat&, const GameMap&);

    // Functions which cannot be overridden by child classes
    const std::string& getName() const
    { return mName; }

    void addSuccessSubGoal(std::unique_ptr<Goal>&& g);
    unsigned numSuccessSubGoals() const;
    Goal* getSuccessSubGoal(int index);

    void addFailureSubGoal(std::unique_ptr<Goal>&& g);
    unsigned numFailureSubGoals() const;
    Goal* getFailureSubGoal(int index);

    static std::string getFormat();
    friend std::ostream& operator<<(std::ostream& os, Goal& g);

protected:
    std::string mName;
    std::string mArguments;
    std::vector<std::unique_ptr<Goal>> mSuccessSubGoals;
    std::vector<std::unique_ptr<Goal>> mFailureSubGoals;
};

#endif // GOAL_H

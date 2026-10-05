#pragma once

#include "types.h"
#include <vector>
#include <string>

class Environment {
private:
	int size;
	std::vector<std::vector<bool>> walls;
	Position bombPosition;
	Position exitPosition;
	Position agentPosition;

	bool bombPicked = false;
	bool bombExploded = false;

	int stunRemaining = 0;
	int totalCost = 0;

public:
	Environment();

	void LoadFromFile(const std::string& filename);
	void Draw(bool showMetrics = true) const;

	Percept GetPercept() const;
	Action ExecuteAction(const Action& action);

	bool IsFinished() const;
	bool MissionSucceeded() const;
	bool MissionFailed() const;

	int GetSize() const;
	int GetTotalCost() const;
	int GetStunRemaining() const;

	Position GetAgentPosition() const;
	Position GetExitPosition() const;
	bool IsBombPicked() const;

	bool IsInside(int x, int y) const;
	bool HasWall(int x, int y) const;
	bool BombSignalAt(Position p) const;
};

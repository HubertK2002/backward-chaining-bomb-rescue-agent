#include "environment.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <iomanip>

using namespace std;

Environment::Environment() : size(0) {}

bool Environment::IsInside(int x, int y) const {
	return x >= 0 && y >= 0 && x < size && y < size;
}

bool Environment::HasWall(int x, int y) const {
	if (!IsInside(x, y)) {
		return true;
	}
	return walls[y][x];
}

int Environment::GetSize() const {
	return size;
}

int Environment::GetTotalCost() const {
	return totalCost;
}

int Environment::GetStunRemaining() const {
	return stunRemaining;
}

Position Environment::GetAgentPosition() const {
	return agentPosition;
}

Position Environment::GetExitPosition() const {
	return exitPosition;
}

bool Environment::IsBombPicked() const {
	return bombPicked;
}

bool Environment::BombSignalAt(Position p) const {
	// Uproszczenie: strefa zagrozenia to kwadrat o zasiegu 2 od bomby.
	// Agent czuje bombe na wszystkich polach z |dx| <= 2 i |dy| <= 2
	// (poza polem samej bomby).
	int dx = abs(bombPosition.x - p.x);
	int dy = abs(bombPosition.y - p.y);
	return (dx != 0 || dy != 0) && dx <= 2 && dy <= 2;
}
void Environment::LoadFromFile(const string& filename) {
	ifstream file(filename);
	if (!file.is_open()) {
		throw runtime_error("Cannot open map file: " + filename);
	}

	vector<string> lines;
	string line;
	while (getline(file, line)) {
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}
		lines.push_back(line);
	}

	if (lines.empty()) {
		throw runtime_error("Empty map file");
	}

	size = (int)lines.size();
	for (const auto& row : lines) {
		if ((int)row.size() != size) {
			throw runtime_error("Map must be NxN");
		}
	}

	walls.assign(size, vector<bool>(size, false));

	bool hasAgent = false;
	bool hasBomb = false;
	bool hasExit = false;

	for (int y = 0; y < size; y++) {
		for (int x = 0; x < size; x++) {
			char c = lines[y][x];
			if (c == 'X') {
				walls[y][x] = true;
			}
			else if (c == 'A') {
				agentPosition = { x, y };
				hasAgent = true;
			}
			else if (c == 'B') {
				bombPosition = { x, y };
				hasBomb = true;
			}
			else if (c == 'E') {
				exitPosition = { x, y };
				hasExit = true;
			}
			else if (c != '.') {
				throw runtime_error("Invalid map character");
			}
		}
	}

	if (!hasAgent || !hasBomb || !hasExit) {
		throw runtime_error("Map must contain A, B, E");
	}
}

Percept Environment::GetPercept() const {
	Percept p;
	p.wallUp = HasWall(agentPosition.x, agentPosition.y - 1);
	p.wallDown = HasWall(agentPosition.x, agentPosition.y + 1);
	p.wallLeft = HasWall(agentPosition.x - 1, agentPosition.y);
	p.wallRight = HasWall(agentPosition.x + 1, agentPosition.y);

	p.bombSensed = (!bombPicked && BombSignalAt(agentPosition));
	p.onBomb = (!bombPicked && agentPosition == bombPosition);
	p.onExit = (agentPosition == exitPosition);

	return p;
}

Action Environment::ExecuteAction(const Action& action) {
	Action result = action;

	if (stunRemaining > 0) {
		stunRemaining--;
		totalCost += 1;
		return { ActionType::Wait, "Agent is stunned and waits", 1 };
	}

	auto tryMove = [&](int nx, int ny, const string& desc) -> Action {
		if (!IsInside(nx, ny) || HasWall(nx, ny)) {
			totalCost += 5;

			// jeśli agent niesie bombę i uderza w ścianę => eksplozja
			if (bombPicked) {
				bombExploded = true;
				return { action.type, desc + " -> collision with wall, bomb exploded", 5 };
			}

			// bez bomby: ściana się rozwala, agent zostaje ogłuszony
			if (IsInside(nx, ny)) {
				walls[ny][nx] = false;
			}
			stunRemaining = 2;
			return { action.type, desc + " -> wall broken, agent stunned", 5 };
		}

		agentPosition = { nx, ny };
		totalCost += 1;
		return { action.type, desc, 1 };
	};

	switch (action.type) {
		case ActionType::MoveUp:
			return tryMove(agentPosition.x, agentPosition.y - 1, "MoveUp");
		case ActionType::MoveDown:
			return tryMove(agentPosition.x, agentPosition.y + 1, "MoveDown");
		case ActionType::MoveLeft:
			return tryMove(agentPosition.x - 1, agentPosition.y, "MoveLeft");
		case ActionType::MoveRight:
			return tryMove(agentPosition.x + 1, agentPosition.y, "MoveRight");
		case ActionType::PickBomb:
			if (!bombPicked && agentPosition == bombPosition) {
				bombPicked = true;
				totalCost += 2;
				return { ActionType::PickBomb, "Picked bomb", 2 };
			}
			totalCost += 1;
			return { ActionType::PickBomb, "PickBomb failed", 1 };
		case ActionType::DropBomb:
			totalCost += 1;
			return { ActionType::DropBomb, "DropBomb", 1 };
		case ActionType::Wait:
			totalCost += 1;
			return { ActionType::Wait, "Wait", 1 };
		default:
			return { ActionType::None, "None", 0 };
	}
}

bool Environment::MissionSucceeded() const {
	return bombPicked && agentPosition == exitPosition && !bombExploded;
}

bool Environment::MissionFailed() const {
	return bombExploded;
}

bool Environment::IsFinished() const {
	return MissionSucceeded() || MissionFailed();
}

void Environment::Draw(bool showMetrics) const {
	cout << "\nMap:\n";
	for (int y = 0; y < size; y++) {
		for (int x = 0; x < size; x++) {
			if (agentPosition.x == x && agentPosition.y == y) {
				cout << "A ";
			}
			else if (walls[y][x]) {
				cout << "X ";
			}
			else if (!bombPicked && bombPosition.x == x && bombPosition.y == y) {
				cout << "B ";
			}
			else if (exitPosition.x == x && exitPosition.y == y) {
				cout << "E ";
			}
			else {
				cout << ". ";
			}
		}
		cout << "\n";
	}
	if (showMetrics) {
		cout << "Total cost: " << totalCost << "\n";
		cout << "Stun remaining: " << stunRemaining << "\n";
	}
}


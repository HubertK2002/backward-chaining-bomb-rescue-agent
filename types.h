#pragma once

#include <string>
#include <vector>

struct Position {
	int x;
	int y;

	bool operator==(const Position& other) const {
		return x == other.x && y == other.y;
	}
};

enum class ActionType {
	None,
	MoveUp,
	MoveDown,
	MoveLeft,
	MoveRight,
	PickBomb,
	DropBomb,
	Wait
};

struct Action {
	ActionType type = ActionType::None;
	std::string description = "None";
	int cost = 0;
};

struct Percept {
	bool wallUp = false;
	bool wallDown = false;
	bool wallLeft = false;
	bool wallRight = false;

	bool bombSensed = false;
	bool onBomb = false;
	bool onExit = false;
};

struct HistoryEntry {
	int situation = -1;
	std::string kind;               // OBS / INF / ACT / GOAL
	std::vector<std::string> premises;
	std::string conclusion;
};

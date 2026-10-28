#include "agent.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <fstream>

using namespace std;

Agent::Agent() : reasoner(kb) {}

void Agent::Initialize() {
	reasoner.LoadDefaultRules();
}

void Agent::UpdateKnowledgeFromPercept(const Environment& env, const Percept& percept) {
	Position p = env.GetAgentPosition();

	bool dangerContext = (!env.IsBombPicked()) && (dangerExplorationActive || percept.bombSensed || percept.onBomb);
	if (dangerContext) {
		kb.AddFact(KnowledgeBase::PosToFact("VisitedDanger", p.x, p.y));
		dangerVisitedCells.insert(PosKey(p));
	}
	else if (returningToDangerEntry) {
		// Powrot do punktu wejscia strefy to przejazd techniczny.
		// Nie oznaczamy tych pol jako VisitedGlobal, aby po powrocie
		// agent mogl je normalnie rozwazac przy szukaniu wyjscia.
		kb.AddFact(KnowledgeBase::PosToFact("VisitedReturn", p.x, p.y));
	}
	else {
		kb.AddFact(KnowledgeBase::PosToFact("VisitedGlobal", p.x, p.y));
		globalVisitedCells.insert(PosKey(p));
	}
	kb.AddFact("At(" + to_string(p.x) + "," + to_string(p.y) + ")");

	if (percept.wallUp) kb.AddObservation("WallUp");
	if (percept.wallDown) kb.AddObservation("WallDown");
	if (percept.wallLeft) kb.AddObservation("WallLeft");
	if (percept.wallRight) kb.AddObservation("WallRight");

	if (percept.bombSensed) kb.AddObservation("BombSensed");
	if (percept.onBomb) kb.AddObservation("OnBomb");
	if (percept.onExit) {
		kb.AddObservation("AtExit");
		hasSeenExit = true;
		seenExit = p;
		knownExitPath = mainPath;
		if (knownExitPath.empty() || !(knownExitPath.back() == p)) {
			knownExitPath.push_back(p);
		}
		hasKnownExitPath = true;
	}

	if (env.IsBombPicked()) {
		kb.AddFact("HasBomb");
	}
}

int Agent::DistanceToNearestBombCandidate(int x, int y) const {
	if (bombCandidates.empty()) return 1000000;

	int best = 1000000;
	for (const auto& c : bombCandidates) {
		int d = abs(c.x - x) + abs(c.y - y);
		if (d < best) best = d;
	}
	return best;
}

string Agent::PosKey(const Position& p) const {
	return to_string(p.x) + "," + to_string(p.y);
}

void Agent::SyncMainPathWithCurrentPosition(const Position& current) {
	if (mainPath.empty()) {
		mainPath.push_back(current);
		return;
	}

	if (mainPath.back() == current) return;

	if (mainPath.size() >= 2 && mainPath[mainPath.size() - 2] == current) {
		mainPath.pop_back();
		return;
	}

	mainPath.push_back(current);
}

Action Agent::MoveTowardsPreviousPathNode(const Position& current, const Position& prev, const string& label) const {
	int dx = prev.x - current.x;
	int dy = prev.y - current.y;
	if (dx == 1 && dy == 0) return { ActionType::MoveRight, label, 1 };
	if (dx == -1 && dy == 0) return { ActionType::MoveLeft, label, 1 };
	if (dx == 0 && dy == 1) return { ActionType::MoveDown, label, 1 };
	if (dx == 0 && dy == -1) return { ActionType::MoveUp, label, 1 };
	return { ActionType::Wait, label + "(invalid)", 1 };
}

void Agent::UpdateBombCandidates(const Environment& env, const Percept& percept) {
	Position a = env.GetAgentPosition();

	if (env.IsBombPicked()) {
		bombCandidates.clear();
		return;
	}

	if (percept.onBomb) {
		bombCandidates.clear();
		bombCandidates.push_back(a);
		return;
	}

	if (percept.bombSensed) {
		for (int dy = -2; dy <= 2; dy++) {
			for (int dx = -2; dx <= 2; dx++) {
				if (dx == 0 && dy == 0) continue;
				int nx = a.x + dx;
				int ny = a.y + dy;
				if (!env.IsInside(nx, ny) || env.HasWall(nx, ny)) continue;

				Position c{ nx, ny };
				auto it = find_if(bombCandidates.begin(), bombCandidates.end(), [&](const Position& p) {
					return p.x == c.x && p.y == c.y;
				});
				if (it == bombCandidates.end()) bombCandidates.push_back(c);
			}
		}
	}

	bombCandidates.erase(
		remove_if(bombCandidates.begin(), bombCandidates.end(), [&](const Position& p) {
			return p.x == a.x && p.y == a.y;
		}),
		bombCandidates.end()
	);

	if (!percept.bombSensed) {
		bombCandidates.erase(
			remove_if(bombCandidates.begin(), bombCandidates.end(), [&](const Position& c) {
				int dx = abs(c.x - a.x);
				int dy = abs(c.y - a.y);
				return dx <= 2 && dy <= 2 && (dx != 0 || dy != 0);
			}),
			bombCandidates.end()
		);
	}
}

void Agent::BuildTemporaryNavigationFacts(const Environment& env) {
	Position a = env.GetAgentPosition();
	Position e = env.GetExitPosition();

	auto isSafe = [&](int x, int y) {
		return env.IsInside(x, y) && !env.HasWall(x, y);
	};

	auto isUnvisited = [&](int x, int y) {
		return !kb.HasFact(KnowledgeBase::PosToFact("Visited", x, y));
	};

	if (isSafe(a.x + 1, a.y) && abs((a.x + 1) - e.x) + abs(a.y - e.y) < abs(a.x - e.x) + abs(a.y - e.y)) kb.AddFact("ExitRightSafe");
	if (isSafe(a.x - 1, a.y) && abs((a.x - 1) - e.x) + abs(a.y - e.y) < abs(a.x - e.x) + abs(a.y - e.y)) kb.AddFact("ExitLeftSafe");
	if (isSafe(a.x, a.y + 1) && abs(a.x - e.x) + abs((a.y + 1) - e.y) < abs(a.x - e.x) + abs(a.y - e.y)) kb.AddFact("ExitDownSafe");
	if (isSafe(a.x, a.y - 1) && abs(a.x - e.x) + abs((a.y - 1) - e.y) < abs(a.x - e.x) + abs(a.y - e.y)) kb.AddFact("ExitUpSafe");

	if (isSafe(a.x + 1, a.y)) {
		kb.AddFact("ExploreRightSafe");
		if (isUnvisited(a.x + 1, a.y)) kb.AddFact("ExploreRightUnvisited");
	}
	if (isSafe(a.x - 1, a.y)) {
		kb.AddFact("ExploreLeftSafe");
		if (isUnvisited(a.x - 1, a.y)) kb.AddFact("ExploreLeftUnvisited");
	}
	if (isSafe(a.x, a.y + 1)) {
		kb.AddFact("ExploreDownSafe");
		if (isUnvisited(a.x, a.y + 1)) kb.AddFact("ExploreDownUnvisited");
	}
	if (isSafe(a.x, a.y - 1)) {
		kb.AddFact("ExploreUpSafe");
		if (isUnvisited(a.x, a.y - 1)) kb.AddFact("ExploreUpUnvisited");
	}

	if (kb.HasFact("BombSensed")) {
		if (isSafe(a.x + 1, a.y)) {
			kb.AddFact("BombRightSafe");
			if (isUnvisited(a.x + 1, a.y)) kb.AddFact("BombRightUnvisited");
		}
		if (isSafe(a.x - 1, a.y)) {
			kb.AddFact("BombLeftSafe");
			if (isUnvisited(a.x - 1, a.y)) kb.AddFact("BombLeftUnvisited");
		}
		if (isSafe(a.x, a.y + 1)) {
			kb.AddFact("BombDownSafe");
			if (isUnvisited(a.x, a.y + 1)) kb.AddFact("BombDownUnvisited");
		}
		if (isSafe(a.x, a.y - 1)) {
			kb.AddFact("BombUpSafe");
			if (isUnvisited(a.x, a.y - 1)) kb.AddFact("BombUpUnvisited");
		}
	}
}

void Agent::SyncDangerPathWithCurrentPosition(const Position& current) {
	if (dangerPath.empty()) {
		dangerPath.push_back(current);
		return;
	}

	if (dangerPath.back() == current) return;

	if (dangerPath.size() >= 2) {
		Position parent = dangerPath[dangerPath.size() - 2];
		if (parent == current) {
			dangerPath.pop_back();
			return;
		}
	}

	dangerPath.push_back(current);
}

Action Agent::DecideDangerExplorationAction(const Environment& env, const Position& current, const Percept& percept) {
	SyncDangerPathWithCurrentPosition(current);
	Position node = dangerPath.back();
	dangerVisitedCells.insert(PosKey(node));

	auto isSafe = [&](int x, int y) {
		return env.IsInside(x, y) && !env.HasWall(x, y);
	};

	struct DirChoice {
		int dx;
		int dy;
		ActionType action;
		const char* canMove;
		const char* moveName;
	};

	static const DirChoice order[4] = {
		{ 0, -1, ActionType::MoveUp, "CanMoveUpInDanger", "MoveUpInDanger" },
		{ 1,  0, ActionType::MoveRight, "CanMoveRightInDanger", "MoveRightInDanger" },
		{ 0,  1, ActionType::MoveDown, "CanMoveDownInDanger", "MoveDownInDanger" },
		{ -1, 0, ActionType::MoveLeft, "CanMoveLeftInDanger", "MoveLeftInDanger" }
	};

	string key = PosKey(node);
	int& nextDir = dangerNextDirIndex[key];

	if (!percept.bombSensed && !percept.onBomb) {
		kb.AddInference({ "!BombSensed", "!OnBomb" }, "OutsideDangerZone");
		if (dangerPath.size() > 1) {
			Position prev = dangerPath[dangerPath.size() - 2];
			dangerPath.pop_back();
			kb.AddInference(
				{ "!IsInDangerZone", "FoundDangerZone" },
				"ReturnToDangerZoneNode"
			);
			return MoveTowardsPreviousPathNode(
				node,
				prev,
				"!IsInDangerZone /\\ FoundDangerZone => BacktrackToDangerZone"
			);
		}

		dangerExplorationActive = false;
		dangerPath.clear();
		dangerNextDirIndex.clear();
		dangerVisitedCells.clear();
		return { ActionType::Wait, "DangerSearchFinished", 1 };
	}

	vector<string> negatives;
	while (nextDir < 4) {
		const DirChoice& d = order[nextDir++];
		int nx = node.x + d.dx;
		int ny = node.y + d.dy;
		bool safe = isSafe(nx, ny);
		bool isBackEdge = false;
		bool visitedDanger = false;

		if (dangerPath.size() >= 2) {
			Position parent = dangerPath[dangerPath.size() - 2];
			isBackEdge = (parent.x == nx && parent.y == ny);
		}
		if (safe) {
			visitedDanger = (dangerVisitedCells.count(PosKey({ nx, ny })) > 0);
		}

		kb.AddQueryTrace(
			string(d.canMove) + "?",
			{ "Wall?", "DangerVisited(" + to_string(nx) + "," + to_string(ny) + ")?", "BackEdge?" },
			{
				string("Wall=") + (safe ? "false" : "true"),
				string("DangerVisited=") + (visitedDanger ? "true" : "false"),
				string("BackEdge=") + (isBackEdge ? "true" : "false")
			}
		);

		if (!safe) {
			kb.AddInference({ "Wall" }, string("!") + d.canMove);
			negatives.push_back(string("!") + d.canMove);
			continue;
		}
		if (isBackEdge) {
			kb.AddInference({ "BackEdge" }, string("!") + d.canMove);
			negatives.push_back(string("!") + d.canMove);
			continue;
		}
		if (visitedDanger) {
			kb.AddInference({ "DangerVisited(" + to_string(nx) + "," + to_string(ny) + ")" }, string("!") + d.canMove);
			negatives.push_back(string("!") + d.canMove);
			continue;
		}

		kb.AddInference(
			{ "!Wall", "!" + string("DangerVisited(" + to_string(nx) + "," + to_string(ny) + ")"), "!BackEdge" },
			string(d.canMove)
		);

		dangerPath.push_back({ nx, ny });
		vector<string> premises = negatives;
		premises.push_back(d.canMove);
		kb.AddInference(premises, string("Choose(") + d.moveName + ")");

		ostringstream desc;
		if (!negatives.empty()) {
			for (int i = 0; i < (int)negatives.size(); i++) {
				if (i > 0) desc << " /\\ ";
				desc << negatives[i];
			}
			desc << " /\\ ";
		}
		desc << d.canMove << " => " << d.moveName;
		return { d.action, desc.str(), 1 };
	}

	kb.AddInference(
		{ "!CanMoveUpInDanger", "!CanMoveRightInDanger", "!CanMoveDownInDanger", "!CanMoveLeftInDanger" },
		"NeedDangerBacktrack"
	);

	if (dangerPath.size() > 1) {
		Position prev = dangerPath[dangerPath.size() - 2];
		dangerPath.pop_back();
		return MoveTowardsPreviousPathNode(
			node,
			prev,
			"!CanMoveUpInDanger /\\ !CanMoveRightInDanger /\\ !CanMoveDownInDanger /\\ !CanMoveLeftInDanger => DangerBacktrack"
		);
	}

	dangerExplorationActive = false;
	dangerPath.clear();
	dangerNextDirIndex.clear();
	dangerVisitedCells.clear();
	return { ActionType::Wait, "DangerSearchFinished", 1 };
}

Action Agent::DecideLocalExploreAction(const Environment& env, const Position& current) {
	auto isSafe = [&](int x, int y) {
		return env.IsInside(x, y) && !env.HasWall(x, y);
	};
	auto isUnvisited = [&](int x, int y) {
		return globalVisitedCells.count(PosKey({ x, y })) == 0;
	};

	struct Candidate {
		int dx;
		int dy;
		ActionType action;
		const char* moveName;
		const char* canGoName;
		const char* wallName;
	};
	static const Candidate order[4] = {
		{0, -1, ActionType::MoveUp, "MoveUp", "CanGoUp", "WallUp"},
		{1, 0, ActionType::MoveRight, "MoveRight", "CanGoRight", "WallRight"},
		{0, 1, ActionType::MoveDown, "MoveDown", "CanGoDown", "WallDown"},
		{-1, 0, ActionType::MoveLeft, "MoveLeft", "CanGoLeft", "WallLeft"}
	};

	auto evaluateDirection = [&](const Candidate& c) {
		int nx = current.x + c.dx;
		int ny = current.y + c.dy;
		bool safe = isSafe(nx, ny);
		bool wall = !safe;
		bool visited = (!wall && !isUnvisited(nx, ny));
		bool unvisited = !wall && !visited;

		kb.AddQueryTrace(
			string(c.canGoName) + "?",
			{ string(c.wallName) + "?", "VisitedGlobal(" + to_string(nx) + "," + to_string(ny) + ")?" },
			{ string(c.wallName) + "=" + (wall ? "true" : "false"), string("VisitedGlobal=") + (visited ? "true" : "false") }
		);

		if (wall) {
			kb.AddInference({ c.wallName }, string("!") + c.canGoName);
		}
		else if (visited) {
			kb.AddInference({ "VisitedGlobal(" + to_string(nx) + "," + to_string(ny) + ")" }, string("!") + c.canGoName);
		}
		else {
			kb.AddInference(
				{ string("!") + c.wallName, "!" + string("VisitedGlobal(" + to_string(nx) + "," + to_string(ny) + ")") },
				string(c.canGoName)
			);
		}
		return safe && unvisited;
	};

	kb.AddQueryTrace(
		"NeedMovementDecision?",
		{ "CurrentGoal?" },
		{ "CurrentGoal=" + currentGoal }
	);

	bool existsUnvisitedNeighbor = false;
	for (const auto& c : order) {
		int nx = current.x + c.dx;
		int ny = current.y + c.dy;
		if (isSafe(nx, ny) && isUnvisited(nx, ny)) {
			existsUnvisitedNeighbor = true;
			break;
		}
	}

	kb.AddQueryTrace(
		"CanVisitNewPosition?",
		{ "ExistsUnvisitedNeighborAround?" },
		{ string("ExistsUnvisitedNeighborAround=") + (existsUnvisitedNeighbor ? "true" : "false") }
	);

	if (!existsUnvisitedNeighbor) {
		kb.AddInference({ "!ExistsUnvisitedNeighborAround" }, "HaveToGoBack");
		if (mainPath.size() > 1) {
			Position prev = mainPath[mainPath.size() - 2];
			kb.AddQueryTrace(
				"HaveToGoBack?",
				{ "CanVisitNewPosition?", "HasPreviousSituation?" },
				{ "false", "true" }
			);
			kb.AddInference({ "HaveToGoBack", "HasPreviousSituation" }, "ChooseBacktrack");
			return MoveTowardsPreviousPathNode(
				current,
				prev,
				"HaveToGoBack /\\ HasPreviousSituation => Backtrack"
			);
		}

		kb.AddQueryTrace(
			"HaveToGoBack?",
			{ "CanVisitNewPosition?", "HasPreviousSituation?" },
			{ "false", "false" }
		);
		kb.AddInference({ "HaveToGoBack", "!HasPreviousSituation" }, "ExploreFinished");
		return { ActionType::Wait, "HaveToGoBack /\\ !HasPreviousSituation => ExploreFinished", 1 };
	}

	kb.AddInference({ "ExistsUnvisitedNeighborAround" }, "CanVisitNewPosition");
	kb.AddQueryTrace(
		"SelectDirectionForVisit?",
		{ "CanVisitNewPosition?" },
		{ "true" }
	);

	bool canGoUp = evaluateDirection(order[0]);
	if (canGoUp) {
		kb.AddInference({ "CanVisitNewPosition", "CanGoUp" }, "ChooseMoveUp");
		return { order[0].action, "CanVisitNewPosition /\\ CanGoUp => MoveUp", 1 };
	}

	bool canGoRight = evaluateDirection(order[1]);
	if (!canGoUp && !canGoRight) {
		kb.AddInference({ "CanVisitNewPosition", "!CanGoUp", "!CanGoRight" }, "NeedCheckDown");
	}
	if (!canGoUp && canGoRight) {
		kb.AddInference({ "CanVisitNewPosition", "!CanGoUp", "CanGoRight" }, "ChooseMoveRight");
		return { order[1].action, "CanVisitNewPosition /\\ !CanGoUp /\\ CanGoRight => MoveRight", 1 };
	}

	bool canGoDown = evaluateDirection(order[2]);
	if (!canGoUp && !canGoRight && !canGoDown) {
		kb.AddInference({ "CanVisitNewPosition", "!CanGoUp", "!CanGoRight", "!CanGoDown" }, "NeedCheckLeft");
	}
	if (!canGoUp && !canGoRight && canGoDown) {
		kb.AddInference({ "CanVisitNewPosition", "!CanGoUp", "!CanGoRight", "CanGoDown" }, "ChooseMoveDown");
		return { order[2].action, "CanVisitNewPosition /\\ !CanGoUp /\\ !CanGoRight /\\ CanGoDown => MoveDown", 1 };
	}

	bool canGoLeft = evaluateDirection(order[3]);
	if (!canGoUp && !canGoRight && !canGoDown && canGoLeft) {
		kb.AddInference({ "CanVisitNewPosition", "!CanGoUp", "!CanGoRight", "!CanGoDown", "CanGoLeft" }, "ChooseMoveLeft");
		return { order[3].action, "CanVisitNewPosition /\\ !CanGoUp /\\ !CanGoRight /\\ !CanGoDown /\\ CanGoLeft => MoveLeft", 1 };
	}

	kb.AddInference(
		{ "CanVisitNewPosition", "!CanGoUp", "!CanGoRight", "!CanGoDown", "!CanGoLeft" },
		"NeedBacktrack"
	);

	if (mainPath.size() > 1) {
		Position prev = mainPath[mainPath.size() - 2];
		kb.AddInference({ "NeedBacktrack", "HasPreviousSituation" }, "ChooseBacktrack");
		return MoveTowardsPreviousPathNode(
			current,
			prev,
			"NeedBacktrack /\\ HasPreviousSituation => Backtrack"
		);
	}

	kb.AddInference({ "NeedBacktrack", "!HasPreviousSituation" }, "ExploreFinished");
	return { ActionType::Wait, "NeedBacktrack /\\ !HasPreviousSituation => ExploreFinished", 1 };
}

string Agent::InferCurrentGoal(const Environment& env, const Percept& percept) {
	bool hasBomb = env.IsBombPicked();
	bool atExit = percept.onExit;
	bool isInDangerZone = percept.bombSensed || percept.onBomb;
	bool foundExit = hasSeenExit || atExit;
	bool foundDangerBeforeUpdate = foundDangerZone;

	kb.AddQueryTrace(
		"CurrentGoal?",
		{ "HasBomb?", "IsInDangerZone?", "FoundDangerZone?", "FoundExit?", "AtExit?" },
		{
			string("HasBomb=") + (hasBomb ? "true" : "false"),
			string("IsInDangerZone=") + (isInDangerZone ? "true" : "false"),
			string("FoundDangerZone=") + (foundDangerBeforeUpdate ? "true" : "false"),
			string("FoundExit=") + (foundExit ? "true" : "false"),
			string("AtExit=") + (atExit ? "true" : "false")
		}
	);

	if (isInDangerZone) {
		foundDangerZone = true;
	}

	if (!hasBomb && !isInDangerZone && !foundDangerZone) {
		lastGoalInference = "!HasBomb /\\ !IsInDangerZone /\\ !FoundDangerZone => LookingForDangerZone";
		kb.AddInference(
			{ "!HasBomb", "!IsInDangerZone", "!FoundDangerZone" },
			"Goal(LookingForDangerZone)"
		);
		return "LookingForDangerZone";
	}

	if (!hasBomb && isInDangerZone && foundDangerZone) {
		lastGoalInference = "!HasBomb /\\ IsInDangerZone /\\ FoundDangerZone => SearchingDangerZone";
		kb.AddInference(
			{ "!HasBomb", "IsInDangerZone", "FoundDangerZone" },
			"Goal(SearchingDangerZone)"
		);
		return "SearchingDangerZone";
	}

	if (!hasBomb && !isInDangerZone && foundDangerZone) {
		lastGoalInference = "!HasBomb /\\ !IsInDangerZone /\\ FoundDangerZone => ReturningToDangerZone";
		kb.AddInference(
			{ "!HasBomb", "!IsInDangerZone", "FoundDangerZone" },
			"Goal(ReturningToDangerZone)"
		);
		return "ReturningToDangerZone";
	}

	if (hasBomb && returningToDangerEntry) {
		lastGoalInference = "HasBomb /\\ ReturningToDangerEntry => ReturnToDangerEntry";
		kb.AddInference(
			{ "HasBomb", "ReturningToDangerEntry" },
			"Goal(ReturnToDangerEntry)"
		);
		return "ReturnToDangerEntry";
	}

	if (hasBomb && !foundExit && !atExit) {
		lastGoalInference = "HasBomb /\\ !FoundExit /\\ !AtExit => LookingForExit";
		kb.AddInference(
			{ "HasBomb", "!FoundExit", "!AtExit" },
			"Goal(LookingForExit)"
		);
		return "LookingForExit";
	}

	if (hasBomb && foundExit && !atExit) {
		lastGoalInference = "HasBomb /\\ FoundExit /\\ !AtExit => ReturningToExit";
		kb.AddInference(
			{ "HasBomb", "FoundExit", "!AtExit" },
			"Goal(ReturningToExit)"
		);
		return "ReturningToExit";
	}

	if (hasBomb && atExit) {
		lastGoalInference = "HasBomb /\\ AtExit => MissionComplete";
		kb.AddInference(
			{ "HasBomb", "AtExit" },
			"Goal(MissionComplete)"
		);
		return "MissionComplete";
	}

	lastGoalInference = "NoRuleMatched => Unknown";
	kb.AddInference({}, "Goal(Unknown)");
	return "Unknown";
}

Action Agent::Decide(Environment& env) {
	situationCounter++;
	kb.SetCurrentSituation(situationCounter);
	kb.ClearTransientFacts();

	Position currentPosition = env.GetAgentPosition();
	Percept percept = env.GetPercept();

	SyncMainPathWithCurrentPosition(currentPosition);

	if (env.IsBombPicked()) {
		if (!returningToDangerEntry && hasSeenExit) {
			returningToExit = true;
		}
		dangerExplorationActive = false;
		dangerPath.clear();
		dangerNextDirIndex.clear();
		dangerVisitedCells.clear();
	}

	UpdateKnowledgeFromPercept(env, percept);
	UpdateBombCandidates(env, percept);
	BuildTemporaryNavigationFacts(env);
	currentGoal = InferCurrentGoal(env, percept);
	kb.AddGoalTrace({}, "CurrentGoal(" + currentGoal + ")");

	auto formatPath = [&](const vector<Position>& path) {
		ostringstream ss;
		ss << "[";
		for (int i = 0; i < (int)path.size(); i++) {
			if (i > 0) ss << " -> ";
			ss << "(" << path[i].x << "," << path[i].y << ")";
		}
		ss << "]";
		return ss.str();
	};

	{
		ostringstream sit;
		sit << "s=" << situationCounter
			<< ", goal=" << currentGoal
			<< ", s_main=" << (mainPath.empty() ? -1 : (int)mainPath.size() - 1)
			<< ", s_danger=" << (dangerPath.empty() ? -1 : (int)dangerPath.size() - 1)
			<< ", returningToExit=" << (returningToExit ? "true" : "false")
			<< ", returningToDangerEntry=" << (returningToDangerEntry ? "true" : "false")
			<< ", dangerActive=" << (dangerExplorationActive ? "true" : "false")
			<< ", bombCandidates=" << bombCandidates.size()
			<< ", mainPath=" << formatPath(mainPath)
			<< ", dangerPath=" << formatPath(dangerPath);
		lastSituationSnapshot = sit.str();
		kb.AddSituationLog(lastSituationSnapshot);
	}

	if (percept.onBomb && !env.IsBombPicked()) {
		kb.AddInference({ "OnBomb", "!HasBomb" }, "Choose(PickBomb)");
		if (hasDangerEntryPoint) {
			returningToDangerEntry = true;
			returningToExit = false;
			kb.AddInference(
				{ "OnBomb", "FoundDangerZoneEntry" },
				"NeedReturnToDangerEntryAfterPickup"
			);
		}
		kb.AddActionLog("Pick bomb");
		return { ActionType::PickBomb, "OnBomb /\\ !HasBomb => PickBomb", 2 };
	}

	if (returningToDangerEntry && env.IsBombPicked()) {
		if (currentPosition == dangerEntryPoint) {
			returningToDangerEntry = false;
			if (hasSeenExit) {
				returningToExit = true;
			}
			kb.AddInference(
				{ "HasBomb", "AtDangerEntry" },
				"ResumeExitSearchFromDangerEntry"
			);
		}
		else if (mainPath.size() > 1) {
			Position prev = mainPath[mainPath.size() - 2];
			kb.AddInference(
				{ "HasBomb", "ReturningToDangerEntry", "CanReturnToPreviousSituation" },
				"Choose(ReturnToDangerEntryStep)"
			);
			Action backToEntry = MoveTowardsPreviousPathNode(
				currentPosition,
				prev,
				"HasBomb /\\ ReturningToDangerEntry /\\ CanReturnToPreviousSituation => ReturnToDangerEntryStep"
			);
			kb.AddActionLog(backToEntry.description);
			return backToEntry;
		}
	}

	if (returningToExit && env.IsBombPicked()) {
		if (currentPosition == seenExit || percept.onExit) {
			returningToExit = false;
		}
		else if (hasKnownExitPath) {
			int idx = -1;
			for (int i = 0; i < (int)knownExitPath.size(); i++) {
				if (knownExitPath[i] == currentPosition) {
					idx = i;
					break;
				}
			}

			if (idx >= 0 && idx + 1 < (int)knownExitPath.size()) {
				Position next = knownExitPath[idx + 1];
				kb.AddInference(
					{ "HasBomb", "FoundExit", "OnKnownExitPath", "!AtExit" },
					"Choose(FollowKnownExitPath)"
				);
				Action step = MoveTowardsPreviousPathNode(
					currentPosition,
					next,
					"HasBomb /\\ FoundExit /\\ OnKnownExitPath /\\ !AtExit => FollowKnownExitPath"
				);
				kb.AddActionLog(step.description);
				return step;
			}

			if (idx == -1 && mainPath.size() > 1) {
				Position prev = mainPath[mainPath.size() - 2];
				kb.AddInference(
					{ "HasBomb", "FoundExit", "!OnKnownExitPath", "CanReturnToPreviousSituation" },
					"Choose(ReturnToKnownExitPath)"
				);
				Action back = MoveTowardsPreviousPathNode(
					currentPosition,
					prev,
					"HasBomb /\\ FoundExit /\\ !OnKnownExitPath /\\ CanReturnToPreviousSituation => ReturnToKnownExitPath"
				);
				kb.AddActionLog(back.description);
				return back;
			}
		}
	}

	if (!env.IsBombPicked() && (percept.bombSensed || !bombCandidates.empty())) {
		if (!dangerExplorationActive) {
			dangerExplorationActive = true;
			dangerEntryPoint = currentPosition;
			hasDangerEntryPoint = true;
			dangerPath.clear();
			dangerNextDirIndex.clear();
			dangerVisitedCells.clear();
		}
	}

	if (dangerExplorationActive && !env.IsBombPicked()) {
		Action dangerAction = DecideDangerExplorationAction(env, currentPosition, percept);
		if (!(dangerAction.type == ActionType::Wait && dangerAction.description == "DangerSearchFinished")) {
			kb.AddActionLog(dangerAction.description);
			return dangerAction;
		}
	}

	Action explore = DecideLocalExploreAction(env, currentPosition);
	kb.AddActionLog(explore.description);
	return explore;
}

string Agent::GetLastSituationSnapshot() const {
	return lastSituationSnapshot;
}

string Agent::GetLastGoalInference() const {
	return lastGoalInference;
}

string Agent::GetCurrentGoal() const {
	return currentGoal;
}

int Agent::GetCurrentSituation() const {
	return situationCounter;
}

const vector<Position>& Agent::GetMainPath() const {
	return mainPath;
}

const vector<Position>& Agent::GetKnownExitPath() const {
	return knownExitPath;
}

bool Agent::HasSeenExit() const {
	return hasSeenExit;
}

Position Agent::GetSeenExitPosition() const {
	return seenExit;
}

const vector<HistoryEntry>& Agent::GetHistory() const {
	return kb.GetHistory();
}

bool Agent::SaveHistoryToFile(const string& filename) const {
	ofstream file(filename);
	if (!file.is_open()) {
		return false;
	}

	for (const auto& entry : kb.GetHistory()) {
		file << "[S" << entry.situation << "][" << entry.kind << "] ";
		if (!entry.premises.empty()) {
			for (int i = 0; i < (int)entry.premises.size(); i++) {
				if (i > 0) file << " /\\ ";
				file << entry.premises[i];
			}
			file << " => ";
		}
		file << entry.conclusion << "\n";
	}

	return true;
}

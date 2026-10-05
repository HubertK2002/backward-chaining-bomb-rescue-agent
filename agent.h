#pragma once

#include "environment.h"
#include "knowledge_base.h"
#include "knowledge_reasoner.h"
#include <vector>
#include <map>
#include <set>
#include <string>

class Agent {
private:
	KnowledgeBase kb;
	BackwardReasoner reasoner;
	std::vector<Position> bombCandidates;
	std::set<std::string> globalVisitedCells;
	bool dangerExplorationActive = false;
	std::vector<Position> dangerPath;
	std::map<std::string, int> dangerNextDirIndex;
	std::set<std::string> dangerVisitedCells;
	bool foundDangerZone = false;
	bool returningToDangerEntry = false;
	Position dangerEntryPoint{ 0, 0 };
	bool hasDangerEntryPoint = false;
	std::vector<Position> mainPath;
	bool returningToExit = false;
	bool hasSeenExit = false;
	Position seenExit{ 0, 0 };
	std::vector<Position> knownExitPath;
	bool hasKnownExitPath = false;
	int situationCounter = -1;
	std::string currentGoal = "LookingForExit";
	std::string lastSituationSnapshot;
	std::string lastGoalInference;

	void UpdateKnowledgeFromPercept(const Environment& env, const Percept& percept);
	void UpdateBombCandidates(const Environment& env, const Percept& percept);
	void BuildTemporaryNavigationFacts(const Environment& env);
	int DistanceToNearestBombCandidate(int x, int y) const;
	std::string PosKey(const Position& p) const;
	void SyncDangerPathWithCurrentPosition(const Position& current);
	void SyncMainPathWithCurrentPosition(const Position& current);
	Action DecideDangerExplorationAction(const Environment& env, const Position& current, const Percept& percept);
	Action MoveTowardsPreviousPathNode(const Position& current, const Position& prev, const std::string& label) const;
	Action DecideLocalExploreAction(const Environment& env, const Position& current);
	std::string InferCurrentGoal(const Environment& env, const Percept& percept);

public:
	Agent();

	void Initialize();
	Action Decide(Environment& env);
	std::string GetLastSituationSnapshot() const;
	std::string GetLastGoalInference() const;
	std::string GetCurrentGoal() const;
	int GetCurrentSituation() const;
	void PrintHistory() const;
	bool SaveHistoryToFile(const std::string& filename) const;
};

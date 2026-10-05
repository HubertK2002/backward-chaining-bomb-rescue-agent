#include "knowledge_reasoner.h"

using namespace std;

BackwardReasoner::BackwardReasoner(KnowledgeBase& knowledgeBase) : kb(knowledgeBase) {}

void BackwardReasoner::AddRule(const Rule& rule) {
	rules.push_back(rule);
}

void BackwardReasoner::LoadDefaultRules() {
	// Main goal
	AddRule({ "DeliveredBomb", { "HasBomb", "AtExit" } });

	// Knowledge-level goals (optional for trace/extension)
	AddRule({ "KnowBombLocation", { "BombSensed" } });
	AddRule({ "NeedExplore", { "NotKnowBombLocation" } });
}

bool BackwardReasoner::ProveRecursive(const string& goal, vector<string>& proofStack, int depth) {
	if (depth > 20) {
		return false;
	}

	if (kb.HasFact(goal)) {
		kb.AddGoalTrace({}, "Fact already known: " + goal);
		return true;
	}

	proofStack.push_back(goal);

	for (const auto& rule : rules) {
		if (rule.head != goal) {
			continue;
		}

		bool ok = true;
		for (const auto& subgoal : rule.body) {
			if (!ProveRecursive(subgoal, proofStack, depth + 1)) {
				ok = false;
				break;
			}
		}

		if (ok) {
			kb.AddInference(rule.body, goal);
			proofStack.pop_back();
			return true;
		}
	}

	proofStack.pop_back();
	return false;
}

bool BackwardReasoner::Prove(const string& goal) {
	vector<string> proofStack;
	return ProveRecursive(goal, proofStack, 0);
}

Action BackwardReasoner::ChooseAction() {
	if (kb.HasFact("OnBomb") && !kb.HasFact("HasBomb")) {
		return { ActionType::PickBomb, "Pick bomb", 2 };
	}

	// If carrying bomb, go to exit with safe moves only.
	if (kb.HasFact("HasBomb")) {
		if (kb.HasFact("ExitRightSafe")) return { ActionType::MoveRight, "MoveRight toward exit", 1 };
		if (kb.HasFact("ExitDownSafe")) return { ActionType::MoveDown, "MoveDown toward exit", 1 };
		if (kb.HasFact("ExitUpSafe")) return { ActionType::MoveUp, "MoveUp toward exit", 1 };
		if (kb.HasFact("ExitLeftSafe")) return { ActionType::MoveLeft, "MoveLeft toward exit", 1 };
	}

	// If bomb is sensed, prioritize unvisited moves first.
	if (kb.HasFact("BombUpUnvisited")) return { ActionType::MoveUp, "MoveUp toward bomb", 1 };
	if (kb.HasFact("BombRightUnvisited")) return { ActionType::MoveRight, "MoveRight toward bomb", 1 };
	if (kb.HasFact("BombLeftUnvisited")) return { ActionType::MoveLeft, "MoveLeft toward bomb", 1 };
	if (kb.HasFact("BombDownUnvisited")) return { ActionType::MoveDown, "MoveDown toward bomb", 1 };

	if (kb.HasFact("BombUpSafe")) return { ActionType::MoveUp, "MoveUp toward bomb", 1 };
	if (kb.HasFact("BombRightSafe")) return { ActionType::MoveRight, "MoveRight toward bomb", 1 };
	if (kb.HasFact("BombLeftSafe")) return { ActionType::MoveLeft, "MoveLeft toward bomb", 1 };
	if (kb.HasFact("BombDownSafe")) return { ActionType::MoveDown, "MoveDown toward bomb", 1 };

	// Exploration fallback: prefer unvisited cells to avoid loops.
	if (kb.HasFact("ExploreRightUnvisited")) return { ActionType::MoveRight, "ExploreRight", 1 };
	if (kb.HasFact("ExploreDownUnvisited")) return { ActionType::MoveDown, "ExploreDown", 1 };
	if (kb.HasFact("ExploreUpUnvisited")) return { ActionType::MoveUp, "ExploreUp", 1 };
	if (kb.HasFact("ExploreLeftUnvisited")) return { ActionType::MoveLeft, "ExploreLeft", 1 };

	if (kb.HasFact("ExploreRightSafe")) return { ActionType::MoveRight, "ExploreRight", 1 };
	if (kb.HasFact("ExploreDownSafe")) return { ActionType::MoveDown, "ExploreDown", 1 };
	if (kb.HasFact("ExploreUpSafe")) return { ActionType::MoveUp, "ExploreUp", 1 };
	if (kb.HasFact("ExploreLeftSafe")) return { ActionType::MoveLeft, "ExploreLeft", 1 };

	return { ActionType::Wait, "Wait", 1 };
}

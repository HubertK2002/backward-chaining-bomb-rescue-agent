#include "knowledge_base.h"
#include <vector>

using namespace std;

void KnowledgeBase::PushHistory(const string& kind, const vector<string>& premises, const string& conclusion) {
	history.push_back({ currentSituation, kind, premises, conclusion });
}

void KnowledgeBase::SetCurrentSituation(int situation) {
	currentSituation = situation;
}

int KnowledgeBase::GetCurrentSituation() const {
	return currentSituation;
}

void KnowledgeBase::AddFact(const string& fact) {
	facts.insert(fact);
}

bool KnowledgeBase::HasFact(const string& fact) const {
	return facts.find(fact) != facts.end();
}

void KnowledgeBase::AddObservation(const string& obs) {
	PushHistory("OBS", {}, obs);
	AddFact(obs);
}

void KnowledgeBase::AddInference(const vector<string>& premises, const string& conclusion) {
	PushHistory("INF", premises, conclusion);
	AddFact(conclusion);
}

void KnowledgeBase::AddGoalTrace(const vector<string>& premises, const string& conclusion) {
	PushHistory("GOAL", premises, conclusion);
}

void KnowledgeBase::AddQueryTrace(const string& query, const vector<string>& checks, const vector<string>& answers) {
	string combined = query + " | asks: ";
	for (int i = 0; i < (int)checks.size(); i++) {
		if (i > 0) combined += ", ";
		combined += checks[i];
	}
	combined += " | answers: ";
	for (int i = 0; i < (int)answers.size(); i++) {
		if (i > 0) combined += ", ";
		combined += answers[i];
	}
	PushHistory("QRY", {}, combined);
}

void KnowledgeBase::AddActionLog(const string& action) {
	PushHistory("ACT", {}, action);
}

void KnowledgeBase::AddSituationLog(const string& situation) {
	PushHistory("SIT", {}, situation);
}

const vector<HistoryEntry>& KnowledgeBase::GetHistory() const {
	return history;
}

void KnowledgeBase::ClearTransientFacts() {
	vector<string> transientFacts = {
		"ExitRightSafe", "ExitLeftSafe", "ExitUpSafe", "ExitDownSafe",
		"BombRightSafe", "BombLeftSafe", "BombUpSafe", "BombDownSafe",
		"BombRightUnvisited", "BombLeftUnvisited", "BombUpUnvisited", "BombDownUnvisited",
		"ExploreRightSafe", "ExploreLeftSafe", "ExploreUpSafe", "ExploreDownSafe",
		"ExploreRightUnvisited", "ExploreLeftUnvisited", "ExploreUpUnvisited", "ExploreDownUnvisited",
		"BombSensed", "OnBomb", "AtExit",
		"WallUp", "WallDown", "WallLeft", "WallRight"
	};

	for (const auto& fact : transientFacts) {
		facts.erase(fact);
	}
}

string KnowledgeBase::PosToFact(const string& name, int x, int y) {
	return name + "(" + to_string(x) + "," + to_string(y) + ")";
}

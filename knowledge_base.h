#pragma once

#include "types.h"
#include <set>
#include <string>
#include <vector>

class KnowledgeBase {
private:
	std::set<std::string> facts;
	std::vector<HistoryEntry> history;
	int currentSituation = -1;
	void PushHistory(const std::string& kind, const std::vector<std::string>& premises, const std::string& conclusion);

public:
	void SetCurrentSituation(int situation);
	int GetCurrentSituation() const;

	void AddFact(const std::string& fact);
	bool HasFact(const std::string& fact) const;

	void AddObservation(const std::string& obs);
	void AddInference(const std::vector<std::string>& premises, const std::string& conclusion);
	void AddGoalTrace(const std::vector<std::string>& premises, const std::string& conclusion);
	void AddQueryTrace(const std::string& query, const std::vector<std::string>& checks, const std::vector<std::string>& answers);
	void AddActionLog(const std::string& action);
	void AddSituationLog(const std::string& situation);

	const std::vector<HistoryEntry>& GetHistory() const;

	void ClearTransientFacts();

	static std::string PosToFact(const std::string& name, int x, int y);
};

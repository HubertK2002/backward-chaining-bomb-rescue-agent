#pragma once

#include "knowledge_base.h"
#include "types.h"
#include <string>
#include <vector>

struct Rule {
	std::string head;
	std::vector<std::string> body;
};

class BackwardReasoner {
private:
	KnowledgeBase& kb;
	std::vector<Rule> rules;

	bool ProveRecursive(const std::string& goal, std::vector<std::string>& proofStack, int depth);

public:
	BackwardReasoner(KnowledgeBase& knowledgeBase);

	void AddRule(const Rule& rule);
	bool Prove(const std::string& goal);

	Action ChooseAction();
	void LoadDefaultRules();
};
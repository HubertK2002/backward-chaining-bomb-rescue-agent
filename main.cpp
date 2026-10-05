#include "environment.h"
#include "agent.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <limits>

using namespace std;

vector<string> GetMapFiles(const string& directory) {
	vector<string> files;

	for (const auto& entry : filesystem::directory_iterator(directory)) {
		if (entry.is_regular_file() && entry.path().extension() == ".txt") {
			files.push_back(entry.path().string());
		}
	}

	sort(files.begin(), files.end());
	return files;
}

string ChooseMapFile(const string& directory) {
	vector<string> files = GetMapFiles(directory);

	if (files.empty()) {
		throw runtime_error("No map files found in directory: " + directory);
	}

	cout << "Available maps:\n";
	for (int i = 0; i < (int)files.size(); i++) {
		cout << i + 1 << ". " << filesystem::path(files[i]).filename().string() << "\n";
	}

	int choice;
	while (true) {
		cout << "Choose map number: ";
		cin >> choice;

		if (!cin.fail() && choice >= 1 && choice <= (int)files.size()) {
			break;
		}

		cin.clear();
		cin.ignore(numeric_limits<streamsize>::max(), '\n');
		cout << "Invalid choice. Try again.\n";
	}

	return files[choice - 1];
}

int main() {
	try {
		const bool logicOnly = true;

		string selectedMap = ChooseMapFile("maps");
		cout << "\nSelected map: " << filesystem::path(selectedMap).filename().string() << "\n";

		Environment env;
		env.LoadFromFile(selectedMap);

		Agent agent;
		agent.Initialize();

		int step = 0;
		const int maxSteps = 200;

		while (!env.IsFinished() && step < maxSteps) {
			cout << "\n=== STEP " << step << " ===\n";
			env.Draw(!logicOnly);

			Action action = agent.Decide(env);
			cout << "Situation: S" << agent.GetCurrentSituation()
				 << " | Goal: " << agent.GetCurrentGoal() << "\n";
			cout << "Inferred situation: " << agent.GetLastGoalInference() << "\n";
			if (logicOnly) {
				cout << "Planned action: " << action.description << "\n";
			}
			else {
				cout << "Planned action: " << action.description
					<< " (cost: " << action.cost << ")\n";
			}
			Action executed = env.ExecuteAction(action);

			if (logicOnly) {
				cout << "Executed action: " << executed.description << "\n";
			}
			else {
				cout << "Executed action: " << executed.description
					<< " (cost: " << executed.cost << ")\n";
			}

			this_thread::sleep_for(chrono::milliseconds(400));
			step++;
		}

		cout << "\n=== FINAL ===\n";
		env.Draw(!logicOnly);

		if (env.MissionSucceeded()) {
			cout << "Mission succeeded.\n";
		}
		else if (env.MissionFailed()) {
			cout << "Mission failed.\n";
		}
		else {
			cout << "Mission stopped (step limit).\n";
		}

		if (!logicOnly) {
			cout << "Total cost = " << env.GetTotalCost() << "\n";
		}

		cout << "\n=== AGENT HISTORY ===\n";
		agent.PrintHistory();

		const string logFileName = "agent_history.log";
		if (agent.SaveHistoryToFile(logFileName)) {
			cout << "\nHistory saved to: " << logFileName << "\n";
		}
		else {
			cout << "\nCould not save history to file.\n";
		}
	}
	catch (const exception& e) {
		cerr << "Error: " << e.what() << "\n";
		return 1;
	}

	return 0;
}

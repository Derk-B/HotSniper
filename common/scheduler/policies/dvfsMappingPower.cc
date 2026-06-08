#include "dvfsMappingPower.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace std;

// Only enforce budgets for columns whose suffix matches one of these entries.
// A column name like "C_0_SQ" matches suffix "SQ".
static const vector<string> ACTIVE_COLUMN_SUFFIXES = {"IRF", "SQ"};

static bool matchesActiveSuffix(const string &colName) {
    for (const auto &suffix : ACTIVE_COLUMN_SUFFIXES) {
        if (colName.size() > suffix.size() + 1 &&
            colName.compare(colName.size() - suffix.size(), suffix.size(), suffix) == 0 &&
            colName[colName.size() - suffix.size() - 1] == '_') {
            return true;
        }
    }
    return false;
}

static vector<string> splitCSVLine(const string &line) {
    vector<string> tokens;
    stringstream ss(line);
    string token;
    while (getline(ss, token, ',')) {
        tokens.push_back(token);
    }
    return tokens;
}

DVFSMappingPower::DVFSMappingPower(const PerformanceCounters *performanceCounters,
                                   int coreRows, int coreColumns,
                                   int minFrequency, int maxFrequency,
                                   const string &powerMappingsPath,
                                   const string &powerLookupPath)
    : performanceCounters(performanceCounters),
      coreRows(coreRows), coreColumns(coreColumns),
      minFrequency(minFrequency), maxFrequency(maxFrequency),
      lastMappingKey(""), cachedFrequency(minFrequency)
{
    loadMappings(powerMappingsPath);
    loadLookup(powerLookupPath);
    cout << "[DVFSMappingPower] Loaded " << mappingBudgets.size()
         << " mappings and " << lookupTable.size() << " frequency entries." << endl;
}

void DVFSMappingPower::loadMappings(const string &path) {
    ifstream f(path);
    if (!f.is_open()) {
        cerr << "[DVFSMappingPower] Error: cannot open power mappings file: " << path << endl;
        exit(1);
    }
    string line;
    getline(f, line);
    vector<string> headers = splitCSVLine(line);

    while (getline(f, line)) {
        if (line.empty()) continue;
        vector<string> fields = splitCSVLine(line);
        if (fields.empty()) continue;
        string key = fields[0];
        map<string, double> budgets;
        for (size_t i = 1; i < fields.size() && i < headers.size(); i++) {
            if (!fields[i].empty()) {
                double v = stod(fields[i]);
                if (v > 0.0) {
                    budgets[headers[i]] = v;
                }
            }
        }
        mappingBudgets[key] = move(budgets);
    }
}

void DVFSMappingPower::loadLookup(const string &path) {
    ifstream f(path);
    if (!f.is_open()) {
        cerr << "[DVFSMappingPower] Error: cannot open power lookup file: " << path << endl;
        exit(1);
    }
    string line;
    getline(f, line);
    vector<string> headers = splitCSVLine(line);

    while (getline(f, line)) {
        if (line.empty()) continue;
        vector<string> fields = splitCSVLine(line);
        if (fields.empty() || fields[0].empty()) continue;
        int freqMHz = (int)(stod(fields[0]) * 1000.0 + 0.5);
        map<string, double> powers;
        for (size_t i = 1; i < fields.size() && i < headers.size(); i++) {
            if (!fields[i].empty()) {
                powers[headers[i]] = stod(fields[i]);
            }
        }
        lookupTable.push_back({freqMHz, move(powers)});
    }
    sort(lookupTable.begin(), lookupTable.end(),
         [](const pair<int, map<string, double>> &a, const pair<int, map<string, double>> &b) {
             return a.first < b.first;
         });
}

string DVFSMappingPower::getMappingKey(const vector<bool> &activeCores) const {
    string key;
    key.reserve(activeCores.size());
    for (bool active : activeCores) {
        key += active ? '1' : '0';
    }
    return key;
}

int DVFSMappingPower::findMaxFrequency(const map<string, double> &budgets) const {
    int best = -1;
    for (const auto &entry : lookupTable) {
        bool feasible = true;
        for (const auto &b : budgets) {
            if (!matchesActiveSuffix(b.first)) continue;
            auto it = entry.second.find(b.first);
            if (it != entry.second.end() && it->second > b.second) {
                feasible = false;
                break;
            }
        }
        if (feasible) {
            best = entry.first;
        }
    }
    return (best == -1) ? minFrequency : best;
}

vector<int> DVFSMappingPower::getFrequencies(const vector<int> &oldFrequencies,
                                              const vector<bool> &activeCores) {
    vector<int> frequencies(coreRows * coreColumns);
    string mappingKey = getMappingKey(activeCores);

    if (mappingKey != lastMappingKey) {
        cout << "[DVFSMappingPower] Mapping changed: \"" << lastMappingKey
             << "\" -> \"" << mappingKey << "\"" << endl;
        lastMappingKey = mappingKey;

        auto it = mappingBudgets.find(mappingKey);
        if (it == mappingBudgets.end() || it->second.empty()) {
            cout << "[DVFSMappingPower] No budget constraints for mapping \""
                 << mappingKey << "\", applying min frequency." << endl;
            cachedFrequency = minFrequency;
        } else {
            cachedFrequency = findMaxFrequency(it->second);
            cout << "[DVFSMappingPower] Mapping \"" << mappingKey
                 << "\" -> " << fixed << setprecision(1)
                 << (cachedFrequency / 1000.0) << " GHz (" << cachedFrequency << " MHz)" << endl;
        }
    }

    for (unsigned int i = 0; i < coreRows * coreColumns; i++) {
        frequencies[i] = activeCores[i] ? cachedFrequency : minFrequency;
    }
    return frequencies;
}

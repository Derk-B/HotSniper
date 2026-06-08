/**
 * DVFS policy that sets frequency based on per-mapping component power budgets.
 * On each mapping change (active/inactive core pattern), looks up budgets from
 * power_mappings.csv and finds the highest frequency satisfying all budgets
 * using power_lookup.csv.
 */

#ifndef __DVFS_MAPPING_POWER_H
#define __DVFS_MAPPING_POWER_H

#include <map>
#include <string>
#include <vector>
#include "dvfspolicy.h"

class DVFSMappingPower : public DVFSPolicy {
public:
    DVFSMappingPower(const PerformanceCounters *performanceCounters,
                     int coreRows, int coreColumns,
                     int minFrequency, int maxFrequency,
                     const std::string &powerMappingsPath,
                     const std::string &powerLookupPath);
    virtual std::vector<int> getFrequencies(const std::vector<int> &oldFrequencies,
                                            const std::vector<bool> &activeCores);

private:
    const PerformanceCounters *performanceCounters;
    unsigned int coreRows;
    unsigned int coreColumns;
    int minFrequency;
    int maxFrequency;

    // mapping key (e.g. "1010") -> component name -> budget (only non-zero entries)
    std::map<std::string, std::map<std::string, double>> mappingBudgets;

    // lookup table sorted ascending by frequency in MHz: {freqMHz, {component -> power}}
    std::vector<std::pair<int, std::map<std::string, double>>> lookupTable;

    std::string lastMappingKey;
    int cachedFrequency;

    void loadMappings(const std::string &path);
    void loadLookup(const std::string &path);
    std::string getMappingKey(const std::vector<bool> &activeCores) const;
    int findMaxFrequency(const std::map<std::string, double> &budgets) const;
};

#endif // __DVFS_MAPPING_POWER_H

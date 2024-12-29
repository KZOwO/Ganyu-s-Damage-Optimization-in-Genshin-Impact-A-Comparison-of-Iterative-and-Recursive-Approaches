#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <map>
#include <unordered_map>
using namespace std;

// Structs to hold artifact and weapon data
struct Substat {
    string name;
    double value;
};

struct Artifact {
    string set_name;
    string piece;
    string main_stat;
    double main_stat_value;
    vector<Substat> substats;
};

struct Weapon {
    string name;
    int refinement_level;
    double base_atk;
    string main_stat;
    double main_stat_value;
    vector<Substat> substats;
    double bonus_dmg;
};

// Artifact set effects mapping
unordered_map<string, pair<string, string>> artifact_set_effects = {
    {"Blizzard_Strayer", {"+15% Cryo DMG Bonus", "CRIT Rate +20% (Cryo), +20% (Frozen)"}},
    {"Gladiator's_Finale", {"+18% ATK", "+35% Normal Attack DMG"}},
    {"Shimenawa's_Reminiscence", {"+18% ATK", "+50% Normal/Charged Attack DMG after Skill"}},
    {"Emblem_of_Severed_Fate", {"+20% Energy Recharge", "+25% Burst DMG (ER-based)"}},
    {"Gilded_Dreams", {"+80 Elemental Mastery", "+14% ATK/25% EM for each party type"}},
    {"Retracing_Bolide", {"+35% Shield Strength", "+40% Normal/Charged DMG (shielded)"}},
    {"Vermillion_Hereafter", {"+18% ATK", "+8% ATK (stacked 4x after Skill)"}},
    {"Marechaussee_Hunter", {"+20% CRIT Rate (in Burst)", "+12% Normal/Charged DMG (stacked 3x)"}},
    {"Echoes_of_an_Offering", {"+18% ATK", "70% chance for +30% Normal DMG"}},
    {"Golden_Troupe", {"+20% Skill DMG", "+25% Skill DMG (off-field)"}}
};

// Function to parse artifacts from file
vector<Artifact> loadArtifacts(const string &filename, int max_lines) {
    vector<Artifact> artifacts;
    ifstream file(filename);
    string line;
    int count = 0;

    while (getline(file, line) && count < max_lines) {
        stringstream ss(line);
        Artifact artifact;
        ss >> artifact.set_name >> artifact.piece >> artifact.main_stat >> artifact.main_stat_value;

        string substat_name;
        double substat_value;
        while (ss >> substat_name >> substat_value) {
            artifact.substats.push_back({substat_name, substat_value});
        }

        if (artifact.substats.empty()) {
            cout << "Warning: Artifact with no substats -> " << line << endl;
        }

        artifacts.push_back(artifact);
        count++;
    }

    return artifacts;
}

// Function to parse weapons from file
vector<Weapon> loadWeapons(const string &filename) {
    vector<Weapon> weapons;
    ifstream file(filename);
    string line;

    while (getline(file, line)) {
        stringstream ss(line);
        Weapon weapon;
        ss >> weapon.name >> weapon.refinement_level >> weapon.base_atk >> weapon.main_stat >> weapon.main_stat_value;

        string substat_name;
        double substat_value;
        while (ss >> substat_name >> substat_value) {
            if (substat_name == "NONE") break;
            weapon.substats.push_back({substat_name, substat_value});
        }
        ss >> weapon.bonus_dmg;

        weapons.push_back(weapon);
    }

    return weapons;
}

// Function to calculate damage output
double calculateDamage(double base_atk, double weapon_bonus, double bonus_dmg, const vector<Substat> &substats, const vector<string> &set_effects) {
    double total_atk = base_atk + (base_atk * weapon_bonus / 100);
    double crit_rate = 0;
    double crit_dmg = 0;
    double cryo_dmg_bonus = 0;

    for (const auto &substat : substats) {
        if (substat.name == "Crit_Rate%") {
            crit_rate += substat.value;
        } else if (substat.name == "Crit_DMG%") {
            crit_dmg += substat.value;
        } else if (substat.name == "Cryo_DMG%") {
            cryo_dmg_bonus += substat.value;
        }
    }

    crit_rate = min(crit_rate, 100.0); // Cap Crit Rate at 100%

    // Add artifact set effects
    for (const auto &effect : set_effects) {
        if (effect == "+15% Cryo DMG Bonus") {
            cryo_dmg_bonus += 15;
        } else if (effect == "CRIT Rate +20% (Cryo), +20% (Frozen)") {
            crit_rate += 20; // Simplified assumption
        } else if (effect == "+18% ATK") {
            total_atk += base_atk * 0.18;
        }
    }

    double avg_crit_multiplier = 1 + (crit_dmg / 100) * (crit_rate / 100);
    double damage = total_atk * avg_crit_multiplier * (1 + cryo_dmg_bonus / 100);
    return damage * (1 + bonus_dmg / 100);
}

// Recursive function to find the best combination
void findBestCombinationRecursive(const vector<Artifact> &artifacts, const vector<Weapon> &weapons, int max_constellation,
                                   vector<Artifact> &current_combination, size_t artifact_index,
                                   double &max_damage, string &best_weapon, int &best_constellation,
                                   vector<Artifact> &best_artifacts) {
    static const vector<string> artifact_pieces = {"Flower", "Feather", "Sands", "Goblet", "Circlet"};

    if (current_combination.size() == artifact_pieces.size()) {
        for (const auto &weapon : weapons) {
            for (int constellation = 0; constellation <= max_constellation; ++constellation) {
                vector<Substat> combined_substats;
                for (const auto &artifact : current_combination) {
                    combined_substats.insert(combined_substats.end(), artifact.substats.begin(), artifact.substats.end());
                }
                combined_substats.insert(combined_substats.end(), weapon.substats.begin(), weapon.substats.end());

                vector<string> set_effects = {artifact_set_effects[current_combination[0].set_name].first, artifact_set_effects[current_combination[0].set_name].second};

                double damage = calculateDamage(weapon.base_atk, weapon.main_stat_value, weapon.bonus_dmg, combined_substats, set_effects);

                if (damage > max_damage) {
                    max_damage = damage;
                    best_weapon = weapon.name + " (R" + to_string(weapon.refinement_level) + ")";
                    best_constellation = constellation;
                    best_artifacts = current_combination;
                }
            }
        }
        return;
    }

    string current_piece = artifact_pieces[current_combination.size()];
    for (size_t i = artifact_index; i < artifacts.size(); ++i) {
        if (artifacts[i].piece == current_piece) {
            current_combination.push_back(artifacts[i]);
            findBestCombinationRecursive(artifacts, weapons, max_constellation, current_combination, 0, max_damage, best_weapon, best_constellation, best_artifacts);
            current_combination.pop_back();
        }
    }
}

int main() {
    // File paths based on your configuration
    string artifact_file = "../Artifacts_List.txt"; // Move one level up to reach BIG TASK
    string weapon_file = "../Weapons_With_Scaled_Refinement.txt"; // Move one level up to reach BIG TASK
    int max_constellation = 6; // Max constellation level for Ganyu
    int max_artifacts = 50; // Number of artifact lines to read

    vector<Artifact> artifacts = loadArtifacts(artifact_file, max_artifacts);
    vector<Weapon> weapons = loadWeapons(weapon_file);

    double max_damage = 0;
    string best_weapon;
    int best_constellation = 0;
    vector<Artifact> best_artifacts;
    vector<Artifact> current_combination;

    findBestCombinationRecursive(artifacts, weapons, max_constellation, current_combination, 0, max_damage, best_weapon, best_constellation, best_artifacts);

    // Output the results
    cout << "Best Constellation level: " << best_constellation << endl;
    cout << "Best Weapon: " << best_weapon << endl;
    cout << "Best Artifact Combination:" << endl;

    for (const auto &artifact : best_artifacts) {
        cout << artifact.piece << "\nSet: " << artifact.set_name
             << "\nMain Stat: " << artifact.main_stat << "\nSubstats:";
        if (artifact.substats.empty()) {
            cout << " None";
        } else {
            for (const auto &substat : artifact.substats) {
                cout << " " << substat.name << " " << substat.value;
            }
        }
        cout << endl;
    }

    return 0;
}

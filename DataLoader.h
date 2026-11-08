#ifndef DATA_LOADER_H
#define DATA_LOADER_H
#include "Recipe.h"
#include <cstddef>
#include <istream>
#include <string>
#include <unordered_map>
#include <vector>

// No stemming or synonym substitution: ingredient phrases match exactly after
// ASCII lowercasing and whitespace normalization.
std::string normalizeIngredient(const std::string& text);
bool readCsvRecord(std::istream& input, std::vector<std::string>& fields);
std::vector<std::string> parseIngredientList(const std::string& literal);

struct Query {
    std::vector<int> ingredientIds; // sorted, unique, known vocabulary IDs
    std::vector<std::string> tokens; // all normalized unique phrases
    std::vector<std::string> unknown;
    std::size_t size() const { return tokens.size(); }
};
struct LoadStats {
    std::size_t recordsRead = 0, accepted = 0, rejected = 0, duplicates = 0;
    double milliseconds = 0;
};
class RecipeDatabase {
public:
    // limit counts accepted recipes; zero means all records.
    LoadStats loadCsv(const std::string& path, std::size_t limit = 0);
    Query makeQuery(const std::string& commaSeparated) const;
    const std::vector<Recipe>& recipes() const { return recipes_; }
    const std::vector<std::string>& vocabulary() const { return vocabulary_; }
    // Accounted logical payload; excludes allocator/hash-node/container overhead.
    std::size_t payloadBytes() const;
private:
    std::vector<Recipe> recipes_;
    std::vector<std::string> vocabulary_;
    std::unordered_map<std::string, int> ingredientToId_;
};
#endif

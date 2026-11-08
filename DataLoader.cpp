#include "DataLoader.h"
#include <algorithm>
#include <charconv>
#include <chrono>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <unordered_set>

namespace {
bool space(unsigned char c) { return c==' ' || c=='\t' || c=='\r' || c=='\n' || c=='\f' || c=='\v'; }
int recipeId(const std::string& value) {
    int id = 0;
    auto parsed = std::from_chars(value.data(), value.data()+value.size(), id);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data()+value.size() || id <= 0)
        throw std::invalid_argument("Invalid recipe ID");
    return id;
}
}

std::string normalizeIngredient(const std::string& text) {
    std::string out;
    bool pendingSpace = false;
    for (unsigned char c : text) {
        if (space(c)) { pendingSpace = !out.empty(); continue; }
        if (pendingSpace) out += ' ';
        pendingSpace = false;
        out += static_cast<char>(c >= 'A' && c <= 'Z' ? c-'A'+'a' : c);
    }
    return out;
}

bool readCsvRecord(std::istream& input, std::vector<std::string>& fields) {
    fields.clear();
    std::string field;
    bool quoted = false, afterQuote = false, any = false;
    char c;
    while (input.get(c)) {
        any = true;
        if (quoted) {
            if (c == '"') {
                if (input.peek() == '"') { input.get(c); field += '"'; }
                else { quoted = false; afterQuote = true; }
            } else field += c;
        } else if (c == ',') {
            fields.push_back(std::move(field)); field.clear(); afterQuote = false;
        } else if (c == '\n' || c == '\r') {
            if (c == '\r' && input.peek() == '\n') input.get(c);
            fields.push_back(std::move(field)); return true;
        } else if (c == '"' && field.empty() && !afterQuote) quoted = true;
        else {
            if (afterQuote || c == '"') throw std::runtime_error("Malformed CSV quoting");
            field += c;
        }
    }
    if (quoted) throw std::runtime_error("Unterminated quoted CSV field");
    if (!any) return false;
    fields.push_back(std::move(field)); return true;
}

// Parse the list-of-strings subset of Python literals used by RAW_recipes.csv.
// Never evaluate dataset contents as code. Escaped quotes/backslashes and control
// characters are supported; unknown escapes are preserved literally.
std::vector<std::string> parseIngredientList(const std::string& literal) {
    std::size_t i = 0;
    auto skip = [&] { while (i < literal.size() && space(static_cast<unsigned char>(literal[i]))) ++i; };
    auto fail = [] { throw std::invalid_argument("Invalid ingredient list literal"); };
    skip(); if (i == literal.size() || literal[i++] != '[') fail();
    std::vector<std::string> result;
    skip();
    while (i < literal.size() && literal[i] != ']') {
        if (literal[i] != '\'' && literal[i] != '"') fail();
        char quote = literal[i++];
        std::string word;
        bool closed = false;
        while (i < literal.size()) {
            char c = literal[i++];
            if (c == quote) { closed = true; break; }
            if (c == '\\') {
                if (i == literal.size()) fail();
                c = literal[i++];
                if (c == 'n') word += '\n';
                else if (c == 'r') word += '\r';
                else if (c == 't') word += '\t';
                else if (c == '\\' || c == '\'' || c == '"') word += c;
                else { word += '\\'; word += c; }
            } else word += c;
        }
        if (!closed) fail();
        word = normalizeIngredient(word);
        if (!word.empty()) result.push_back(std::move(word));
        skip();
        if (i < literal.size() && literal[i] == ',') { ++i; skip(); }
        else if (i >= literal.size() || literal[i] != ']') fail();
    }
    if (i == literal.size() || literal[i++] != ']') fail();
    skip(); if (i != literal.size()) fail();
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

LoadStats RecipeDatabase::loadCsv(const std::string& path, std::size_t limit) {
    auto start = std::chrono::steady_clock::now();
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot open dataset: " + path);
    // Build transactionally: a failed reload preserves the existing database.
    RecipeDatabase next;
    std::vector<std::string> row;
    if (!readCsvRecord(file, row)) throw std::runtime_error("Dataset is empty");
    if (!row.empty() && row[0].compare(0, 3, "\xEF\xBB\xBF") == 0) row[0].erase(0,3);
    auto column = [&](const std::string& name) {
        auto it = std::find(row.begin(), row.end(), name);
        if (it == row.end()) throw std::runtime_error("Missing CSV column: " + name);
        return static_cast<std::size_t>(it-row.begin());
    };
    const auto nameCol = column("name"), idCol = column("id"), ingredientsCol = column("ingredients");
    const auto width = row.size();
    std::unordered_set<int> seen;
    LoadStats stats;
    while ((!limit || next.recipes_.size() < limit) && readCsvRecord(file, row)) {
        ++stats.recordsRead;
        if (row.size() != width) { ++stats.rejected; continue; }
        int id;
        std::vector<std::string> ingredients;
        try { id = recipeId(row[idCol]); ingredients = parseIngredientList(row[ingredientsCol]); }
        catch (const std::invalid_argument&) { ++stats.rejected; continue; }
        if (normalizeIngredient(row[nameCol]).empty() || ingredients.empty()) { ++stats.rejected; continue; }
        if (!seen.insert(id).second) { ++stats.duplicates; continue; }
        Recipe recipe{id, row[nameCol], {}};
        for (const auto& ingredient : ingredients) {
            auto found = next.ingredientToId_.find(ingredient);
            if (found == next.ingredientToId_.end()) {
                if (next.vocabulary_.size() >= static_cast<std::size_t>(std::numeric_limits<int>::max()))
                    throw std::runtime_error("Ingredient vocabulary exceeds integer ID range");
                int newId = static_cast<int>(next.vocabulary_.size());
                next.vocabulary_.push_back(ingredient);
                found = next.ingredientToId_.emplace(ingredient, newId).first;
            }
            recipe.ingredientIds.push_back(found->second);
        }
        std::sort(recipe.ingredientIds.begin(), recipe.ingredientIds.end());
        next.recipes_.push_back(std::move(recipe));
        ++stats.accepted;
    }
    if (next.recipes_.empty()) throw std::runtime_error("Dataset contains no usable recipes");
    *this = std::move(next);
    stats.milliseconds = std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    return stats;
}

Query RecipeDatabase::makeQuery(const std::string& text) const {
    Query query;
    std::size_t begin = 0;
    while (begin <= text.size()) {
        auto end = text.find(',', begin);
        auto token = normalizeIngredient(text.substr(begin, end == std::string::npos ? end : end-begin));
        if (!token.empty()) query.tokens.push_back(std::move(token));
        if (end == std::string::npos) break;
        begin = end+1;
    }
    std::sort(query.tokens.begin(), query.tokens.end());
    query.tokens.erase(std::unique(query.tokens.begin(), query.tokens.end()), query.tokens.end());
    if (query.tokens.empty()) throw std::invalid_argument("Enter at least one ingredient");
    for (const auto& token : query.tokens) {
        auto found = ingredientToId_.find(token);
        if (found == ingredientToId_.end()) query.unknown.push_back(token);
        else query.ingredientIds.push_back(found->second);
    }
    std::sort(query.ingredientIds.begin(), query.ingredientIds.end());
    return query;
}

std::size_t RecipeDatabase::payloadBytes() const {
    std::size_t bytes = 0;
    for (const auto& recipe : recipes_) bytes += sizeof(recipe.id) + recipe.name.size() + sizeof(int)*recipe.ingredientIds.size();
    for (const auto& ingredient : vocabulary_) bytes += ingredient.size();
    for (const auto& entry : ingredientToId_) bytes += entry.first.size() + sizeof(entry.second);
    return bytes;
}

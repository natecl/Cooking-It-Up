#ifndef RANKING_H
#define RANKING_H
#include "DataLoader.h"
enum class Method { Jaccard, Cosine };
const char* methodName(Method method);
struct SearchResult {
    std::vector<ScoredRecipe> matches;
    double milliseconds = 0;
    std::size_t evaluated = 0;
};
// Ranks only positive-overlap recipes, breaking exact score ties by ascending ID.
SearchResult findRecipes(const RecipeDatabase& db, const Query& query, std::size_t k, Method method);
#endif

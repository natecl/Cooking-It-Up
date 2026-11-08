#include "Similarity.h"
#include <stdexcept>

std::size_t ingredientOverlap(const std::vector<int>& query, const std::vector<int>& recipe) {
    std::size_t i=0, j=0, overlap=0;
    // Each step advances at least one pointer: O(q+r) time, O(1) space.
    while (i < query.size() && j < recipe.size()) {
        if (query[i] == recipe[j]) { ++overlap; ++i; ++j; }
        else if (query[i] < recipe[j]) ++i;
        else ++j;
    }
    return overlap;
}

double jaccardSimilarity(std::size_t overlap, std::size_t querySize, std::size_t recipeSize) {
    if (overlap > querySize || overlap > recipeSize) throw std::invalid_argument("Impossible overlap");
    const double unionSize = static_cast<double>(querySize) + static_cast<double>(recipeSize) - static_cast<double>(overlap);
    // Empty sets score zero; the UI rejects empty queries and loader rejects empty recipes.
    return unionSize == 0 ? 0 : static_cast<double>(overlap)/unionSize;
}

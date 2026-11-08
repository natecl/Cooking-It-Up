#include "MissingIngredients.h"
#include <algorithm>

std::vector<std::string> missingIngredients(const RecipeDatabase& db, const Recipe& recipe, const Query& query) {
    std::vector<std::string> missing;
    std::size_t j=0;
    for (int ingredient : recipe.ingredientIds) {
        while (j<query.ingredientIds.size() && query.ingredientIds[j]<ingredient) ++j;
        if (j==query.ingredientIds.size() || query.ingredientIds[j]!=ingredient)
            missing.push_back(db.vocabulary().at(static_cast<std::size_t>(ingredient)));
    }
    std::sort(missing.begin(),missing.end());
    return missing;
}

#ifndef MISSING_INGREDIENTS_H
#define MISSING_INGREDIENTS_H
#include "DataLoader.h"
std::vector<std::string> missingIngredients(const RecipeDatabase& db, const Recipe& recipe, const Query& query);
#endif

#ifndef SIMILARITY_H
#define SIMILARITY_H
#include <cstddef>
#include <vector>
// Precondition: both ID sequences are sorted ascending and contain no duplicates.
std::size_t ingredientOverlap(const std::vector<int>& query, const std::vector<int>& recipe);
double jaccardSimilarity(std::size_t overlap, std::size_t querySize, std::size_t recipeSize);
double cosineSimilarity(std::size_t overlap, std::size_t querySize, std::size_t recipeSize);
#endif

#include "Similarity.h"
#include <cmath>
#include <stdexcept>

double cosineSimilarity(std::size_t overlap, std::size_t querySize, std::size_t recipeSize) {
    if (overlap > querySize || overlap > recipeSize) throw std::invalid_argument("Impossible overlap");
    if (querySize == 0 || recipeSize == 0) return 0;
    // Binary vectors: dot product is overlap; norms are sqrt(set cardinality).
    return static_cast<double>(overlap)/std::sqrt(static_cast<double>(querySize)*static_cast<double>(recipeSize));
}

#include "Ranking.h"
#include "Similarity.h"
#include <algorithm>
#include <chrono>
#include <queue>
#include <stdexcept>

const char* methodName(Method method) { return method == Method::Jaccard ? "Jaccard" : "Cosine"; }

SearchResult findRecipes(const RecipeDatabase& db, const Query& query, std::size_t k, Method method) {
    if (!query.size()) throw std::invalid_argument("Query is empty");
    if (!k) throw std::invalid_argument("Result count must be positive");
    auto start = std::chrono::steady_clock::now();
    struct Candidate { std::size_t index, overlap; double score; int id; };
    auto better = [](const Candidate& a, const Candidate& b) {
        return a.score > b.score || (a.score == b.score && a.id < b.id);
    };
    // With 'better' as the comparator the worst retained result is at the top.
    std::priority_queue<Candidate,std::vector<Candidate>,decltype(better)> heap(better);
    const auto& recipes = db.recipes();
    for (std::size_t i=0; i<recipes.size(); ++i) {
        const auto& recipe = recipes[i];
        auto overlap = ingredientOverlap(query.ingredientIds, recipe.ingredientIds);
        if (!overlap) continue;
        double score = method == Method::Jaccard
            ? jaccardSimilarity(overlap, query.size(), recipe.ingredientIds.size())
            : cosineSimilarity(overlap, query.size(), recipe.ingredientIds.size());
        Candidate candidate{i,overlap,score,recipe.id};
        if (heap.size() < k) heap.push(candidate);
        else if (better(candidate,heap.top())) { heap.pop(); heap.push(candidate); }
    }
    std::vector<Candidate> selected;
    while (!heap.empty()) { selected.push_back(heap.top()); heap.pop(); }
    std::sort(selected.begin(),selected.end(),better);
    SearchResult result;
    result.evaluated = recipes.size();
    for (const auto& candidate : selected) {
        const auto& r = recipes[candidate.index];
        result.matches.push_back({r.id,r.name,candidate.score,static_cast<int>(candidate.overlap),static_cast<int>(r.ingredientIds.size())});
    }
    result.milliseconds = std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    return result;
}

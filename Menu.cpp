#include "Menu.h"
#include "MissingIngredients.h"
#include <algorithm>
#include <charconv>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <unordered_map>

std::size_t positiveNumber(const std::string& value, const std::string& label) {
    std::size_t result=0;
    auto parsed=std::from_chars(value.data(),value.data()+value.size(),result);
    if (parsed.ec!=std::errc{} || parsed.ptr!=value.data()+value.size() || result==0)
        throw std::invalid_argument(label+" must be a positive whole number");
    return result;
}

void displayResults(const RecipeDatabase& db, const Query& query, const SearchResult& result, Method method, std::ostream& out) {
    out << "\n" << methodName(method) << " results | " << result.evaluated << " recipes evaluated\n";
    out << std::fixed << std::setprecision(6);
    if (result.matches.empty()) out << "No recipes share any of these ingredients.\n";
    // Build the ID lookup once for this display, outside the measured search.
    std::unordered_map<int,const Recipe*> byId;
    if (!result.matches.empty()) for (const auto& r : db.recipes()) byId.emplace(r.id,&r);
    for (std::size_t i=0; i<result.matches.size(); ++i) {
        const auto& match=result.matches[i];
        out << i+1 << ". " << match.name << " [id=" << match.recipeId << "] | score=" << match.score
            << " | matched=" << match.overlapCount << '/' << match.recipeIngredientCount << '\n';
        auto missing=missingIngredients(db,*byId.at(match.recipeId),query);
        out << "   Missing ingredients: ";
        if (missing.empty()) out << "none - you have every listed ingredient";
        for (std::size_t j=0;j<missing.size();++j) out << (j?", ":"") << missing[j];
        out << '\n';
    }
    out << "Query time: " << std::setprecision(3) << result.milliseconds << " ms (scoring + top-N; excludes display)\n";
}

void runMenu(const RecipeDatabase& db, std::istream& in, std::ostream& out) {
    std::string ingredients, count="5", method="both", line;
    while (true) {
        out << "\nCOOKING IT UP\n"
            << "1. Enter ingredients: " << (ingredients.empty()?"(none)":ingredients) << '\n'
            << "2. Number of results: " << count << '\n'
            << "3. Method: " << method << '\n'
            << "4. Find recipes\n0. Exit\nChoice: " << std::flush;
        if (!std::getline(in,line) || line=="0") return;
        try {
            if (line=="1") {
                out << "Ingredients (comma-separated exact phrases): " << std::flush;
                if (!std::getline(in,line)) return;
                db.makeQuery(line); ingredients=line;
            } else if (line=="2") {
                out << "Number of results: " << std::flush;
                if (!std::getline(in,line)) return;
                positiveNumber(line,"Result count"); count=line;
            } else if (line=="3") {
                out << "Method (jaccard / cosine / both): " << std::flush;
                if (!std::getline(in,line)) return;
                line=normalizeIngredient(line);
                if (line!="jaccard" && line!="cosine" && line!="both") throw std::invalid_argument("Choose jaccard, cosine, or both");
                method=line;
            } else if (line=="4") {
                auto query=db.makeQuery(ingredients);
                if (!query.unknown.empty()) {
                    out << "Unknown ingredients (kept in score denominator): ";
                    for (const auto& word:query.unknown) out << word << "; ";
                    out << '\n';
                }
                for (Method m : {Method::Jaccard,Method::Cosine})
                    if (method=="both" || method==normalizeIngredient(methodName(m)))
                        displayResults(db,query,findRecipes(db,query,positiveNumber(count,"Result count"),m),m,out);
            } else out << "Choose 0, 1, 2, 3, or 4.\n";
        } catch (const std::invalid_argument& error) { out << "Input error: " << error.what() << '\n'; }
    }
}

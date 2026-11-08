#ifndef MENU_H
#define MENU_H
#include "Ranking.h"
#include <iosfwd>
#include <string>
std::size_t positiveNumber(const std::string& value, const std::string& label);
void displayResults(const RecipeDatabase& db, const Query& query, const SearchResult& result, Method method, std::ostream& out);
void runMenu(const RecipeDatabase& db, std::istream& in, std::ostream& out);
void runBenchmark(const std::string& dataset, const std::string& output, std::size_t repeats);
#endif

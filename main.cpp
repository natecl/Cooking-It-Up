#include "Menu.h"
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char** argv) {
    try {
        std::string dataset="data/sample_recipes.csv", ingredients, method="both", benchmark;
        std::size_t k=5, limit=0, repeats=11;
        bool queryProvided=false;
        for (int i=1;i<argc;++i) {
            std::string option=argv[i];
            if (option=="--help" || option=="-h") {
                std::cout << "Cooking It Up (C++17)\n"
                    << "Run from the repository root.\n"
                    << "  --data PATH       Food.com CSV (default: original 8-recipe sample)\n"
                    << "  --query PHRASES   Comma-separated ingredients; omit for interactive menu\n"
                    << "  --top N           Positive result count (default 5)\n"
                    << "  --method NAME     jaccard, cosine, or both (default both)\n"
                    << "  --limit N         Load first N valid recipes (default all)\n"
                    << "  --benchmark PATH  Write benchmark CSV, including preprocessing separately\n"
                    << "  --repeats N       Timed repetitions per benchmark case (default 11)\n";
                return 0;
            }
            if (i+1>=argc) throw std::invalid_argument("Missing value for "+option);
            std::string value=argv[++i];
            if (option=="--data") dataset=value;
            else if (option=="--query") { ingredients=value; queryProvided=true; }
            else if (option=="--method") method=normalizeIngredient(value);
            else if (option=="--top") k=positiveNumber(value,"Result count");
            else if (option=="--limit") limit=positiveNumber(value,"Limit");
            else if (option=="--repeats") repeats=positiveNumber(value,"Repetitions");
            else if (option=="--benchmark") benchmark=value;
            else throw std::invalid_argument("Unknown option: "+option);
        }
        if (method!="both" && method!="jaccard" && method!="cosine") throw std::invalid_argument("Method must be jaccard, cosine, or both");
        if (!benchmark.empty()) {
            if (queryProvided || limit) throw std::invalid_argument("Benchmark uses fixed queries and sizes; omit --query and --limit");
            runBenchmark(dataset,benchmark,repeats); return 0;
        }
        RecipeDatabase db;
        auto stats=db.loadCsv(dataset,limit);
        std::cout << "Loaded " << stats.accepted << " recipes; vocabulary=" << db.vocabulary().size()
            << "; rejected=" << stats.rejected << "; duplicate IDs=" << stats.duplicates
            << "; preprocessing=" << stats.milliseconds << " ms\n";
        if (!queryProvided) { runMenu(db,std::cin,std::cout); return 0; }
        auto query=db.makeQuery(ingredients);
        for (const auto& unknown:query.unknown) std::cout << "Unknown ingredient (retained in denominator): " << unknown << '\n';
        for (Method m:{Method::Jaccard,Method::Cosine})
            if (method=="both" || method==normalizeIngredient(methodName(m)))
                displayResults(db,query,findRecipes(db,query,k,m),m,std::cout);
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n'; return 1;
    }
    return 0;
}

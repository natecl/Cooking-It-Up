#include "DataLoader.h"
#include "Similarity.h"
#include "Ranking.h"
#include "MissingIngredients.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>

int checks=0;
void check(bool condition,const char* message) { ++checks; if (!condition) throw std::runtime_error(message); }
template<class F> void throws(F fn) { bool did=false; try { fn(); } catch(const std::exception&) { did=true; } check(did,"Expected exception"); }
void near(double actual,double expected) { check(std::abs(actual-expected)<1e-12,"Incorrect similarity score"); }

int main(int argc,char** argv) {
    try {
        if(argc!=2) throw std::runtime_error("Pass fixture directory");
        std::string fixture=std::string(argv[1])+"/edge_recipes.csv";
        check(normalizeIngredient("  OLIVE\t OIL \r\n")=="olive oil","Normalization");
        auto list=parseIngredientList("[\"chef's salt\", 'rice', ' RICE ', 'a\\\\b', 'a\\\'b', '']");
        check(list.size()==4,"Literal normalization/deduplication");
        check(std::find(list.begin(),list.end(),"a'b")!=list.end(),"Escaped quote");
        check(std::find(list.begin(),list.end(),"a\\b")!=list.end(),"Escaped slash");
        for(const std::string bad:{"[rice]","['rice' 'salt']","['rice'","[] garbage","['x',,]"}) throws([&]{parseIngredientList(bad);});
        std::istringstream csv("a,b\r\n\"x, y\",\"two\"\"quotes\nline\"\r\n");
        std::vector<std::string> fields;
        check(readCsvRecord(csv,fields)&&fields.size()==2,"CSV header");
        check(readCsvRecord(csv,fields)&&fields[0]=="x, y"&&fields[1]=="two\"quotes\nline","CSV quoting/multiline");
        check(!readCsvRecord(csv,fields),"CSV EOF");
        std::istringstream broken("\"unterminated"); throws([&]{readCsvRecord(broken,fields);});
        near(jaccardSimilarity(2,3,4),0.4);
        near(cosineSimilarity(2,3,4),2/std::sqrt(12.0));
        near(jaccardSimilarity(0,0,0),0); near(cosineSimilarity(0,0,4),0);
        near(jaccardSimilarity(3,3,3),1); near(cosineSimilarity(3,3,3),1);
        throws([]{jaccardSimilarity(3,2,4);}); throws([]{cosineSimilarity(3,4,2);});
        check(ingredientOverlap({1,3,5},{2,3,5,7})==2,"Two-pointer intersection");
        RecipeDatabase db; auto stats=db.loadCsv(fixture);
        check(stats.recordsRead==10&&stats.accepted==4&&stats.rejected==5&&stats.duplicates==1,"Cleaning counters");
        check(db.recipes()[1].name=="A \"quoted\" name\non two lines","Multiline name");
        auto query=db.makeQuery(" RICE, rice, SALT, unknown ");
        check(query.size()==3&&query.ingredientIds.size()==2&&query.unknown.size()==1,"Query set and unknown words");
        auto j=findRecipes(db,query,99,Method::Jaccard);
        check(j.matches.size()==4&&j.matches.front().recipeId==10,"Ranking and excessive k");
        near(j.matches.front().score,2.0/3);
        auto rice=db.makeQuery("rice");
        auto tied=findRecipes(db,rice,4,Method::Jaccard);
        check(tied.matches[0].recipeId==80&&tied.matches[1].recipeId==10&&tied.matches[2].recipeId==20&&tied.matches[3].recipeId==70,"Tie ordering");
        auto miss=missingIngredients(db,db.recipes()[0],rice);
        check(miss==std::vector<std::string>{"salt"},"Missing ingredient names");
        check(missingIngredients(db,db.recipes()[0],query).empty(),"No missing ingredients");
        check(findRecipes(db,db.makeQuery("unobtainium"),4,Method::Cosine).matches.empty(),"Zero overlap excluded");
        throws([&]{db.makeQuery(" , , ");}); throws([&]{findRecipes(db,rice,0,Method::Jaccard);});
        throws([&]{db.loadCsv(fixture+".missing");});
        check(db.recipes().size()==4,"Failed reload preserves database");
        auto prefix=db.loadCsv(fixture,2);
        check(prefix.accepted==2&&prefix.recordsRead==2,"Accepted recipe prefix limit");
        db.loadCsv(fixture);

        // Independent oracle: set membership for overlap + full sort for ranking.
        // Reuse a fixture database but vary every possible subset including unknowns.
        std::vector<std::string> words={"rice","salt","water","garlic","unknown"};
        for(unsigned mask=1;mask<32;++mask) {
            std::string text;
            for(unsigned i=0;i<words.size();++i) if(mask&(1u<<i)) text+=words[i]+",";
            auto q=db.makeQuery(text);
            std::set<int> qs(q.ingredientIds.begin(),q.ingredientIds.end());
            for(Method m:{Method::Jaccard,Method::Cosine}) {
                std::vector<ScoredRecipe> oracle;
                for(const auto& r:db.recipes()) {
                    int overlap=0; for(int id:r.ingredientIds) overlap+=static_cast<int>(qs.count(id));
                    if(!overlap) continue;
                    double score=m==Method::Jaccard?static_cast<double>(overlap)/(q.size()+r.ingredientIds.size()-overlap)
                        : static_cast<double>(overlap)/std::sqrt(static_cast<double>(q.size())*r.ingredientIds.size());
                    oracle.push_back({r.id,r.name,score,overlap,static_cast<int>(r.ingredientIds.size())});
                }
                std::sort(oracle.begin(),oracle.end(),[](const auto& a,const auto& b){return a.score>b.score||(a.score==b.score&&a.recipeId<b.recipeId);});
                for(std::size_t k:{std::size_t(1),std::size_t(2),std::size_t(10)}) {
                    auto result=findRecipes(db,q,k,m);
                    check(result.matches.size()==std::min(k,oracle.size()),"Oracle result size");
                    for(std::size_t i=0;i<result.matches.size();++i) {
                        check(result.matches[i].recipeId==oracle[i].recipeId,"Heap versus full-sort oracle");
                        near(result.matches[i].score,oracle[i].score);
                    }
                }
            }
        }
        // Random set intersections independently check the overlap primitive.
        std::mt19937 generator(3530);
        for(int trial=0;trial<1000;++trial) {
            std::set<int> a,b; for(int i=0;i<30;++i) { if(generator()%2) a.insert(i); if(generator()%3==0) b.insert(i); }
            std::size_t expected=0; for(int id:a) expected+=b.count(id);
            check(ingredientOverlap({a.begin(),a.end()},{b.begin(),b.end()})==expected,"Random intersection oracle");
        }
        std::cout << "Passed " << checks << " checks\n";
    } catch(const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}

#include "Menu.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>

void runBenchmark(const std::string& dataset, const std::string& output, std::size_t repeats) {
    if (!repeats) throw std::invalid_argument("Repetitions must be positive");
    std::ofstream csv(output);
    if (!csv) throw std::runtime_error("Cannot write benchmark CSV: "+output);
    csv << "recipes,vocabulary,records_read,rejected,duplicates,load_ms,payload_bytes,query_id,query,method,repeats,median_ms,p95_ms,top_k,returned,top1_id,top10_overlap,ordered_equal\n";
    csv << std::fixed << std::setprecision(6);
    const std::vector<std::string> queries={
        "chicken, rice, salt", "flour, sugar, eggs, butter", "tomatoes, garlic, olive oil, onion",
        "milk", "water, salt, pepper, onion, garlic, butter, sugar, flour",
        "chicken, unobtainium spice", "unobtainium spice"};
    std::set<std::size_t> completed;
    for (std::size_t limit : {std::size_t(1000),std::size_t(10000),std::size_t(100000),std::size_t(0)}) {
        RecipeDatabase db;
        auto stats=db.loadCsv(dataset,limit);
        if (!completed.insert(stats.accepted).second) continue;
        for (std::size_t q=0;q<queries.size();++q) {
            auto query=db.makeQuery(queries[q]);
            auto j=findRecipes(db,query,10,Method::Jaccard); // one warmup per method
            auto c=findRecipes(db,query,10,Method::Cosine);
            std::size_t overlap=0;
            for (const auto& a:j.matches) for (const auto& b:c.matches) if (a.recipeId==b.recipeId) ++overlap;
            bool equal=j.matches.size()==c.matches.size();
            for (std::size_t i=0;equal && i<j.matches.size();++i) equal=j.matches[i].recipeId==c.matches[i].recipeId;
            std::vector<double> jt,ct;
            for (std::size_t rep=0;rep<repeats;++rep) {
                // Alternate method order to reduce systematic cache/order bias.
                if (rep%2==0) { jt.push_back(findRecipes(db,query,10,Method::Jaccard).milliseconds); ct.push_back(findRecipes(db,query,10,Method::Cosine).milliseconds); }
                else { ct.push_back(findRecipes(db,query,10,Method::Cosine).milliseconds); jt.push_back(findRecipes(db,query,10,Method::Jaccard).milliseconds); }
            }
            for (Method method:{Method::Jaccard,Method::Cosine}) {
                auto& times=method==Method::Jaccard?jt:ct;
                const auto& result=method==Method::Jaccard?j:c;
                std::sort(times.begin(),times.end());
                double median=times[times.size()/2];
                if (times.size()%2==0) median=(median+times[times.size()/2-1])/2;
                auto p95=static_cast<std::size_t>(std::ceil(0.95*static_cast<double>(times.size())))-1;
                csv << stats.accepted << ',' << db.vocabulary().size() << ',' << stats.recordsRead << ',' << stats.rejected << ',' << stats.duplicates
                    << ',' << stats.milliseconds << ',' << db.payloadBytes() << ',' << q+1 << ",\"" << queries[q] << "\","
                    << methodName(method) << ',' << repeats << ',' << median << ',' << times[p95] << ",10," << result.matches.size() << ',';
                if (!result.matches.empty()) csv << result.matches.front().recipeId;
                csv << ',' << overlap << ',' << (equal?1:0) << '\n';
            }
        }
        std::cout << "Benchmarked " << stats.accepted << " recipes (load " << stats.milliseconds << " ms)\n";
    }
    csv.flush();
    if (!csv) throw std::runtime_error("Failed writing benchmark CSV");
    std::cout << "Saved " << output << '\n';
}

# Cooking It Up

**CodeMonks - COP3530 Project 2b**  
Artem Antonian, Nathan Chin-Lue, Marissa Micioni

Enter the ingredients in your pantry and compare **Jaccard** and **binary cosine** recipe rankings. Both similarity algorithms and the sorted-set intersection are implemented directly in C++17. No similarity/search framework is used.

## Run immediately with the sample

Requirements: a C++17 compiler and CMake 3.16+. Python 3.11+ is used for downloading the full dataset and optional integration tests; it is not required by the C++ application.

```sh
git clone https://github.com/natecl/Cooking-It-Up.git
cd Cooking-It-Up
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Run from the repository root:

```sh
./build/cooking_it_up
```

On Windows with Visual Studio CMake generators use `build\Release\cooking_it_up.exe` in place of `./build/cooking_it_up` in all commands. The default is the original **8-recipe demonstration fixture**, not the full Food.com dataset.

The menu lets you enter comma-separated ingredient phrases, set the result count, select Jaccard/cosine/both, search repeatedly, and exit. Invalid entries display an explanation and preserve the last valid setting. End-of-file exits cleanly.

One-shot search:

```sh
./build/cooking_it_up --query "chicken, rice, salt" --top 3 --method both
./build/cooking_it_up --help
```

Each method prints recipe names and IDs, scores, matched ingredient counts, missing ingredients, and elapsed search time. The sample's `Simple chicken rice` scores 1.0 for both methods with this query.

## Use the real dataset

Source: [Food.com Recipes and Interactions on Kaggle](https://www.kaggle.com/datasets/shuyangli94/food-com-recipes-and-user-interactions), version 2. Only `RAW_recipes.csv` is needed; no interactions or reviews are used. It is approximately 281 MiB uncompressed and is intentionally excluded from Git history.

```sh
python3 scripts/download_data.py
./build/cooking_it_up --data data/RAW_recipes.csv
./build/cooking_it_up --data data/RAW_recipes.csv --query "chicken, rice, salt" --top 5
```

If Kaggle blocks anonymous downloads, download `RAW_recipes.csv` from the linked dataset page with your own account and place it in `data/`. The app reads the raw CSV directly in C++; no preprocessing tool is required. The supplied dataset snapshot produces **231,636 usable recipes**, **14,941 normalized ingredients**, and one rejected blank-name record out of 231,637 source records.

SHA-256 of the tested raw CSV:

```text
6a8136d1da9e03396a9f52d72200b3fddb8fe103f11e1496effca30ac4e0539f
```

The downloader leaves an existing file unchanged. Data rights remain with the original authors; see [provenance](docs/PROVENANCE.md).

## Matching rules and algorithms

- Ingredient phrases are lowercased for ASCII letters, trimmed, whitespace-collapsed, and deduplicated. `olive oil` is one ingredient. Exact normalized phrases are required; `egg` and `eggs` remain different. Ingredient phrases containing commas cannot be expressed as a single menu token.
- Recipes hold sorted unique integer ingredient IDs in a shared hash-map vocabulary. A two-pointer walk computes overlap without building dense vocabulary-sized vectors.
- Jaccard: `overlap / (query_size + recipe_size - overlap)`.
- Binary cosine: `overlap / sqrt(query_size * recipe_size)`.
- Unknown query phrases remain in the query's cardinality, lowering similarity correctly, but never enter the shared vocabulary. All-unknown queries report no matches. Empty queries are rejected.
- Only positive-overlap recipes are returned. A bounded heap retains the best N results; final ties use ascending recipe ID. Requesting more results than available returns all positive matches.
- Zero missing ingredients means the pantry covers a recipe's listed ingredients, not that all pantry items are used. A score below 1 can still mean no missing ingredients.
- Search time includes scanning, scoring, heap selection, final sorting, and result materialization. It excludes CSV loading, query normalization, missing-ingredient display, and terminal output.

## Tests and benchmarks

```sh
# Independent Python set/full-sort oracle on the downloaded full dataset
python3 tests/test_full_dataset.py build/cooking_it_up data/RAW_recipes.csv

# Seven fixed queries at 1k, 10k, 100k, and all usable recipes
./build/cooking_it_up --data data/RAW_recipes.csv --benchmark benchmarks/results.csv --repeats 21

# Address/undefined-behavior checks on supported Clang/GCC platforms
cmake -S . -B build-sanitize -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
cmake --build build-sanitize --parallel
ctest --test-dir build-sanitize --output-on-failure
```

Each benchmark case has one warmup per method, alternating timed method order, and median/p95 timing. Preprocessing is measured separately. Dataset subsets are deterministic file prefixes, so their recipe distributions differ. Results describe this workload and machine, not a universal winner. `payload_bytes` is explicitly accounted logical storage and excludes allocator, hash-node, and capacity overhead; it is not process RSS.

Unit tests include CSV multiline/escaped fields, malformed input, cleaning, duplicate IDs, empty and unknown queries, hand-calculated scores, ties, oversized N, missing ingredients, exhaustive query subsets versus full-sort rankings, and 1,000 random overlap checks. Integration tests exercise the real executable, menu recovery, repeated searches, and a case where the two methods rank differently. GitHub Actions checks Linux, macOS, Windows, and Linux sanitizers.

## Files

| File | Purpose |
|---|---|
| `Recipe.h` | Inherited shared structures |
| `DataLoader.*` | Streaming CSV, cleaning, vocabulary, query normalization |
| `Jaccard.cpp`, `Cosine.cpp`, `Similarity.h` | Handwritten overlap and similarity functions |
| `Ranking.*` | Bounded top-N selection and stable tie policy |
| `MissingIngredients.*` | Recipe-minus-pantry set difference |
| `main.cpp`, `Menu.*` | Command-line arguments and interactive menu |
| `Benchmark.cpp` | Reproducible experiment harness |
| `benchmarks/` | Actual measured timings, environment, validation evidence |
| `docs/` | Report draft, recording script, complexity, requirements, provenance |

## Submission status

The implementation and measured experiments are complete. The five-page, 12-point PDF is a **draft** until the team supplies truthful actual contributions and reflection notes, and the uploaded YouTube URL. Artem's username is omitted at the user's request. See [submission checklist](docs/SUBMISSION_CHECKLIST.md) and [recording script](docs/VIDEO_SCRIPT.md).

The team proposal and inherited header are credited in [PROVENANCE.md](docs/PROVENANCE.md). The implementation and supporting technical materials were generated with Codex assistance at Nathan's request. Planned roles are not claimed as completed human contributions. No Canvas submission has been made.

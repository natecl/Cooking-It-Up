# Cooking It Up recording script

**Target: 4 minutes 30 seconds. Hard limit: 5 minutes.** A single member may present. Record a real terminal walkthrough in Loom, Zoom, or a screen recorder, then upload the finished recording to YouTube as public or unlisted. This file is the script the user requested, not a completed video.

## Prepare before recording

Run from the repository root. Build the Release binary and download the data using the README. Enlarge your terminal font so the output is legible at 1080p. Have the repository, report page 4, and `benchmarks/results.csv` open. Do not spend recording time downloading or compiling. Rehearse once and keep the recording below five minutes.

On Windows substitute `build\Release\cooking_it_up.exe` for `./build/cooking_it_up`.

## 0:00-0:25 - Introduce the problem

Show the repository README.

> This is Cooking It Up, the CodeMonks project for COP3530. Our team is Artem Antonian, Nathan Chin-Lue, and Marissa Micioni. The program takes ingredients already in a pantry and finds recipes with similar ingredient sets. It compares Jaccard and binary cosine similarity, both implemented directly in C++.

Describe AI assistance and your actual contributions accurately if discussing authorship; use the provenance record rather than the proposal's planned roles as evidence of completed work.

## 0:25-1:35 - Show the interactive interface on real data

```sh
./build/cooking_it_up --data data/RAW_recipes.csv
```

Point out `231636` loaded recipes, `14941` vocabulary entries, and the separate preprocessing time. Then enter the following menu responses, one per prompt:

```text
1
chicken, rice, salt
2
3
3
both
4
```

> The loader reads the real Food.com CSV, normalizes ingredient phrases, and stores sorted unique ingredient IDs. One blank-name record is rejected. Here I enter three pantry ingredients, request three results, and compare both methods. Each result shows the name, ID, similarity, number of matched ingredients, missing ingredients, and query time. An empty missing list means I have the recipe's listed ingredients; its score can still be below one if my pantry has extras.

Point out the different numeric scores and any ties. Explain that ties use ascending recipe ID.

## 1:35-2:25 - Demonstrate settings, validation, and repeated search

Continue in the same process:

```text
2
0
1
CHICKEN, chicken, unobtainium spice
3
cosine
4
1
unobtainium spice
4
0
```

> A result count of zero is rejected and the last valid count remains. I can change the ingredients and select cosine alone without restarting. Case differences and duplicates normalize to one ingredient. Unknown phrases are reported and stay in the denominator. An all-unknown query returns a clear no-match message. Empty input is also rejected. Now I exit normally.

## 2:25-3:10 - Explain why the algorithms can rank differently

Show `Jaccard.cpp`, `Cosine.cpp`, and the ranking-reversal paragraph in `docs/COMPLEXITY.md`.

> A two-pointer walk counts overlap between sorted sets. Jaccard divides overlap by the union size. Binary cosine divides it by the square root of the two set sizes multiplied together. For a four-ingredient query, a recipe matching two out of three scores 0.400 by Jaccard and 0.577 by cosine. A recipe matching all four out of eleven scores 0.364 and 0.603. Jaccard prefers the first recipe; cosine prefers the second. The integration tests verify that reversal. A bounded heap retains the best N matches instead of sorting every recipe.

## 3:10-3:55 - Show tests and measured comparison

```sh
ctest --test-dir build -C Release --output-on-failure
```

Show `benchmarks/validation.txt` and report page 4. If needed, show the benchmark command already documented in the README; avoid waiting for a complete benchmark run on camera.

> The local tests include 1,910 C++ checks, command-line and menu integration, and sanitizer checks. An independent Python set-based reference agrees with all fourteen full-dataset top-ten rankings. Benchmarks use seven fixed queries, four dataset sizes, one warmup, and twenty-one repetitions per method. At full size, both methods average about 3.9 milliseconds across query medians on this M4 Pro. The difference is small; neither is a universal winner. Loading takes about 1.88 seconds and is reported separately.

## 3:55-4:30 - Complexity, limits, and close

Show `docs/COMPLEXITY.md` and README run commands.

> For N recipes, q query ingredients, maximum recipe length r, and k results, scanning and ranking take O(N times (q plus r), plus N log(k plus one)), apart from output string copies. Both methods have the same asymptotic complexity and use the same data representation. Ingredient matching uses exact normalized phrases, so synonyms remain different, and ingredient quantities are not modeled. The README includes sample-data instructions, the full-data download, tests, benchmarks, and the report. This completes the feature walkthrough.

Stop by 4:30 to retain a buffer. Do not add invented personal learning or contribution claims.

## After recording

1. Check the video is legible, covers the live features, and is at most 5:00.
2. Upload to YouTube as public or unlisted; verify the link works for a viewer.
3. Put the actual URL in `docs/report_metadata.json` under `video_url`.
4. Add missing team metadata and truthful reflections, run `python3 scripts/build_report.py`, and commit the updated PDF and metadata.
5. Confirm the PDF is at most five pages and submit the repository URL in Canvas. The script itself does not satisfy the video requirement.

# Benchmark evidence

`results.csv` contains the actual November 8, 2026 Release-build measurements. `environment.json` records machine/build details, data SHA-256, command, scope, and protocol. `validation.txt` is the independent Python oracle result. `demo_output.txt` records a real full-data search. `process_memory.txt` is a separate macOS `/usr/bin/time -l` process measurement; it is not algorithm-specific memory.

```sh
./build/cooking_it_up --data data/RAW_recipes.csv --benchmark benchmarks/results.csv --repeats 21
```

The 56 rows are four accepted-recipe prefix sizes times seven queries times two methods. Each method has one warmup and 21 timed repetitions, with execution order alternating. `median_ms` is the median; `p95_ms` uses nearest rank. `load_ms` is one preprocessing observation repeated on each row for that subset, **not repeated independent load measurements**. The aggregate report table takes the arithmetic mean of the seven per-query medians, including the all-unknown query.

`top10_overlap` counts shared recipe IDs between methods. `ordered_equal=1` means identical ordered result IDs, and includes the empty result case. At full size the methods share all 10 IDs on each nonempty query; the eight-ingredient query changes the order. At smaller prefixes their selected sets sometimes differ.

`payload_bytes` counts logical recipe IDs, name bytes, ingredient IDs, vocabulary bytes, duplicated hash-key bytes, and key-associated integer values. It omits object headers, vector capacities, allocator/hash-node overhead, and the runtime. Process RSS is therefore larger. Full-data payload is 16,395,899 bytes; separately measured peak RSS is 59,621,376 bytes. Methods use the same database and heap representation, so this measurement does not establish a per-method memory advantage.

These prefixes preserve source file order. Query distribution and prefix composition are not randomized; the data are not a relevance-labeled evaluation set. Cache state and ordinary system noise affect measurements. Individual query medians and p95 values support a more useful assessment than an aggregate alone.

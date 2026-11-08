# Complexity and implementation notes

Let N be retained recipes, V vocabulary size, M total retained ingredient memberships, q unique query phrases (including unknowns), r the maximum ingredients in a recipe, k = min(requested N, retained recipes), B the largest CSV record, L the maximum ingredient/name length, and T the total CSV characters read. The result-count parameter is called k here to avoid confusion with dataset size N.

| Operation | Worst-case time | Auxiliary space |
|---|---|---|
| Streaming CSV reader | O(T) across input | O(B) |
| Ingredient normalization | O(L) per phrase | O(L) |
| Per-recipe phrase deduplication | O(r log r * L) | O(rL) |
| Recipe ID sorting | O(r log r) | O(log r) sort stack |
| Full loading | O(T + N r log r * L + M V L + N^2) including pathological hash collisions | O(N + M + VL + B), assuming bounded names; add total name bytes |
| Query preparation | O(q log q * L + q V L) with pathological hash collisions | O(qL) |
| Two-pointer overlap | O(q + r) | O(1) |
| Jaccard or cosine score once overlap is known | O(1) | O(1) |
| Full scan + bounded top-k + final sorting | O(N(q + r) + N log(k+1) + kL) | O(k) candidate heap, plus returned name bytes |
| Missing ingredients for one recipe | O(q + r + r log r * L) including alphabetical name sorting | O(rL) |

In ordinary hash-table behavior, vocabulary and duplicate-ID operations have expected constant probes; character hashing/comparison still costs O(L). Expected loading is O(T + N r log r * L), and expected query preparation is O(q log q * L). Worst-case hash collisions are explicitly included above because the assignment asks for worst-case bounds. Memory estimates include both stored vocabulary strings and duplicated hash-map keys. Transactional reloading temporarily retains both old and new databases.

Ranking is independent of hash-table performance after query normalization because stored IDs are sorted vectors. `std::priority_queue` maintains at most k lightweight candidates. Its comparator defines a total deterministic order using exact computed score then integer recipe ID. The final sort is O(k log k); this is absorbed by N log(k+1). Copying only selected names avoids copying N strings during scoring.

Display additionally builds a temporary ID-to-recipe pointer hash map for all N recipes: expected O(N + k(q+r+r log r*L)), worst-case O(N^2 + kN + k(q+r+r log r*L)), with O(N) extra pointers/nodes. This display cost is excluded from measured search time and should be distinguished from ranking's O(k) working storage. A future permanent index or inclusion of recipe indices in results could remove the repeated display map construction.

## What the scores mean

For pantry Q and recipe R, overlap is |Q intersection R|. Jaccard divides by |Q union R|. Binary cosine divides by sqrt(|Q|*|R|). Missing ingredients are R minus Q. Duplicate occurrences do not change these binary set scores. Neither score models ingredient quantity, taste, instructions, nutrition, allergies, or food safety.

A concrete ranking reversal for Q={a,b,c,d}: recipe A={a,b,y} gives Jaccard 2/5=0.4 and cosine 2/sqrt(12)=0.57735. Recipe B contains a,b,c,d plus seven other ingredients: Jaccard 4/11=0.36364 and cosine 4/sqrt(44)=0.60302. Jaccard ranks A first; cosine ranks B first. This is covered by an executable integration test.

Both methods have the same asymptotic scan and memory requirements. A square root versus a union calculation changes a small per-candidate constant; overlap scanning and heap maintenance may dominate. Timing differences are not evidence that one algorithm has a better Big O or universally better recipe recommendations.

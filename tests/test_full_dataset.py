#!/usr/bin/env python3
"""Optional real-data validation with Python's independent CSV/list/set implementations.

Usage: python3 tests/test_full_dataset.py build/cooking_it_up data/RAW_recipes.csv
Not run in CI because the copyrighted source data is downloaded separately.
"""
import ast
import csv
import math
from pathlib import Path
import re
import subprocess
import sys

exe, dataset = sys.argv[1:]
normalize = lambda s: ' '.join(s.lower().split())
recipes = []
seen = set()
vocabulary = set()
rejected = []
with Path(dataset).open(newline='', encoding='utf-8-sig') as f:
    for row in csv.DictReader(f):
        ingredients = {normalize(s) for s in ast.literal_eval(row['ingredients']) if normalize(s)}
        if not row['name'].strip() or not ingredients:
            rejected.append(row['id'])
            continue
        recipe_id = int(row['id'])
        if recipe_id in seen:
            continue
        seen.add(recipe_id)
        vocabulary.update(ingredients)
        recipes.append((recipe_id, ingredients))
queries = ['chicken, rice, salt', 'flour, sugar, eggs, butter', 'tomatoes, garlic, olive oil, onion',
           'milk', 'water, salt, pepper, onion, garlic, butter, sugar, flour',
           'chicken, unobtainium spice', 'unobtainium spice']
for text in queries:
    query = {normalize(s) for s in text.split(',') if normalize(s)}
    r = subprocess.run([exe, '--data', dataset, '--query', text, '--top', '10'], capture_output=True, text=True, check=True)
    assert f'Loaded {len(recipes)} recipes; vocabulary={len(vocabulary)};' in r.stdout
    for method in ['Jaccard', 'Cosine']:
        expected = []
        for recipe_id, ingredients in recipes:
            overlap = len(query & ingredients)
            if overlap:
                score = overlap / len(query | ingredients) if method == 'Jaccard' else overlap / math.sqrt(len(query) * len(ingredients))
                expected.append((recipe_id, score))
        expected.sort(key=lambda x: (-x[1], x[0]))
        section = r.stdout.split(method + ' results', 1)[1].split('Query time:', 1)[0]
        actual = [(int(a), float(b)) for a, b in re.findall(r'\[id=(\d+)\] \| score=([\d.]+)', section)]
        assert [x[0] for x in actual] == [x[0] for x in expected[:10]], (text, method, actual, expected[:10])
        assert all(abs(a[1]-b[1]) <= 0.00000051 for a,b in zip(actual, expected))
print(f'PASS: {len(recipes):,} cleaned recipes; {len(vocabulary):,} ingredients; rejected IDs={rejected}')
print('All 14 full-dataset top-10 rankings and scores agree with independent Python set/full-sort oracle.')

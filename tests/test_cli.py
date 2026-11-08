"""Exercise the executable as a user, including menu recovery and ranking reversal."""
import csv
import pathlib
import subprocess
import sys
import tempfile

exe, root = sys.argv[1:]
sample = str(pathlib.Path(root) / 'data/sample_recipes.csv')
def run(args=(), text=None):
    return subprocess.run([exe, '--data', sample, *args], input=text, text=True, capture_output=True)

r = run(['--query', 'CHICKEN, rice, salt', '--top', '1'])
assert r.returncode == 0 and r.stdout.count('Simple chicken rice [id=1] | score=1.000000') == 2, r
for args in (['--query', ''], ['--top', '0'], ['--top', '-1'], ['--top', '2x'], ['--method', 'bad'], ['--bogus', 'x'], ['--top']):
    assert run(args).returncode == 1, args
assert 'No recipes share' in run(['--query', 'unobtainium']).stdout
r = run(text='4\n2\n0\n3\nbad\n1\nchicken, rice, salt\n2\n1\n3\ncosine\n4\n1\nrice\n4\n0\n')
assert r.returncode == 0 and r.stdout.count('Input error:') == 3, r.stdout
assert r.stdout.count('Cosine results') == 2 and 'Jaccard results' not in r.stdout
assert run(text='').returncode == 0
with tempfile.TemporaryDirectory() as folder:
    path = pathlib.Path(folder) / 'benchmark.csv'
    r = run(['--benchmark', str(path), '--repeats', '2'])
    assert r.returncode == 0, r.stderr
    rows = list(csv.DictReader(path.open()))
    assert len(rows) == 14 and all(int(x['recipes']) == 8 for x in rows)
    # Same query: Jaccard prefers a small partial match, cosine a larger full match.
    reversal = pathlib.Path(folder) / 'reversal.csv'
    with reversal.open('w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['id', 'name', 'ingredients'])
        writer.writerow([1, 'Small partial', str(['a', 'b', 'y'])])
        writer.writerow([2, 'Large full', str(['a', 'b', 'c', 'd'] + [f'x{i}' for i in range(7)])])
    r = subprocess.run([exe, '--data', str(reversal), '--query', 'a,b,c,d', '--top', '1'], text=True, capture_output=True)
    assert r.returncode == 0
    assert 'Jaccard results' in r.stdout and '1. Small partial' in r.stdout and '1. Large full' in r.stdout, r.stdout
print('CLI integration passed: arguments, menu recovery, repeated search, benchmark CSV, ranking reversal')

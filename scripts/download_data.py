#!/usr/bin/env python3
"""Download only RAW_recipes.csv from the user-selected Kaggle dataset."""
import argparse
import hashlib
from pathlib import Path
import tempfile
import urllib.request
import zipfile

URL = 'https://www.kaggle.com/api/v1/datasets/download/shuyangli94/food-com-recipes-and-user-interactions/RAW_recipes.csv'
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=Path('data/RAW_recipes.csv'))
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    if args.output.exists():
        print(f'Already exists: {args.output}; leaving it unchanged.')
        return
    with tempfile.TemporaryDirectory() as folder:
        archive = Path(folder) / 'recipes.zip'
        print('Downloading RAW_recipes.csv from Kaggle (about 83 MiB compressed)...')
        request = urllib.request.Request(URL, headers={'User-Agent': 'CookingItUp/1.0'})
        with urllib.request.urlopen(request, timeout=120) as response, archive.open('wb') as out:
            while chunk := response.read(1024 * 1024):
                out.write(chunk)
        with zipfile.ZipFile(archive) as z:
            # Read one exact member; never extract arbitrary archive paths.
            with z.open('RAW_recipes.csv') as source, args.output.open('xb') as out:
                while chunk := source.read(1024 * 1024):
                    out.write(chunk)
    digest = hashlib.file_digest(args.output.open('rb'), 'sha256').hexdigest()
    print(f'Saved {args.output} ({args.output.stat().st_size:,} bytes)\nSHA256 {digest}')

if __name__ == '__main__':
    main()

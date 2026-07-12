#!/usr/bin/env python3
"""Fetch canonical corpus from upstream repository."""
import os
import sys
import urllib.request
import json

def fetch_file(url, output_path):
    """Fetch a file from GitHub raw URL."""
    try:
        with urllib.request.urlopen(url) as response:
            content = response.read().decode('utf-8')
            with open(output_path, 'w', encoding='utf-8') as f:
                f.write(content.rstrip())
            print(f"  Fetched: {output_path}")
            return True
    except Exception as e:
        print(f"  ERROR fetching {output_path}: {e}")
        return False

def fetch_canonical_data(upstream_repo, upstream_commit, output_dir):
    """Fetch canonical-data directory from upstream repository."""
    # GitHub API endpoint for tree
    api_url = f"https://api.github.com/repos/{upstream_repo}/git/trees/{upstream_commit}?recursive=1"
    
    headers = {
        "Accept": "application/vnd.github.v3+json",
        "User-Agent": "ASH-Pattern-System/CanonicalDataFetcher"
    }
    
    try:
        req = urllib.request.Request(api_url)
        req.add_header('Accept', 'application/vnd.github.v3+json')
        req.add_header('User-Agent', 'ASH-Pattern-System/CanonicalDataFetcher')
        with urllib.request.urlopen(req) as response:
            tree_data = json.loads(response.read().decode('utf-8'))
    except Exception as e:
        print(f"ERROR fetching tree: {e}")
        return False
    
    # Filter for canonical-data files
    canonical_files = []
    if 'tree' in tree_data:
        for item in tree_data['tree']:
            if item['path'].startswith('canonical-data/'):
                canonical_files.append(item)
    
    print(f"Found {len(canonical_files)} files in canonical-data/")
    
    # Fetch each file
    success_count = 0
    for file_info in canonical_files:
        if file_info['type'] == 'blob':
            raw_url = f"https://raw.githubusercontent.com/{upstream_repo}/{upstream_commit}/{file_info['path']}"
            output_path = os.path.join(output_dir, file_info['path'])
            
            # Create parent directory
            os.makedirs(os.path.dirname(output_path), exist_ok=True)
            
            if fetch_file(raw_url, output_path):
                success_count += 1
    
    print(f"\n========================================")
    print("Canonical Data Fetch Complete!")
    print("========================================")
    print(f"Successfully fetched {success_count} files to: {output_dir}")
    return True

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: fetch-canonical-data.py <upstream_repo> <upstream_commit>")
        print("Example: fetch-canonical-data.py flynn33/ASH-Pattern-System cc253f3d137a27f0eeb471bed62bbdb939e3b6d1")
        sys.exit(1)
    
    upstream_repo = sys.argv[1]
    upstream_commit = sys.argv[2]
    output_dir = os.path.join(os.path.dirname(__file__), '..', 'canonical-data')
    
    success = fetch_canonical_data(upstream_repo, upstream_commit, output_dir)
    sys.exit(0 if success else 1)

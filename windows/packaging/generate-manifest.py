#!/usr/bin/env python3
"""Generate release manifest for SDK package."""
import json
import os
import sys

def sha256_file(filepath):
    """Calculate SHA256 hash of a file."""
    sha256 = hashlib.sha256()
    with open(filepath, 'rb') as f:
        for chunk in iter(lambda: f.read(8192), b''):
            sha256.update(chunk)
    return sha256.hexdigest()

def generate_manifest(output_dir, manifest_file):
    """Generate release manifest."""
    checksums = {}
    files = []
    
    for root, dirs, filenames in os.walk(output_dir):
        for filename in filenames:
            filepath = os.path.join(root, filename)
            relpath = os.path.relpath(filepath, output_dir)
            
            # Calculate hash
            hash_value = sha256_file(filepath)
            checksums[relpath] = hash_value
            
            files.append({
                "name": relpath,
                "path": relpath
            })
    
    manifest = {
        "name": "ASH Pattern System Windows",
        "version": "1.0.0",
        "platform": "x64",
        "type": "SDK",
        "description": "Native C++ semantic core library for APS pattern matching",
        "releaseDate": os.popen('date +%Y-%m-%d').read().strip(),
        "buildConfiguration": "Release",
        "files": files,
        "checksums": checksums,
        "signingStatus": "unsigned",
        "signingBlockedReason": "Production signing certificate not yet provided",
        "installProcedures": {
            "sdk_zip": "Extract and register headers + library",
            "msi_installer": "Not implemented (future)",
            "msix_package": "Not implemented (requires product UI)"
        },
        "nextSteps": [
            "Review package contents",
            "Verify checksums match",
            "Test installation in target project",
            "Request production signing certificate for signed release"
        ]
    }
    
    with open(manifest_file, 'w') as f:
        json.dump(manifest, f, indent=2)
    
    print(f"Release manifest written to: {manifest_file}")

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: generate-manifest.py <output_dir> <manifest_file>")
        sys.exit(1)
    
    output_dir = sys.argv[1]
    manifest_file = sys.argv[2]
    
    import hashlib
    generate_manifest(output_dir, manifest_file)

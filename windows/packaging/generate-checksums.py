#!/usr/bin/env python3
"""Generate SHA256 checksums and SBOM for SDK package."""
import hashlib
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

def generate_checksums(output_dir, checksum_file):
    """Generate SHA256 checksums for all files in the output directory."""
    checksums = {}
    for root, dirs, files in os.walk(output_dir):
        for file in files:
            filepath = os.path.join(root, file)
            relpath = os.path.relpath(filepath, output_dir)
            hash_value = sha256_file(filepath)
            checksums[relpath] = hash_value
            print(f"{relpath}  {hash_value}")
    
    with open(checksum_file, 'w') as f:
        for path, hash_val in sorted(checksums.items()):
            f.write(f"{path}  {hash_val}\n")
    
    print(f"\nChecksums written to: {checksum_file}")
    return checksums

def generate_sbom(output_dir, sbom_file, checksums):
    """Generate Software Bill of Materials (SBOM)."""
    files = []
    for root, dirs, filenames in os.walk(output_dir):
        for filename in filenames:
            filepath = os.path.join(root, filename)
            relpath = os.path.relpath(filepath, output_dir)
            file_type = "header" if relpath.startswith("include") else "library" if relpath.endswith(".lib") else "other"
            files.append({
                "name": relpath,
                "path": relpath,
                "type": file_type
            })
    
    sbom = {
        "name": "ASH Pattern System Windows",
        "version": "1.0.0",
        "platform": "x64",
        "type": "SDK",
        "description": "Native C++ semantic core library for APS pattern matching",
        "dependencies": [],
        "files": files,
        "checksums": checksums,
        "license": "SEE LICENSE IN LICENSE",
        "buildDate": os.popen('date -u +%Y-%m-%dT%H:%M:%SZ').read().strip()
    }
    
    with open(sbom_file, 'w') as f:
        json.dump(sbom, f, indent=2)
    
    print(f"SBOM written to: {sbom_file}")

def generate_manifest(output_dir, manifest_file, checksums):
    """Generate release manifest."""
    files = []
    for root, dirs, files in os.walk(output_dir):
        for file in files:
            filepath = os.path.join(root, file)
            relpath = os.path.relpath(filepath, output_dir)
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
    if len(sys.argv) != 4:
        print("Usage: generate-checksums.py <output_dir> <checksum_file> <sbom_file>")
        sys.exit(1)
    
    output_dir = sys.argv[1]
    checksum_file = sys.argv[2]
    sbom_file = sys.argv[3]
    
    checksums = generate_checksums(output_dir, checksum_file)
    generate_sbom(output_dir, sbom_file, checksums)

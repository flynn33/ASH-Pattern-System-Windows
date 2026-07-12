#!/usr/bin/env python3
"""Create ZIP archive from staged SDK contents."""
import zipfile
import os
import sys

def create_zip(output_dir, zip_path):
    """Create a ZIP archive from the output directory."""
    if not os.path.exists(output_dir):
        print(f"ERROR: Output directory not found: {output_dir}")
        return False
    
    with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as zf:
        for root, dirs, files in os.walk(output_dir):
            for file in files:
                filepath = os.path.join(root, file)
                arcname = os.path.relpath(filepath, output_dir)
                zf.write(filepath, arcname)
    
    print(f"Created: {zip_path}")
    return True

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: create-zip.py <output_dir> <zip_path>")
        sys.exit(1)
    
    output_dir = sys.argv[1]
    zip_path = sys.argv[2]
    
    success = create_zip(output_dir, zip_path)
    sys.exit(0 if success else 1)

"""Acquire complete hash-pinned reference bytes into an external cache."""
import argparse
import json
from pathlib import Path
import sys
from canonical_resources import PIN, REPOSITORY, acquire


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('upstream_repo', choices=[REPOSITORY])
    parser.add_argument('upstream_commit', choices=[PIN])
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--source-root', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    try:
        result = acquire(args.output, args.source_root)
    except Exception as error:
        print(f'Resource acquisition failed: {error}', file=sys.stderr)
        return 1
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(f"Resource integrity passed: {result['resourceCount']} pinned files. Native behavior was not executed.")
    return 0


if __name__ == '__main__':
    sys.exit(main())

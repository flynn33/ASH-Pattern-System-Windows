"""Check offline authority integrity; native conformance is a separate executable."""
import argparse
import json
from pathlib import Path
import sys
from canonical_resources import validate_cache


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--authority-cache', required=True, type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    try:
        result = validate_cache(args.authority_cache)
    except Exception as error:
        result = {'schemaVersion': '1.0', 'status': 'fail', 'kind': 'resource-integrity',
                  'error': str(error), 'nativeBehaviorExecuted': False}
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({key: value for key, value in result.items() if key != 'corpusIdentity'}))
    return 0 if result['status'] == 'pass' else 1


if __name__ == '__main__':
    sys.exit(main())

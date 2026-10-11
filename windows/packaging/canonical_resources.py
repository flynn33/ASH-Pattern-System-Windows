"""Pinned reference acquisition and integrity; never native behavioral proof."""
from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import shutil
import subprocess
import tempfile
import urllib.request

PIN = "50d7e5ab88932a4a967ffc2c83bd18fae45b73c8"
REPOSITORY = "flynn33/ASH-Pattern-System"
LOCK = Path(__file__).with_name("canonical-resource-lock.json")


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def safe_relative(value: str) -> str:
    path = PurePosixPath(value)
    if (not value or "\\" in value or ":" in value or path.is_absolute()
            or any(part in ("", ".", "..") for part in value.split("/"))):
        raise ValueError(f"Unsafe resource path: {value!r}")
    return value


def load_lock() -> dict:
    lock = json.loads(LOCK.read_text(encoding="utf-8"))
    if lock["sourceCommit"] != PIN or lock["repository"] != REPOSITORY:
        raise ValueError("Resource lock is not bound to the selected authority")
    paths = [safe_relative(item["path"]) for item in lock["files"]]
    if not paths or len({p.casefold() for p in paths}) != len(paths):
        raise ValueError("Empty or colliding resource lock")
    return lock


def validate_cache(cache: Path) -> dict:
    cache = cache.resolve()
    lock = load_lock()
    expected = {item["path"]: item for item in lock["files"]}
    manifest_path = cache / "authority-resource-manifest.json"
    if not manifest_path.is_file() or manifest_path.is_symlink():
        raise ValueError("Missing completed authority resource manifest")
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest.get("sourceCommit") != PIN or manifest.get("resourceLockSha256") != sha256(LOCK.read_bytes()):
        raise ValueError("Authority cache pin/lock identity mismatch")
    if manifest.get("files") != lock["files"]:
        raise ValueError("Authority resource inventory mismatch")
    actual = set()
    for file in cache.rglob("*"):
        if file.is_symlink():
            raise ValueError("Symlink resource rejected")
        if file.is_file():
            actual.add(file.relative_to(cache).as_posix())
    if actual != set(expected) | {"authority-resource-manifest.json"}:
        raise ValueError("Incomplete or unexpected resource inventory")
    for relative, item in expected.items():
        data = (cache / relative).read_bytes()
        if len(data) != item["size"] or sha256(data) != item["sha256"]:
            raise ValueError(f"Resource bytes do not match canonical pin: {relative}")
        blob = hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()
        if blob != item["gitBlob"]:
            raise ValueError(f"Resource Git identity mismatch: {relative}")
    corpus = json.loads((cache / "conformance/1.0/manifest.json").read_text(encoding="utf-8"))
    return {"schemaVersion": "1.0", "status": "pass", "kind": "resource-integrity",
            "sourceCommit": PIN, "resourceCount": len(expected), "corpusIdentity": corpus,
            "nativeBehaviorExecuted": False}


def acquire(output: Path, source: Path | None = None) -> dict:
    output = output.resolve()
    product = Path(__file__).resolve().parents[2]
    if output == product or output.is_relative_to(product) or product.is_relative_to(output):
        raise ValueError("Authority resources must live outside the product checkout")
    if output.exists():
        return validate_cache(output)
    lock = load_lock()
    if source:
        source = source.resolve()
        observed = subprocess.check_output(["git", "-C", str(source), "rev-parse", "HEAD"], text=True).strip()
        if observed != PIN:
            raise ValueError("Source checkout is not at the exact authority pin")
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = Path(tempfile.mkdtemp(prefix=".authority-", dir=output.parent))
    try:
        for item in lock["files"]:
            relative = safe_relative(item["path"])
            if source:
                data = subprocess.check_output(["git", "-C", str(source), "show", f"{PIN}:{relative}"])
            else:
                request = urllib.request.Request(
                    f"https://raw.githubusercontent.com/{REPOSITORY}/{PIN}/{relative}",
                    headers={"User-Agent": "ASH-Windows-Reference-Acquisition"})
                with urllib.request.urlopen(request, timeout=60) as response:
                    data = response.read()
            if sha256(data) != item["sha256"] or len(data) != item["size"]:
                raise ValueError(f"Acquired byte integrity mismatch: {relative}")
            destination = temporary / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(data)
        manifest = {"schemaVersion": "1.0", "sourceCommit": PIN,
                    "repository": REPOSITORY, "resourceLockSha256": sha256(LOCK.read_bytes()),
                    "files": lock["files"]}
        (temporary / "authority-resource-manifest.json").write_text(
            json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
        result = validate_cache(temporary)
        os.rename(temporary, output)
        return result
    finally:
        if temporary.exists():
            if not temporary.resolve().is_relative_to(output.parent) or not temporary.name.startswith(".authority-"):
                raise ValueError("Refusing unsafe authority staging cleanup")
            shutil.rmtree(temporary)

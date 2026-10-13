#!/usr/bin/env python3
"""Extract a reference ZIP into a new directory, omitting repository detritus."""

import argparse
from pathlib import Path, PurePosixPath
import shutil
import stat
from zipfile import ZipFile


SKIP_PARTS = {".git", "__MACOSX", ".DS_Store", "__pycache__", ".entire"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("destination", type=Path)
    parser.add_argument("--root", required=True, help="Top-level directory in ZIP")
    parser.add_argument("--skip-symlinks", action="store_true", help="Omit symbolic links from reference snapshots")
    args = parser.parse_args()
    if not args.root or PurePosixPath(args.root).parts != (args.root,) or args.root in {".", ".."}:
        parser.error("--root must be a single directory name")
    if args.destination.exists() or args.destination.is_symlink():
        parser.error("destination must not already exist")

    with ZipFile(args.archive) as archive:
        members = []
        seen = set()
        for info in archive.infolist():
            path = PurePosixPath(info.filename)
            if path.is_absolute() or ".." in path.parts or "\\" in info.filename:
                parser.error(f"unsafe archive path: {info.filename}")
            if not path.parts:
                continue
            if any(p in SKIP_PARTS or p.startswith("._") for p in path.parts):
                continue
            if path.parts[0] != args.root:
                parser.error(f"unexpected archive root: {info.filename}")
            relative = Path(*path.parts[1:])
            if relative == Path("."):
                continue
            mode = info.external_attr >> 16
            filetype = stat.S_IFMT(mode)
            if filetype == stat.S_IFLNK and args.skip_symlinks:
                continue
            if filetype not in {0, stat.S_IFREG, stat.S_IFDIR}:
                parser.error(f"unsupported special file: {info.filename}")
            if relative in seen:
                parser.error(f"duplicate archive path: {info.filename}")
            seen.add(relative)
            members.append((info, relative, mode))
        if not members:
            parser.error("archive contains no usable entries")

        args.destination.mkdir(parents=True)
        count = 0
        try:
            for info, relative, mode in members:
                target = args.destination / relative
                if info.is_dir():
                    target.mkdir(parents=True, exist_ok=True)
                    continue
                target.parent.mkdir(parents=True, exist_ok=True)
                with archive.open(info) as source, target.open("xb") as output:
                    shutil.copyfileobj(source, output)
                target.chmod(0o755 if mode & 0o111 else 0o644)
                count += 1
        except BaseException:
            shutil.rmtree(args.destination)
            raise
    print(f"Imported {count} files into {args.destination}")


if __name__ == "__main__":
    main()

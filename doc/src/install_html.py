#!/usr/bin/env python3

# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

"A Meson install script to install a Sphinx HTML build"

import argparse
import gzip
import os
import shutil
import tempfile
import typing
from pathlib import Path


def tree_filter(root: Path):
    "Create a filter for Sphinx HTML content, suitable for shutil.copytree"

    def ignore(visiting: str, names: typing.List[str]) -> typing.List[str]:
        # Never copy hidden directories
        result = [name for name in names if name.startswith(".")]

        # Certain directories contain the special files, copy these verbatim
        relpath = Path(visiting).relative_to(root)
        if relpath.parts and relpath.parts[0] in ("_static", "_images", "_sources"):
            return result

        for name in names:
            if (Path(visiting) / name).is_dir():
                continue  # Always iterate down directories
            if name.endswith(".html"):
                continue  # Always include HTML files
            if (
                len(relpath.parts) == 0
                and name.endswith(".pdf")
                or name.endswith(".epub")
            ):
                continue  # Keep EPUB and PDF outputs
            # If the above cases all fail, ignore this file.
            result.append(name)

        return result

    return ignore


def maybe_compress_gzip(
    src: Path, dst: Path, *, quiet: bool = False, max_ratio: float = 0.95
) -> None:
    "Compress the given file using gzip, if the size decreases as a result"
    with tempfile.SpooledTemporaryFile(1024 * 1024) as temp:
        with gzip.GzipFile(filename=src.name, fileobj=temp, mode="w") as tempgzip:
            with src.open("rb") as f:
                original_size = os.fstat(f.fileno()).st_size
                shutil.copyfileobj(f, tempgzip)
        ratio = temp.tell() / original_size
        if ratio > max_ratio:
            # Didn't compress enough, just skip
            return
        if not quiet:
            print(f"Installing {src} (compressed to {ratio*100:.1f}%) -> {dst}")
        temp.seek(0)
        with dst.open("wb") as f:
            shutil.copyfileobj(temp, f)


def copier(*, quiet: bool = False, dry_run: bool = True):
    "Create a copy function that can print and/or dry-run the install process"

    def copy(src, dst, **kwargs):
        src, dst = Path(src), Path(dst)
        if not quiet:
            print(f"Installing {src} to {dst}")
        if not dry_run:
            shutil.copy2(src, dst, **kwargs)
            maybe_compress_gzip(src, dst.with_suffix(dst.suffix + ".gz"), quiet=quiet)

    return copy


def main() -> None:
    "Main function"
    parser = argparse.ArgumentParser()
    parser.add_argument("index_html", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()

    if not args.index_html.exists():
        print(f"File '{args.index_html}' not found, skipping HTML site install")
        return

    root = args.index_html.parent
    prefix = Path(os.environ["MESON_INSTALL_DESTDIR_PREFIX"]) / args.destination
    quiet = bool(os.environ.get("MESON_INSTALL_QUIET"))
    dry_run = bool(os.environ.get("MESON_INSTALL_DRY_RUN"))

    shutil.copytree(
        root,
        prefix,
        symlinks=True,
        ignore=tree_filter(root),
        copy_function=copier(quiet=quiet, dry_run=dry_run),
        dirs_exist_ok=True,
    )


if __name__ == "__main__":
    main()

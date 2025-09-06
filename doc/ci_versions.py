#!/usr/bin/env python3

# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

# pylint: disable=fixme

"Script to generate the main documentation site's version.json"

import argparse
import contextlib
import json
import os
import re
import typing
import urllib.request
from pathlib import Path

if typing.TYPE_CHECKING:
    import collections.abc

API_URL = os.environ["CI_API_V4_URL"]
PROJECT_ID = int(os.environ["CI_PROJECT_ID"])


class VersionTuple(typing.NamedTuple):
    "Decoded version, suitable for sorting"
    major: int
    minor: int
    patch: int
    prerelease: str


class Version:
    "Available version of the documentation"

    def __init__(
        self,
        version: str,
        url: str,
        *,
        name: str | None = None,
        preferred: bool = False,
    ) -> None:
        "Create a Version from field settings"
        self._version = version
        self._url = url
        self._name = name if name is not None else version
        self._preferred = preferred

    @classmethod
    def from_json(cls, data: dict) -> "typing.Self":
        "Create a Version by parsing the given data blob parsed from JSON"
        version = data.get("version")
        if not isinstance(version, str) or not version:
            raise ValueError(
                f"Bad Version data, 'version' must be a non-empty string: {data!r}"
            )

        url = data.get("url")
        if not isinstance(url, str) or not url:
            raise ValueError(
                f"Bad Version data, 'url' must be a non-empty string: {data!r}"
            )

        name = version
        if "name" in data:
            name = data["name"]
            if not isinstance(name, str) or not name:
                raise ValueError(
                    f"Bad Version data, 'name' must be a non-empty string: {data!r}"
                )

        preferred = data.get("preferred", False)
        if not isinstance(preferred, bool):
            raise ValueError(
                f"Bad Version data, 'preferred' must be a boolean: {data!r}"
            )

        return cls(version=version, url=url, name=name, preferred=preferred)

    @classmethod
    def from_file(cls, path: Path | os.PathLike) -> "typing.Self":
        "Create a Version from the given JSON file"
        with Path(path).open("rb") as f:
            return cls.from_json(json.load(f))

    @property
    def preferred(self) -> bool:
        "If True, this Version is preferred over others and users will be directed to it"
        return self._preferred

    @preferred.setter
    def preferred(self, value: bool) -> None:
        "Set whether this Version is preferred over others"
        self._preferred = value

    @property
    def name(self) -> str:
        "The display name of this Version"
        return self._name

    @name.setter
    def name(self, value: str) -> None:
        "Set the display name for this Version"
        self._name = value

    @property
    def json(self) -> dict:
        "Get this Version as a suitable JSON-able blob"
        result: "dict[str, str | bool]" = {"version": self._version, "url": self._url}
        if self._name != self._version:
            result["name"] = self._name
        if self.preferred:
            result["preferred"] = True
        return result

    @property
    def version_tuple(self) -> VersionTuple:
        "Decode the version to get a VersionTuple"
        match = re.fullmatch(r"(\d+)\.(\d+)(?:\.(\d+))?(?:-(.*))?", self._version)
        if match is None:
            raise ValueError(f"Bad version string: {self._version}")
        return VersionTuple(
            int(match[1]), int(match[2]), (int(match[3]) if match[3] else 0), match[4]
        )

    def __str__(self) -> str:
        "Pretty printing for Versions"
        extras = []
        if self._version != self._name:
            extras.append(self._version)
        if self.preferred:
            extras.append("preferred")
        extras_str = " (" + ", ".join(extras) + ")" if extras else ""
        return f'"{self._name}"{extras_str} -> {self._url}'


PDF_ARCHIVE_URL_FORMAT = "https://hpctoolkit.gitlab.io/hpctoolkit-manual-archive/{}.pdf"
ARCHIVED_VERSIONS: "list[Version]" = [
    # FIXME: We currently don't host older versions of the manual, although they should
    # be available in our Git history. Below are the versions that use PDF manuals.
    # Version("2024.01.1", PDF_ARCHIVE_URL_FORMAT.format("2024.01.1")),
    # Version("2023.08.0", PDF_ARCHIVE_URL_FORMAT.format("2023.08.0")),
    # Version("2022.10.01", PDF_ARCHIVE_URL_FORMAT.format("release-2022.10.01")),
    # Version("2022.05.15", PDF_ARCHIVE_URL_FORMAT.format("release-2022.05.15")),
    # Version("2022.04.15", PDF_ARCHIVE_URL_FORMAT.format("release-2022.04")),
    # Version("2021.05.15", PDF_ARCHIVE_URL_FORMAT.format("release-2021.05")),
    # Version("2020.08", PDF_ARCHIVE_URL_FORMAT.format("release-2020.08")),
    # Version("2018.09", PDF_ARCHIVE_URL_FORMAT.format("release-2018.09")),
    # Version("2017.10", PDF_ARCHIVE_URL_FORMAT.format("release-2017.10")),
    # Version("2017.06", PDF_ARCHIVE_URL_FORMAT.format("release-2017.06")),
    # Version("2016.12", PDF_ARCHIVE_URL_FORMAT.format("release-2016.12")),
]


@contextlib.contextmanager
def gitlab_urlopen(url: str):
    "Open the given GitLab URL, accounting for authentication"
    request = urllib.request.Request(url)
    if "CI_JOB_TOKEN" in os.environ:
        request.add_header("JOB-TOKEN", os.environ["CI_JOB_TOKEN"])
    with urllib.request.urlopen(request) as response:
        yield response


def collect_released_versions(root_url: str) -> "collections.abc.Generator[Version]":
    "Scrape released Versions of the documentation from the hosting site"
    with gitlab_urlopen(API_URL + f"/projects/{PROJECT_ID}/releases") as response:
        raw = json.load(response)
    for candidate in raw:
        version = None
        try:
            with gitlab_urlopen(
                root_url + f"/{candidate['tag_name']}/_version.json"
            ) as response:
                version = Version.from_json(json.load(response))
        except urllib.error.URLError as err:
            print(
                f"Failed to fetch deployment for {candidate['tag_name']} ({candidate['name']}), skipping: {err.reason}"
            )
            continue
        yield version


def main():
    "Main function"
    parser = argparse.ArgumentParser()
    parser.add_argument("root_url", help="Root URL where the versioned docs are hosted")
    parser.add_argument(
        "version_json", type=Path, help="version.json for the current deployment"
    )
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    versions = sorted(
        [
            Version.from_file(args.version_json),
            *collect_released_versions(args.root_url),
            *ARCHIVED_VERSIONS,
        ],
        key=lambda v: v.version_tuple,
        reverse=True,
    )

    # Mark the latest non-prerelease Version as the one and only preferred version
    for v in versions:
        v.preferred = False
    for v in versions:
        if not v.version_tuple.prerelease:
            v.preferred = True
            v.name = v.name + " (stable)"
            break

    # Final report and output
    print("Found the following versions:")
    for v in versions:
        print(f"    {v}")
    with args.output.open("w", encoding="utf-8") as f:
        json.dump([v.json for v in versions], f, separators=(",", ":"))


if __name__ == "__main__":
    main()

#!/usr/bin/env python3

# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

# pylint: disable=missing-module-docstring

import argparse
import collections.abc
import datetime
import enum
import json
import os
import re
import typing
import urllib.parse
import urllib.request


class GitLabDiscount(enum.StrEnum):
    "Discount that may be applied to GitLab compute minutes"

    NONE = "none"
    GITLAB_FOR_OSS = "oss"
    GITLAB_FOR_OSS_FORK = "oss-fork"

    def apply(self, cost_factor: float | int) -> float | int:
        "Apply this discount to the given cost factor, and return the resulting factor"
        match self:
            case self.NONE:
                return cost_factor
            case self.GITLAB_FOR_OSS:
                return 0.5
            case self.GITLAB_FOR_OSS_FORK:
                return 0.008
        raise ValueError(self)


class Cost(collections.abc.Mapping[str, float]):
    "Class representing CI job cost in various known units"

    # Cost factors for GitLab-hosted runners. Taken from the table at:
    # https://docs.gitlab.com/ci/pipelines/compute_minutes/#cost-factors-of-hosted-runners-for-gitlabcom
    _GITLAB_SAAS_COST_FACTOR: dict[str, int] = {
        "saas-linux-small-amd64": 1,
        "saas-linux-medium-amd64": 2,
        "saas-linux-large-amd64": 3,
        "saas-linux-xlarge-amd64": 6,
        "saas-linux-2xlarge-amd64": 12,
        "saas-linux-medium-amd64-gpu-standard": 7,
        "saas-linux-small-arm64": 1,
        "saas-linux-medium-arm64": 2,
        "saas-linux-large-arm64": 3,
        "saas-macos-medium-m1": 6,
        "saas-macos-large-m2pro": 12,
        "saas-windows-medium-amd64": 1,
    }

    @classmethod
    def _factors_for(
        cls, tags: frozenset[str], *, gitlab_discount: GitLabDiscount
    ) -> tuple[dict[str, float | int], datetime.timedelta]:
        "Derive the cost factors and time unit for the given tag set"
        if not tags:
            return {"untagged": 1}, datetime.timedelta(minutes=1)
        if len(tags) == 1:
            tag = next(iter(tags))
            if tag.startswith("saas-"):
                return {
                    "gitlab": gitlab_discount.apply(cls._GITLAB_SAAS_COST_FACTOR[tag])
                }, datetime.timedelta(minutes=1)
            if tag.startswith("rice-"):
                return {"project": 1}, datetime.timedelta(minutes=1)
        if any(tag.startswith("hpsf-") for tag in tags):
            return {"hpsf": 1}, datetime.timedelta(minutes=1)
        raise ValueError(f"Unrecognized tag set: {tags}")

    def __init__(
        self,
        tags: collections.abc.Iterable[str],
        time: datetime.timedelta = datetime.timedelta(),
        *,
        gitlab_discount: GitLabDiscount = GitLabDiscount.NONE,
    ) -> None:
        "Initialize a new Cost for the given CI tags and job time"
        self._tags = frozenset(tags)
        self._factors, self._unit = self._factors_for(
            self._tags, gitlab_discount=gitlab_discount
        )
        self._time = time

    @property
    def tags(self) -> frozenset[str]:
        "Get the CI tags for this Cost"
        return self._tags

    @property
    def total(self) -> float:
        "Get the total cost this Cost represents"
        return sum(self.values())

    def __add__(self, other: typing.Self) -> typing.Self:
        "Add two Costs together"
        if self._tags != other._tags:
            raise ValueError(
                f"Invalid addition between incompatible costs: {self._tags} vs. {other._tags}"
            )
        return self.__class__(self._tags, self._time + other._time)

    def __iadd__(self, other: typing.Self) -> typing.Self:
        "Add more time to a Cost"
        if self._tags != other._tags:
            raise ValueError(
                f"Invalid addition between incompatible costs: {self._tags} vs. {other._tags}"
            )
        self._time += other._time
        return self

    def __getitem__(self, unit: str) -> float:
        "Get the specific cost by unit"
        return self._factors.get(unit, 0) * (self._time / self._unit)

    def __len__(self) -> int:
        "Get the number of units this Cost has non-zero cost for"
        return len(self._factors)

    def __iter__(self) -> collections.abc.Iterator[str]:
        "Iterate over all the units this Cost has non-zero cost for"
        return iter(self._factors)


class Job:
    "GitLab CI job, usually in a larger CI pipeline"

    def __init__(self, raw: dict) -> None:
        "Create a new Job from the given raw JSON-decoded GitLab API call"
        self._raw = raw
        # NB: Job status values are from: https://docs.gitlab.com/api/jobs/#job-status-values
        if self._raw["status"] not in (
            "canceled",
            "canceling",
            "created",
            "failed",
            "manual",
            "pending",
            "preparing",
            "running",
            "scheduled",
            "skipped",
            "success",
            "waiting_for_resource",
        ):
            raise ValueError(f"Invalid job state: {self._raw['status']}")

    @property
    def id(self) -> int:
        "GitLab global ID for the Job"
        return int(self._raw["id"])

    @property
    def passed(self) -> bool:
        "True if the job passed successfully"
        return self._raw["status"] == "success"

    @property
    def failed(self) -> bool:
        "True if the job failed with an error"
        return self._raw["status"] == "failed"

    @property
    def canceled(self) -> bool:
        "True if the job was canceled before it could finish"
        return self._raw["status"] in ("canceling", "canceled", "skipped")

    @property
    def incomplete(self) -> bool:
        "True if the job has not yet completed"
        return self._raw["status"] in (
            "created",
            "manual",
            "pending",
            "preparing",
            "running",
            "scheduled",
            "waiting_for_resource",
        )

    def cost(self, **kwargs) -> Cost:
        "Calculate the Cost associated with this Job"
        if self.incomplete:
            raise ValueError("Cost cannot be calculated for an incomplete Job")
        start = datetime.datetime.fromisoformat(self._raw["started_at"])
        end = datetime.datetime.fromisoformat(self._raw["finished_at"])
        if end < start:
            raise RuntimeError(
                f"Invalid start/end timestamps on GitLab job: {end} < {start}"
            )
        return Cost(self._raw["tag_list"], end - start, **kwargs)

    # pylint: disable=too-many-arguments
    @classmethod
    def from_pipeline(
        cls,
        instance_url: urllib.parse.ParseResult,
        project_id: str | int,
        pipeline_id: int,
        *,
        include_retried: bool = False,
        authentication: tuple[str, str] | None = None,
    ) -> list[typing.Self]:
        "Fetch all the Jobs for a GitLab pipeline"
        headers = {}
        if authentication is not None:
            headers[authentication[0]] = authentication[1]

        result = []
        url: str | None = urllib.parse.urlunparse(
            instance_url._replace(
                path=f"/api/v4/projects/{urllib.parse.quote_plus(str(project_id))}/pipelines/{pipeline_id:d}/jobs?include_retried={'true' if include_retried else 'false'}"
            )
        )
        while url is not None:
            with urllib.request.urlopen(
                urllib.request.Request(url, headers=headers)
            ) as response:
                for raw_job in json.load(response):
                    if not raw_job["tag_list"]:
                        match instance_url.hostname:
                            case "gitlab.com":
                                raw_job["tag_list"] = ["saas-linux-small-amd64"]
                    result.append(cls(raw_job))
                match = re.search(
                    r'<([^>]+)>;\s*rel="next"', response.getheader("Link", "")
                )
                url = match[1] if match is not None else None
        return result


class CostTable:
    "Table of Costs that can be printed prettily"

    _COLUMN_NAMES = {
        "project": "Project Minutes",
        "gitlab": "GitLab Compute Minutes",
        "hpsf": "HPSF CI Minutes",
    }

    def __init__(self, initial_costs: collections.abc.Iterable[Cost] = tuple()) -> None:
        "Initialize a Table for Costs from the given iterable"
        self._data: dict[frozenset[str], Cost] = {}
        for cost in initial_costs:
            self.add(cost)

    def add(self, cost: Cost) -> None:
        "Add more Cost to this Table"
        if cost.tags in self._data:
            self._data[cost.tags] += cost
        else:
            self._data[cost.tags] = cost

    def __str__(self) -> str:
        "Stringify this Table in a nice markdown-ish format"

        def tags_str(tags: collections.abc.Iterable[str]) -> str:
            return "[" + ", ".join(sorted(tags)) + "]"

        columns = {
            unit: name
            for unit, name in self._COLUMN_NAMES.items()
            if any(cost[unit] > 0 for cost in self._data.values())
        }
        headers = list(columns.values()) + ["Tags"]
        rows = []
        for cost in sorted(self._data.values(), key=lambda c: c.total, reverse=True):
            rows.append([f"{cost[key]:.2f}" for key in columns] + [tags_str(cost.tags)])
        rows.append([""] * len(headers))
        rows.append(
            [f"{sum(cost[key] for cost in self._data.values()):.2f}" for key in columns]
            + ["Totals"]
        )

        col_widths = [
            max(len(row[column]) for row in rows + [headers])
            for column in range(len(headers))
        ]
        col_alignments = [">"] * (len(headers) - 1) + ["<"]

        def pad(content: str, width: int, alignment: str = "") -> str:
            form = f"{{:{alignment:s}{width:d}s}}"
            return form.format(content)

        def pad_row(
            row: collections.abc.Iterable[str],
        ) -> collections.abc.Generator[str]:
            for cell, width, alignment in zip(row, col_widths, col_alignments):
                yield pad(cell, width, alignment)

        def md_header(width: int, alignment: str = "c") -> str:
            left = ":" if alignment == "<" else " "
            right = ":" if alignment == ">" else " "
            return left + "-" * (width - 2) + right

        lines = []
        lines.append("| " + " | ".join(pad_row(headers)) + " |")
        lines.append(
            "|"
            + "|".join(
                md_header(width + 2, alignment)
                for width, alignment in zip(col_widths, col_alignments)
            )
            + "|"
        )
        for row in rows:
            lines.append("| " + " | ".join(pad_row(row)) + " |")
        return "\n".join(lines)


def main():
    "Main function"

    def instance_or_pipeline_url(
        raw_url: str,
    ) -> urllib.parse.ParseResult | tuple[urllib.parse.ParseResult, str, int]:
        "Parse a GitLab instance or pipeline URL"
        url = urllib.parse.urlparse(raw_url)
        if not all(part == "" for part in [url.params, url.query, url.fragment]):
            raise ValueError(
                "URL must not include parameter, query, or fragment components"
            )
        if url.path != "":
            decoded = re.fullmatch(r"/(.+)/-/pipelines/(\d+)", url.path)
            if decoded is None:
                raise ValueError(f"URL does not refer to a GitLab pipeline: {raw_url}")
            return (url._replace(path=""), decoded[1], int(decoded[2]))
        return url

    parser = argparse.ArgumentParser(
        description="Analyze the given GitLab CI pipeline and produce a performance report"
    )
    parser.add_argument("--ci-token", help="GitLab CI token to use for authentication")
    parser.add_argument(
        "--gitlab-discount",
        type=GitLabDiscount,
        choices=GitLabDiscount,
        default=GitLabDiscount.NONE,
        help="GitLab compute discount to apply",
    )
    parser.add_argument(
        "--exclude",
        type=int,
        help="Exclude a job by its id, can be repeated",
        action="append",
    )
    parser.add_argument(
        "url",
        type=instance_or_pipeline_url,
        help="URL of the pipeline or GitLab instance",
    )
    parser.add_argument("project_id", type=int, nargs="?", help="GitLab project ID")
    parser.add_argument("pipeline_id", type=int, nargs="?", help="GitLab pipeline ID")
    args = parser.parse_args()
    if not isinstance(args.url, urllib.parse.ParseResult):
        args.url, args.project_id, args.pipeline_id = args.url

    if not args.ci_token:
        if token := os.environ.get("CI_JOB_TOKEN"):
            args.ci_token = token

    auth: tuple[str, str] | None = None
    if args.ci_token is not None:
        auth = ("JOB-TOKEN", args.ci_token)
    jobs = Job.from_pipeline(
        args.url, args.project_id, args.pipeline_id, authentication=auth
    )
    if args.exclude is not None:
        jobs = [job for job in jobs if job.id not in args.exclude]

    if any(job.incomplete for job in jobs):
        print(
            "WARNING: Some jobs have not yet completed. These jobs will be ignored, which may cause inaccuracies in the report below."
        )
        print()

    print("SUCCESSFUL JOBS COST BREAKDOWN:")
    print(
        CostTable(
            job.cost(gitlab_discount=args.gitlab_discount) for job in jobs if job.passed
        )
    )


if __name__ == "__main__":
    main()

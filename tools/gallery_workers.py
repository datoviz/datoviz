#!/usr/bin/env python3
"""Bounded deterministic worker helpers for gallery media tools."""

from __future__ import annotations

import concurrent.futures
import os
from collections.abc import Callable
from typing import TypeVar, cast


MAX_AUTO_JOBS = 4

T = TypeVar("T")
R = TypeVar("R")


def parse_jobs(value: str) -> int:
    """Parse a worker count with an automatic CPU-bounded default."""
    if value == "auto":
        return max(1, min(MAX_AUTO_JOBS, os.cpu_count() or 1))
    try:
        jobs = int(value)
    except ValueError as exc:
        raise ValueError(f"invalid worker count: {value!r}") from exc
    if jobs < 1:
        raise ValueError("worker count must be at least 1")
    return jobs


def bounded_parallel_map(
    items: list[T],
    worker: Callable[[T], R],
    jobs: int,
    label: Callable[[T], str] = str,
) -> list[R]:
    """Run bounded work, preserve input order, and stop scheduling after failure."""
    if jobs < 1:
        raise ValueError("jobs must be at least 1")
    if jobs == 1 or len(items) <= 1:
        results = []
        for item in items:
            try:
                results.append(worker(item))
            except Exception as exc:
                raise RuntimeError(
                    f"gallery media worker failed: {label(item)}: {exc}"
                ) from exc
        return results

    results: list[R | None] = [None] * len(items)
    failures: list[tuple[int, Exception]] = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as executor:
        next_index = 0
        active: dict[concurrent.futures.Future[R], int] = {}

        def submit_one() -> None:
            nonlocal next_index
            if next_index >= len(items):
                return
            index = next_index
            next_index += 1
            active[executor.submit(worker, items[index])] = index

        for _ in range(min(jobs, len(items))):
            submit_one()

        while active and not failures:
            done, _ = concurrent.futures.wait(
                active, return_when=concurrent.futures.FIRST_COMPLETED
            )
            for future in sorted(done, key=active.__getitem__):
                index = active.pop(future)
                try:
                    results[index] = future.result()
                except Exception as exc:
                    failures.append((index, exc))
            if failures:
                for future in active:
                    future.cancel()
                concurrent.futures.wait(active)
                for future, index in active.items():
                    if future.cancelled():
                        continue
                    try:
                        results[index] = future.result()
                    except Exception as exc:
                        failures.append((index, exc))
                break
            for _ in range(len(done)):
                submit_one()

    if failures:
        details = "; ".join(
            f"{label(items[index])}: {exc}" for index, exc in sorted(failures)
        )
        raise RuntimeError(f"gallery media workers failed: {details}")
    if any(result is None for result in results):
        raise RuntimeError("gallery media workers returned an incomplete output set")
    return [cast(R, result) for result in results]

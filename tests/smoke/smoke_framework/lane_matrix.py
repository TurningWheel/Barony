from __future__ import annotations


def compute_lane_result(child_result: str, *checks: bool) -> str:
    if child_result != "pass":
        return "fail"
    for check in checks:
        if not check:
            return "fail"
    return "pass"


def update_lane_counts(
    lane_result: str,
    *,
    total_lanes: int,
    pass_lanes: int,
    fail_lanes: int,
) -> tuple[int, int, int]:
    total_lanes += 1
    if lane_result == "pass":
        pass_lanes += 1
    else:
        fail_lanes += 1
    return total_lanes, pass_lanes, fail_lanes


def single_lane_counts(lane_result: str) -> tuple[int, int]:
    if lane_result == "pass":
        return 1, 0
    return 0, 1

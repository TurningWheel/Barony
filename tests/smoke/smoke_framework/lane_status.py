from __future__ import annotations


def pass_fail(is_pass: bool) -> str:
    return "pass" if is_pass else "fail"


def result_from_failures(failures: int) -> str:
    return pass_fail(failures == 0)

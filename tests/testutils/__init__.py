from dataclasses import dataclass
from typing import List, Optional


@dataclass
class Args:
    verbose: bool = False
    no_standalone: bool = False
    no_clang_tidy: bool = False
    no_fixits: bool = False
    only_standalone: bool = False
    dump_ast: bool = False
    qt_versions: List[int] = None
    exclude: Optional[str] = None
    jobs: int = 14
    check_names: List[str] = None
    cxx_args: str = ""
    qt_namespaced: bool = False
    clang_version: int = -1

    def __post_init__(self):
        # avoid mutable default pitfalls
        if self.qt_versions is None:
            self.qt_versions = [5, 6]
        if self.check_names is None:
            self.check_names = []
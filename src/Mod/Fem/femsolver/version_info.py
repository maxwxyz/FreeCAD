# **************************************************************************
# *   Copyright (c) 2026 FreeCAD Developers                               *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This library is free software; you can redistribute it and/or         *
# *   modify it under the terms of the GNU Library General Public           *
# *   License as published by the Free Software Foundation; either          *
# *   version 2 of the License, or (at your option) any later version.      *
# *                                                                         *
# *   This library is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this library; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307   *
# *   USA                                                                   *
# **************************************************************************

"""Collect versions of installed FEM solvers for the About information."""

import re
import subprocess

from femsolver import settings

_SOLVERS = (
    ("CalculiX", "Calculix", ("-v",)),
    ("ElmerSolver", "ElmerSolver", ("-v",)),
    ("MYSTRAN", "Mystran", ()),
)


def _extract_version(output):
    match = re.search(r"\bversion\s*:?\s*([^\s,]+)", output, re.IGNORECASE)
    return match.group(1) if match else ""


def get_solver_versions():
    """Return a summary of versions reported by the available FEM solvers."""
    versions = []
    for label, solver, arguments in _SOLVERS:
        binary = settings.get_binary(solver, silent=True)
        if not binary:
            continue
        try:
            process = subprocess.run(
                [binary, *arguments],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                timeout=2,
                check=False,
            )
        except (OSError, subprocess.TimeoutExpired):
            continue
        version = _extract_version(process.stdout)
        if version:
            versions.append(f"{label} {version}")
    return ", ".join(versions)

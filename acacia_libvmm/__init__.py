# Copyright 2026, UNSW
# SPDX-License-Identifier: BSD-2-Clause

import sys
from pathlib import Path

_SDDF_ROOT = Path(__file__).resolve().parents[1] / "dep" / "sddf"
if str(_SDDF_ROOT) not in sys.path:
    sys.path.insert(0, str(_SDDF_ROOT))

from acacia_sddf.blk import sDDFBlk
from acacia_sddf.board import BOARDS, Board
from acacia_sddf.driver_manifest import (
    sDDFDriverConfig,
    sDDFDriverManifest,
    register_sddf_subsystem,
)
from acacia_sddf.net import sDDFEthernet
from acacia_sddf.sddf import sDDFDriverClass
from acacia_sddf.serial import sDDFSerial
from acacia_sddf.timer import sDDFTimer

from .config import GuestMemoryKind
from .vmm import LibvmmVirtMachine

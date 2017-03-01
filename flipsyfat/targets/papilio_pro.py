#!/usr/bin/env python3

import argparse

from migen import *
from flipsyfat.cores.sd_emulator import SDEmulator
from misoc.targets.papilio_pro import BaseSoC
from migen.build.generic_platform import *
from misoc.integration.soc_sdram import *
from misoc.integration.builder import *

io = [
    ("sdemu", 0,
        Subsignal("clk", Pins("C:8")),
        Subsignal("cmd", Pins("C:9")),
        Subsignal("d", Pins("C:10 C:11 C:12 C:13")),
        IOStandard("LVCMOS33")
    )
]

class Flipsyfat(BaseSoC):
    def __init__(self, **kwargs):
        BaseSoC.__init__(self, **kwargs)
        self.platform.add_extension(io)
        self.submodules.sdemu = SDEmulator(self.platform, self.platform.request("sdemu"))
        self.csr_devices += ["sdemu"]


def main():
    parser = argparse.ArgumentParser(description="Flipsyfat port to the Papilio Pro")
    builder_args(parser)
    soc_sdram_args(parser)
    args = parser.parse_args()

    soc = Flipsyfat(**soc_sdram_argdict(args))
    builder = Builder(soc, **builder_argdict(args))
    builder.build()


if __name__ == "__main__":
    main()

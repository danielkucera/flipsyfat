#!/usr/bin/env python3

import argparse

from migen import *
from flipsyfat.cores.sd_emulator import SDEmulator
from flipsyfat.cores.sd_trigger import SDTrigger
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
    ),
    ("trigger", 0,
        Pins("C:0 C:1 C:2 C:3 C:4 C:5 C:6 C:7"),
        IOStandard("LVCMOS33")
    ),
    ("debug", 0,
        Pins("C:14 C:15"),
        IOStandard("LVCMOS33")
    ),
]


class Flipsyfat(BaseSoC):
    mem_map = {
        "sdemu": 0x30000000,
    }
    mem_map.update(BaseSoC.mem_map)

    def __init__(self, **kwargs):
        BaseSoC.__init__(self, **kwargs)
        self.platform.add_extension(io)

        self.submodules.sdemu = SDEmulator(self.platform, self.platform.request("sdemu"))
        self.register_mem("sdemu", self.mem_map["sdemu"], self.sdemu.bus, self.sdemu.mem_size)
        self.csr_devices += ["sdemu"]
        self.interrupt_devices += ["sdemu"]

        self.submodules.sdtrig = SDTrigger(self.sdemu.ll, self.platform.request("trigger"))
        self.csr_devices += ["sdtrig"]

        # Activity LED
        self.io_activity = (self.sdemu.ll.block_read_act | self.sdemu.ll.block_write_act )
        self.sync += self.platform.request("user_led").eq(self.io_activity)
 
        debug = self.platform.request("debug")
        self.comb += debug[0].eq(self.sdemu.ll.block_read_act)
        self.comb += debug[1].eq(self.sdemu.ll.spi_cs)


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

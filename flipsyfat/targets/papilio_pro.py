#!/usr/bin/env python3

import argparse

from migen import *
from flipsyfat.cores.sd_emulator import SDEmulator
from flipsyfat.cores.sd_trigger import SDTrigger
from flipsyfat.cores.sd_timer import SDTimer
from flipsyfat.cores.clock import ClockOutput
from misoc.targets.papilio_pro import BaseSoC
from misoc.cores.gpio import GPIOTristate
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
    ("gpio", 0,
        Pins("A:0 A:1 A:2 A:3 A:4 A:5 A:6 A:7 " +
             "A:8 A:9 A:10 A:11 A:12 A:13 A:14 A:15"),
        IOStandard("LVCMOS33")
    ),
    ("debug", 0,
        Pins("B:12 B:13 B:14 B:15"),
        IOStandard("LVCMOS33")
    ),
    ("clkout", 0,
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
        BaseSoC.__init__(self, uart_baudrate=500000, **kwargs)
        self.platform.add_extension(io)

        self.submodules.sdemu = SDEmulator(self.platform, self.platform.request("sdemu"))
        self.register_mem("sdemu", self.mem_map["sdemu"], self.sdemu.mem_size, self.sdemu.bus)
        self.csr_devices += ["sdemu"]
        self.interrupt_devices += ["sdemu"]

        self.submodules.sdtimer = SDTimer(self.sdemu.ll)
        self.csr_devices += ["sdtimer"]

        self.submodules.sdtrig = SDTrigger(self.sdemu.ll, self.platform.request("trigger"))
        self.csr_devices += ["sdtrig"]

        self.submodules.gpio = GPIOTristate(self.platform.request("gpio"))
        self.csr_devices += ["gpio"]

        self.submodules.clkout = ClockOutput(self.platform.request("clkout"))
        self.csr_devices += ["clkout"]

        # Activity LED
        self.io_activity = self.sdemu.ll.block_read_act | self.sdemu.ll.block_write_act
        self.sync += self.platform.request("user_led").eq(self.io_activity)

        # Debug signals
        self.comb += self.platform.request("debug").eq(Cat(
            self.sdemu.ll.block_read_act,
            self.sdemu.ll.data_out_done,
            self.sdtimer._capture.re,
        ))


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

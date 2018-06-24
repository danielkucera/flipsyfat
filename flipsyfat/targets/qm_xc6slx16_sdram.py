#!/usr/bin/env python3

import argparse

from migen import *
from flipsyfat.cores.sd_emulator import SDEmulator
from flipsyfat.cores.sd_trigger import SDTrigger
from flipsyfat.cores.sd_timer import SDTimer
from flipsyfat.cores.clock import ClockOutput
from misoc.cores.gpio import GPIOTristate
from migen.build.generic_platform import *
from misoc.integration.soc_sdram import *
from misoc.integration.builder import *

from migen.build.generic_platform import *
from migen.build.xilinx import XilinxPlatform
from migen.build.xilinx.programmer import XC3SProg


_io = [
    ("led1", 0, Pins("T9"), IOStandard("LVCMOS33"), Drive(24), Misc("SLEW=QUIETIO")),

    ("led3", 0, Pins("R9"), IOStandard("LVCMOS33"), Drive(24), Misc("SLEW=QUIETIO")),

    ("clk32", 0, Pins("A10"), IOStandard("LVCMOS33")), #?? sys_clk?

    ("serial", 0,
        Subsignal("tx", Pins("P12"), IOStandard("LVCMOS33"), Misc("SLEW=SLOW")),
        Subsignal("rx", Pins("M11"), IOStandard("LVCMOS33"), Misc("PULLUP"))
    ),

    ("spiflash", 0,
        Subsignal("cs_n", Pins("T3")),
        Subsignal("clk", Pins("R11")),
        Subsignal("mosi", Pins("T10")),
        Subsignal("miso", Pins("P10"), Misc("PULLUP")),
        IOStandard("LVCMOS33"), Misc("SLEW=FAST")
    ),
    ("spiflash2x", 0,
        Subsignal("cs_n", Pins("T3")),
        Subsignal("clk", Pins("R11")),
        Subsignal("dq", Pins("T10", "P10")),
        IOStandard("LVCMOS33"), Misc("SLEW=FAST")
    ),

    ("sdram_clock", 0, Pins("H1"), IOStandard("LVCMOS33"), Misc("SLEW=FAST")),
    ("sdram", 0,
        Subsignal("a", Pins("L4 M3 M4 N3 R2 R1 P2 P1 N1 M1 L3 L1 K1")),
        Subsignal("ba", Pins("K3 K2")),
        Subsignal("cs_n", Pins("J3")),
        Subsignal("cke", Pins("J1")),
        Subsignal("ras_n", Pins("J4")),
        Subsignal("cas_n", Pins("H3")),
        Subsignal("we_n", Pins("G3")),
        Subsignal("dq", Pins("A3 A2 B3 B2 C3 C2 D3 E3 G1 F1 F2 E1 E2 D1 C1 B1")),
        Subsignal("dm", Pins("F3 H2")),  #????
        IOStandard("LVCMOS33"), Misc("SLEW=FAST")
    )
]

_connectors = [
    ("U7", "- - - - - - E12 E13 B15 B16 C15 C16 D14 D16 E15 E16 F15 F16 G11 F12 " +
    "F14 F13 G16 G13 H15 H16 G12 H11 H13 H14 J14 J16 J11 J12 K14 J13 K15 K16 " +
    "L16 L14 K11 K12 M15 M16 N14 N16 M13 M14 L12 L13 P15 P16 R15 R16 R14 T15 " +
    "T13 T14 T12 R12"),
    ("U8", "- - - - - - A14 B14 C13 A13 B12 A12 C11 A11 B10 A9 C9 A8 B8 A7 C7 A6 " + #22 next start
     "B6 A5 B5 A5 E10 C10 E11 F10 F9 D9 C8 D8 E7 E6 F7 C6 D6 M6 P4 N6 P5 N6 M7 " +
     "P6 N8 L7 P9 T4 T5 R5 T6 T7 N9 M9 M10 P11 P12 M11"), #P12 M11 used as serial
]


class Platform(XilinxPlatform):
    default_clk_name = "clk32"
    default_clk_period = 31.25

    def __init__(self):
        XilinxPlatform.__init__(self, "xc6slx16-ftg256", _io, _connectors)

    def create_programmer(self):
        return XC3SProg("papilio", "bscan_spi_lx9_papilio.bit")

import argparse
from fractions import Fraction

from migen import *
from migen.genlib.resetsync import AsyncResetSynchronizer
#from migen.build.platforms import papilio_pro

from misoc.cores.sdram_settings import MT48LC4M16
from misoc.cores.sdram_phy import GENSDRPHY
from misoc.cores import spi_flash
from misoc.integration.soc_sdram import *
from misoc.integration.builder import *


class _CRG(Module):
    def __init__(self, platform, clk_freq):
        self.clock_domains.cd_sys = ClockDomain()
        self.clock_domains.cd_sys_ps = ClockDomain()

        f0 = 32*1000000
        clk32 = platform.request("clk32")
        clk32a = Signal()
        self.specials += Instance("IBUFG", i_I=clk32, o_O=clk32a)
        clk32b = Signal()
        self.specials += Instance("BUFIO2", p_DIVIDE=1,
                                  p_DIVIDE_BYPASS="TRUE", p_I_INVERT="FALSE",
                                  i_I=clk32a, o_DIVCLK=clk32b)
        f = Fraction(int(clk_freq), int(f0))
        n, m, p = f.denominator, f.numerator, 8
        assert f0/n*m == clk_freq
        pll_lckd = Signal()
        pll_fb = Signal()
        pll = Signal(6)
        self.specials.pll = Instance("PLL_ADV", p_SIM_DEVICE="SPARTAN6",
                                     p_BANDWIDTH="OPTIMIZED", p_COMPENSATION="INTERNAL",
                                     p_REF_JITTER=.01, p_CLK_FEEDBACK="CLKFBOUT",
                                     i_DADDR=0, i_DCLK=0, i_DEN=0, i_DI=0, i_DWE=0, i_RST=0, i_REL=0,
                                     p_DIVCLK_DIVIDE=1, p_CLKFBOUT_MULT=m*p//n, p_CLKFBOUT_PHASE=0.,
                                     i_CLKIN1=clk32b, i_CLKIN2=0, i_CLKINSEL=1,
                                     p_CLKIN1_PERIOD=1000000000/f0, p_CLKIN2_PERIOD=0.,
                                     i_CLKFBIN=pll_fb, o_CLKFBOUT=pll_fb, o_LOCKED=pll_lckd,
                                     o_CLKOUT0=pll[0], p_CLKOUT0_DUTY_CYCLE=.5,
                                     o_CLKOUT1=pll[1], p_CLKOUT1_DUTY_CYCLE=.5,
                                     o_CLKOUT2=pll[2], p_CLKOUT2_DUTY_CYCLE=.5,
                                     o_CLKOUT3=pll[3], p_CLKOUT3_DUTY_CYCLE=.5,
                                     o_CLKOUT4=pll[4], p_CLKOUT4_DUTY_CYCLE=.5,
                                     o_CLKOUT5=pll[5], p_CLKOUT5_DUTY_CYCLE=.5,
                                     p_CLKOUT0_PHASE=0., p_CLKOUT0_DIVIDE=p//1,
                                     p_CLKOUT1_PHASE=0., p_CLKOUT1_DIVIDE=p//1,
                                     p_CLKOUT2_PHASE=0., p_CLKOUT2_DIVIDE=p//1,
                                     p_CLKOUT3_PHASE=0., p_CLKOUT3_DIVIDE=p//1,
                                     p_CLKOUT4_PHASE=0., p_CLKOUT4_DIVIDE=p//1,  # sys
                                     p_CLKOUT5_PHASE=270., p_CLKOUT5_DIVIDE=p//1,  # sys_ps
        )
        self.specials += Instance("BUFG", i_I=pll[4], o_O=self.cd_sys.clk)
        self.specials += Instance("BUFG", i_I=pll[5], o_O=self.cd_sys_ps.clk)
        self.specials += AsyncResetSynchronizer(self.cd_sys, ~pll_lckd)

        self.specials += Instance("ODDR2", p_DDR_ALIGNMENT="NONE",
                                  p_INIT=0, p_SRTYPE="SYNC",
                                  i_D0=0, i_D1=1, i_S=0, i_R=0, i_CE=1,
                                  i_C0=self.cd_sys.clk, i_C1=~self.cd_sys.clk,
                                  o_Q=platform.request("sdram_clock"))


class BaseSoC(SoCSDRAM):
    def __init__(self, **kwargs):
        platform = Platform()
        clk_freq = 80*1000000
        SoCSDRAM.__init__(self, platform, clk_freq,
                          cpu_reset_address=0x60000,
                          **kwargs)

        self.submodules.crg = _CRG(platform, clk_freq)

        self.submodules.sdrphy = GENSDRPHY(platform.request("sdram"))
        sdram_module = MT48LC4M16(clk_freq, "1:1")
        self.register_sdram(self.sdrphy, "minicon",
                            sdram_module.geom_settings, sdram_module.timing_settings)

        if not self.integrated_rom_size:
            self.submodules.spiflash = spi_flash.SpiFlash(platform.request("spiflash2x"),
                                                          dummy=4, div=6)
            self.flash_boot_address = 0x70000
            self.register_rom(self.spiflash.bus, 0x1000000)
            self.csr_devices.append("spiflash")


io = [
    ("sdemu", 0,
        Subsignal("clk", Pins("U8:27")),
        Subsignal("cmd", Pins("U7:9")),
        Subsignal("d", Pins("U7:10 U7:11 U7:12 U7:13")),
        IOStandard("LVCMOS33")
    ),
    ("trigger", 0,
        Pins("U7:16 U7:17 U7:18 U7:19 U7:20 U7:21 U7:22 U7:24"),
        IOStandard("LVCMOS33")
    ),
    ("gpio", 0,
        Pins("U8:6 U8:7 U8:8 U8:9 U8:10 U8:11 U8:12 U8:13 U8:14 U8:15 " +
            "U8:16 U8:17 U8:18 U8:19 U8:20 U8:21"),
        IOStandard("LVCMOS33")
    ),
    ("debug", 0,
        Pins("U8:28 U8:29 U8:30 U8:31"),
        IOStandard("LVCMOS33")
    ),
    ("clkout", 0,
        Pins("U8:24 U8:25"),
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
        self.sync += self.platform.request("led1").eq(self.io_activity)

        # Debug signals
        self.comb += self.platform.request("debug").eq(Cat(
            self.sdemu.ll.block_read_act,
            self.sdemu.ll.data_out_done,
            self.sdtimer._capture.re,
        ))


def main():
    parser = argparse.ArgumentParser(description="Flipsyfat port to the QM_XC6SLX16_SDRAM Core Board")
    builder_args(parser)
    soc_sdram_args(parser)
    args = parser.parse_args()

    soc = Flipsyfat(**soc_sdram_argdict(args))
    builder = Builder(soc, **builder_argdict(args))
    builder.build()


if __name__ == "__main__":
    main()

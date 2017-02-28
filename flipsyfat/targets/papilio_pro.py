#!/usr/bin/env python3

import argparse

from migen import *
from flipsyfat.cores.sd_emulator import SDEmulator
from misoc.targets.papilio_pro import BaseSoC
from misoc.integration.soc_sdram import *
from misoc.integration.builder import *


class PapilioWing:
    def __init__(self, platform, wing):
        if len(wing) == 1:
            width = 16
            first = 0
        elif len(wing) == 2 and wing.endswith('L'):
            # Low half
            width = 8
            first = 0
        elif len(wing) == 2 and wing.endswith('H'):
            width = 8
            first = 8
        else:
            raise ValueError('Badly formed Wing name')
        b = ['%s:%d' % (wing[0], first+i) for i in range(width)]
        self.io = platform.constraint_manager.connector_manager.resolve_identifiers(b)


class SDEmulatorWing(PapilioWing):
    def __init__(self, platform, wing):
        PapilioWing.__init__(self, platform, wing)
        self.clk = self.io[0]
        self.cmd = self.io[1]
        self.dat = self.io[2:6]


class Flipsyfat(BaseSoC):
    def __init__(self, **kwargs):
        BaseSoC.__init__(self, **kwargs)
        self.submodules.sd_emulator = SDEmulator(SDEmulatorWing(self.platform, 'CL'))


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

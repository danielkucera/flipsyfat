from migen import *
from misoc.interconnect.csr import *


class ClockOutput(Module, AutoCSR):
    def __init__(self, pins, width=8):
        self.pins = Array(pins)
        self._div = CSRStorage(width)
        cnt = Signal(width)
        toggle = Signal()
        self.comb += [p.eq(toggle) for p in self.pins]
        dv = self._div.storage
        self.sync += [
            If(toggle | ~(dv == 0),
                If(cnt == 0,
                    toggle.eq(~toggle),
                    cnt.eq(dv - 1))
                .Else(cnt.eq(cnt - 1))
            )
        ]

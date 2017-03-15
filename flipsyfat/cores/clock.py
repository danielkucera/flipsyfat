from migen import *
from misoc.interconnect.csr import *


class ClockOutput(Module, AutoCSR):
    def __init__(self, pin, width=8):
        self._div = CSRStorage(width)
        cnt = Signal(width)
        toggle = Signal()
        self.comb += pin.eq(toggle)
        dv = self._div.storage
        self.sync += [
            If(toggle | ~(dv == 0),
                If(cnt == 0,
                    toggle.eq(~toggle),
                    cnt.eq(dv - 1))
                .Else(cnt.eq(cnt - 1))
            )
        ]

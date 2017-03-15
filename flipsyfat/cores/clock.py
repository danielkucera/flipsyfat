from migen import *
from misoc.interconnect.csr import *


class ClockOutput(Module, AutoCSR):
    def __init__(self, pin, width=8):
        self._enable = CSRStorage()
        self._div = CSRStorage(width)

        cnt = Signal(width)
        toggle = Signal()
        self.comb += pin.eq(toggle)

        self.sync += If(self._enable.storage | toggle,
                        If(cnt == 0,
                            toggle.eq(~toggle),
                            cnt.eq(self._div.storage))
                        .Else(cnt.eq(cnt - 1)))

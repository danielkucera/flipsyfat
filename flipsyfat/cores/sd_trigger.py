from migen import *
from migen.genlib.cdc import MultiReg
from misoc.interconnect.csr import *
from migen.fhdl.decorators import ClockDomainsRenamer


class SDTriggerOutputDriver(Module, AutoCSR):
    def __init__(self, trig_out, latch_in, oe_in, pulse_cycles=20):

        oe_prev = Signal()
        counter = Signal(log2_int(pulse_cycles, need_pow2=False))

        self.sync += [
            oe_prev.eq(oe_in),
            If(oe_prev & ~oe_in,
                counter.eq(pulse_cycles),
                trig_out.eq(latch_in)
            ).Elif(counter == 0,
                trig_out.eq(0)
            ).Else(
                counter.eq(counter - 1)
            )
        ]


class SDTrigger(Module, AutoCSR):
    """Add-on core for generating trigger signals timed in sync with
       the SDEmulator's data output completion.
        """

    def __init__(self, sd_linklayer, pins):
        self._latch = CSRStorage(len(pins))

        self.clock_domains.cd_sd = ClockDomain(reset_less=True)
        self.comb += self.cd_sd.clk.eq(sd_linklayer.cd_sd.clk)

        sdcd_latch = Signal(len(pins))
        self.specials += MultiReg(self._latch.storage, sdcd_latch, odomain="sd", n=3)

        # Output circuit itself is entirely in SD clock domain
        self.submodules.drv = ClockDomainsRenamer("sd")(
           SDTriggerOutputDriver(pins, sdcd_latch, sd_linklayer.dat_t.oe))

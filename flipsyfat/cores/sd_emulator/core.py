from migen import *
from misoc.interconnect.csr import *


class SDEmulator(Module, AutoCSR):
	def __init__(self, slot):
		counter = Signal(16)
		self.slot = slot
		self._foo = CSRStatus(len(counter))
		self.sync += counter.eq(counter + slot.clk)


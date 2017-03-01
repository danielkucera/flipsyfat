from migen import *
from misoc.interconnect.csr import *


class SDEmulator(Module, AutoCSR):
	def __init__(self, slot):
		counter = Signal(8)
		self.slot = slot
		self._foo = CSRStatus(len(counter))
		self.sync += counter.eq(counter + 1)
		self.comb += self._foo.status.eq(counter)

from migen import *
from misoc.interconnect.csr import *


class SDEmulator(Module, AutoCSR):
	def __init__(self, slot):
		self.slot = slot


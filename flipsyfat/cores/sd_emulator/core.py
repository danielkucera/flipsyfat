from flipsyfat.cores.sd_emulator.linklayer import SDLinkLayer

from migen import *
from misoc.interconnect.csr import *
from misoc.interconnect.csr_eventmanager import *
from misoc.interconnect import wishbone

class SDEmulator(Module, AutoCSR):
    """Core for emulating SD card memory a block at a time,
       with reads and writes backed by software. 
       """

    # Read and write buffers, each a single 512 byte block
    mem_size = 1024

    def __init__(self, platform, pads, **kwargs):
        self.submodules.ll = SDLinkLayer(platform, pads, **kwargs)

        self.submodules.ev = EventManager()
        self.ev.read = EventSourceLevel()
        self.ev.write = EventSourceLevel()
        self.ev.finalize()
        self.comb += [
            self.ev.read.trigger.eq(self.ll.block_read_act),
            self.ev.write.trigger.eq(self.ll.block_write_act)
        ]

        # Wishbone access to SRAM buffers
        self.bus = wishbone.Interface()
        self.submodules.wb_rd_buffer = wishbone.SRAM(self.ll.rd_buffer, read_only=False)
        self.submodules.wb_wr_buffer = wishbone.SRAM(self.ll.wr_buffer, read_only=False)
        wb_slaves = [
            (lambda a: a[9] == 0, self.wb_rd_buffer.bus),
            (lambda a: a[9] == 1, self.wb_wr_buffer.bus)
        ]
        self.submodules.wb_decoder = wishbone.Decoder(self.bus, wb_slaves, register=True)

        # Direct access to "manager" interface for block flow control
        self._read_act = CSRStatus()
        self._read_addr = CSRStatus(32)
        self._read_num = CSRStatus(32)
        self._read_stop = CSRStatus()
        self._read_go = CSR()

        self._write_act = CSRStatus()
        self._write_addr = CSRStatus(32)
        self._write_num = CSRStatus(32)
        self._write_done = CSR()

        self._preerase_num = CSRStatus(23)
        self._erase_start = CSRStatus(32)
        self._erase_end = CSRStatus(32)

        self.comb += [
            self._read_act.status.eq(self.ll.block_read_act),
            self._read_addr.status.eq(self.ll.block_read_addr),
            self._read_num.status.eq(self.ll.block_read_num),
            self._read_stop.status.eq(self.ll.block_read_stop),
            self._write_act.status.eq(self.ll.block_write_act),
            self._write_addr.status.eq(self.ll.block_write_addr),
            self._write_num.status.eq(self.ll.block_write_num),
            self._preerase_num.status.eq(self.ll.block_preerase_num),
            self._erase_start.status.eq(self.ll.block_erase_start),
            self._erase_end.status.eq(self.ll.block_erase_end),
            self.ll.block_read_go.eq(self._read_go.re),
            self.ll.block_write_done.eq(self._write_done.re)]

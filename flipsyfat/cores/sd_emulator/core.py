import os

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


class SDLinkLayer(Module):
    """This is a Migen wrapper around the lower-level parts of the SD card emulator
       from Google Project Vault's Open Reference Platform. This core still does all
       SD card command processing in hardware, presenting a RAM buffered interface
       for single 512 byte blocks.
       """
    block_size = 512
   
    def  __init__(self, platform, pads, enable_hs=True):
        self.pads = pads        

        # Verilog sources from ProjectVault ORP
        platform.add_source_dir(os.path.join(os.path.abspath(os.path.dirname(__file__)), "verilog"))

        # Adapt PHY tristate style
        self.cmd_t = TSTriple()
        self.dat_t = TSTriple(4)
        sd_cmd_t = Signal()
        sd_dat_t = Signal(4)
        self.specials += self.cmd_t.get_tristate(pads.cmd)
        self.specials += self.dat_t.get_tristate(pads.d)
        self.comb += self.cmd_t.oe.eq(~sd_cmd_t)
        self.comb += self.dat_t.oe.eq(~sd_dat_t)

        # The external SD clock drives a separate clock domain
        self.clock_domains.cd_sd = ClockDomain(reset_less=True)
        self.comb += self.cd_sd.clk.eq(pads.clk)

        self.specials.rd_buffer = Memory(32, self.block_size//4)
        self.specials.wr_buffer = Memory(32, self.block_size//4)
        self.specials.internal_rd_port = self.rd_buffer.get_port(clock_domain="sd")
        self.specials.internal_wr_port = self.wr_buffer.get_port(write_capable=True, clock_domain="sd")

        # Communication between PHY and Link layers
        self.card_state = Signal(4)
        self.mode_4bit = Signal()
        self.mode_spi = Signal()
        self.cmd_in = Signal(48)
        self.cmd_in_cmd = Signal(6)
        self.cmd_in_last = Signal(6)
        self.cmd_in_crc_good = Signal()
        self.cmd_in_act = Signal()
        self.spi_cs = Signal()
        self.data_in_act = Signal()
        self.data_in_busy = Signal()
        self.data_in_another = Signal()
        self.data_in_stop = Signal()
        self.data_in_done = Signal()
        self.data_in_crc_good = Signal()
        self.resp_out = Signal(136)
        self.resp_type = Signal(4)
        self.resp_busy = Signal()
        self.resp_act = Signal()
        self.resp_done = Signal()
        self.data_out_reg = Signal(512)
        self.data_out_src = Signal()
        self.data_out_len = Signal(10)
        self.data_out_busy = Signal()
        self.data_out_act = Signal()
        self.data_out_stop = Signal()
        self.data_out_done = Signal()

        # Status outputs
        self.info_card_desel = Signal()
        self.err_op_out_range = Signal()
        self.err_unhandled_cmd = Signal()
        self.err_cmd_crc = Signal()

        # Debug signals
        self.phy_odc = Signal(11)
        self.phy_ostate = Signal(7)

        # I/O request outputs
        self.block_read_act = Signal()
        self.block_read_addr = Signal(32)
        self.block_read_num = Signal(32)
        self.block_read_stop = Signal()
        self.block_write_act = Signal()
        self.block_write_addr = Signal(32)
        self.block_write_num = Signal(32)
        self.block_preerase_num = Signal(23)
        self.block_erase_start = Signal(32)
        self.block_erase_end = Signal(32)

        # I/O completion inputs
        self.block_read_go = Signal()
        self.block_write_done = Signal()

        self.specials += Instance("sd_phy",
            i_clk_50 = ClockSignal(),
            i_reset_n = ~ResetSignal(),
            i_sd_clk = ClockSignal("sd"),
            i_sd_cmd_i = self.cmd_t.i,
            o_sd_cmd_o = self.cmd_t.o,
            o_sd_cmd_t = sd_cmd_t,
            i_sd_dat_i = self.dat_t.i,
            o_sd_dat_o = self.dat_t.o,
            o_sd_dat_t = sd_dat_t,
            i_card_state = self.card_state,
            o_cmd_in = self.cmd_in,
            o_cmd_in_crc_good = self.cmd_in_crc_good,
            o_cmd_in_act = self.cmd_in_act,
            o_spi_cs = self.spi_cs,
            i_data_in_act = self.data_in_act,
            o_data_in_busy = self.data_in_busy,
            i_data_in_another = self.data_in_another,
            i_data_in_stop = self.data_in_stop,
            o_data_in_done = self.data_in_done,
            o_data_in_crc_good = self.data_in_crc_good,
            i_resp_out = self.resp_out,
            i_resp_type = self.resp_type,
            i_resp_busy = self.resp_busy,
            i_resp_act = self.resp_act,
            o_resp_done = self.resp_done,
            i_mode_4bit = self.mode_4bit,
            o_mode_spi = self.mode_spi,
            i_data_out_reg = self.data_out_reg,
            i_data_out_src = self.data_out_src,
            i_data_out_len = self.data_out_len,
            o_data_out_busy = self.data_out_busy,
            i_data_out_act = self.data_out_act,
            i_data_out_stop = self.data_out_stop,
            o_data_out_done = self.data_out_done,
            o_bram_rd_sd_addr = self.internal_rd_port.adr,
            i_bram_rd_sd_q = self.internal_rd_port.dat_r,
            o_bram_wr_sd_addr = self.internal_wr_port.adr,
            o_bram_wr_sd_wren = self.internal_wr_port.we,
            o_bram_wr_sd_data = self.internal_wr_port.dat_w,
            i_bram_wr_sd_q = self.internal_wr_port.dat_r,
            o_odc = self.phy_odc,
            o_ostate = self.phy_ostate
        )

        self.specials += Instance("sd_link",
            i_clk_50 = ClockSignal(),
            i_reset_n = ~ResetSignal(),
            o_link_card_state = self.card_state,
            i_phy_cmd_in = self.cmd_in,
            i_phy_cmd_in_crc_good = self.cmd_in_crc_good,
            i_phy_cmd_in_act = self.cmd_in_act,
            i_phy_spi_cs = self.spi_cs,
            o_phy_data_in_act = self.data_in_act,
            i_phy_data_in_busy = self.data_in_busy,
            o_phy_data_in_stop = self.data_in_stop,
            o_phy_data_in_another = self.data_in_another,
            i_phy_data_in_done = self.data_in_done,
            i_phy_data_in_crc_good = self.data_in_crc_good,
            o_phy_resp_out = self.resp_out,
            o_phy_resp_type = self.resp_type,
            o_phy_resp_busy = self.resp_busy,
            o_phy_resp_act = self.resp_act,
            i_phy_resp_done = self.resp_done,
            o_phy_mode_4bit = self.mode_4bit,
            o_phy_mode_spi = self.mode_spi,
            o_phy_data_out_reg = self.data_out_reg,
            o_phy_data_out_src = self.data_out_src,
            o_phy_data_out_len = self.data_out_len,
            i_phy_data_out_busy = self.data_out_busy,
            o_phy_data_out_act = self.data_out_act,
            o_phy_data_out_stop = self.data_out_stop,
            i_phy_data_out_done = self.data_out_done,
            o_block_read_act = self.block_read_act,
            i_block_read_go = self.block_read_go,
            o_block_read_addr = self.block_read_addr,
            o_block_read_num = self.block_read_num,
            o_block_read_stop = self.block_read_stop,
            o_block_write_act = self.block_write_act,
            i_block_write_done = self.block_write_done,
            o_block_write_addr = self.block_write_addr,
            o_block_write_num = self.block_write_num,
            o_block_preerase_num = self.block_preerase_num,
            o_block_erase_start = self.block_erase_start,
            o_block_erase_end = self.block_erase_end,
            i_opt_enable_hs = Constant(enable_hs),
            o_cmd_in_last = self.cmd_in_last,
            o_info_card_desel = self.info_card_desel,
            o_err_unhandled_cmd = self.err_unhandled_cmd,
            o_err_cmd_crc = self.err_cmd_crc,
            o_cmd_in_cmd = self.cmd_in_cmd
        ) 

import os

from migen import *
from misoc.interconnect.csr import *


class SDEmulator(Module, AutoCSR):
    """This is a Migen wrapper around the lower-level parts of the SD card emulator
       from Google Project Vault's Open Reference Platform. This core still does all
       SD card command processing in hardware, presenting a RAM buffered interface
       for single 512 byte blocks.
       """

    def  __init__(self, platform, pads, enable_hs=True):

        self.pads = pads        
        self.cmd_t = TSTriple()
        self.specials += self.cmd_t.get_tristate(pads.cmd)
        self.dat_t = TSTriple(4)
        self.specials += self.dat_t.get_tristate(pads.d)

        self.card_state = Signal(4)
        self.mode_4bit = Signal()
        self.phy_odc = Signal(11)
        self.phy_ostate = Signal(7)

        self.cmd_in = Signal(48)
        self.cmd_in_cmd = Signal(6)
        self.cmd_in_last = Signal(6)
        self.cmd_in_crc_good = Signal()
        self.cmd_in_act = Signal()

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

        self.bram_rd_sd_clk = Signal()
        self.bram_rd_sd_addr = Signal(7)
        self.bram_rd_sd_wren = Signal()
        self.bram_rd_sd_data = Signal(32)
        self.bram_rd_sd_q = Signal(32)

        self.bram_wr_sd_clk = Signal()
        self.bram_wr_sd_addr = Signal(7)
        self.bram_wr_sd_wren = Signal()
        self.bram_wr_sd_data = Signal(32)
        self.bram_wr_sd_q = Signal(32)

        self.bram_rd_ext_clk = Signal()
        self.bram_rd_ext_addr = Signal(7)
        self.bram_rd_ext_wren = Signal()
        self.bram_rd_ext_data = Signal(32)
        self.bram_rd_ext_q = Signal(32)

        self.bram_wr_ext_clk = Signal()
        self.bram_wr_ext_addr = Signal(7)
        self.bram_wr_ext_wren = Signal()
        self.bram_wr_ext_data = Signal(32)
        self.bram_wr_ext_q = Signal(32)

        self.block_read_act = Signal()
        self.block_read_go = Signal()
        self.block_read_addr = Signal(32)
        self.block_read_num = Signal(32)
        self.block_read_stop = Signal()

        self.block_write_act = Signal()
        self.block_write_done = Signal()
        self.block_write_addr = Signal(32)
        self.block_write_num = Signal(32)
        self.block_preerase_num = Signal(23)
        self.block_erase_start = Signal(32)
        self.block_erase_end = Signal(32)

        self.info_card_desel = Signal()
        self.err_host_is_spi = Signal()
        self.err_op_out_range = Signal()
        self.err_unhandled_cmd = Signal()
        self.err_cmd_crc = Signal()

        self.specials += Instance("sd_bram_block_dp",
            i_a_clk = self.bram_rd_sd_clk,
            i_a_wr = self.bram_rd_sd_wren,
            i_a_addr = self.bram_rd_sd_addr,
            i_a_din = self.bram_rd_sd_data,
            o_a_dout = self.bram_rd_sd_q,
            i_b_clk = self.bram_rd_ext_clk,
            i_b_wr = self.bram_rd_ext_wren,
            i_b_addr = self.bram_rd_ext_addr,
            i_b_din = self.bram_rd_ext_data,
            o_b_dout = self.bram_rd_ext_q,
        )

        self.specials += Instance("sd_bram_block_dp",
            i_a_clk = self.bram_wr_sd_clk,
            i_a_wr = self.bram_wr_sd_wren,
            i_a_addr = self.bram_wr_sd_addr,
            i_a_din = self.bram_wr_sd_data,
            o_a_dout = self.bram_wr_sd_q,
            i_b_clk = self.bram_wr_ext_clk,
            i_b_wr = self.bram_wr_ext_wren,
            i_b_addr = self.bram_wr_ext_addr,
            i_b_din = self.bram_wr_ext_data,
            o_b_dout = self.bram_wr_ext_q,
        )

        self.specials += Instance("sd_phy",
            i_clk_50 = ClockSignal(),
            i_reset_n = ResetSignal(),
            i_sd_clk = self.pads.clk,
            i_sd_cmd_i = self.cmd_t.i,
            o_sd_cmd_o = self.cmd_t.o,
            o_sd_cmd_t = self.cmd_t.oe,
            i_sd_dat_i = self.dat_t.i,
            o_sd_dat_o = self.dat_t.o,
            o_sd_dat_t = self.dat_t.oe,
            i_card_state = self.card_state,
            o_cmd_in = self.cmd_in,
            o_cmd_in_crc_good = self.cmd_in_crc_good,
            o_cmd_in_act = self.cmd_in_act,
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
            i_data_out_reg = self.data_out_reg,
            i_data_out_src = self.data_out_src,
            i_data_out_len = self.data_out_len,
            o_data_out_busy = self.data_out_busy,
            i_data_out_act = self.data_out_act,
            i_data_out_stop = self.data_out_stop,
            o_data_out_done = self.data_out_done,
            o_bram_rd_sd_clk = self.bram_rd_sd_clk,
            o_bram_rd_sd_addr = self.bram_rd_sd_addr,
            o_bram_rd_sd_wren = self.bram_rd_sd_wren,
            o_bram_rd_sd_data = self.bram_rd_sd_data,
            i_bram_rd_sd_q = self.bram_rd_sd_q,
            o_bram_wr_sd_clk = self.bram_wr_sd_clk,
            o_bram_wr_sd_addr = self.bram_wr_sd_addr,
            o_bram_wr_sd_wren = self.bram_wr_sd_wren,
            o_bram_wr_sd_data = self.bram_wr_sd_data,
            i_bram_wr_sd_q = self.bram_wr_sd_q,
            o_odc = self.phy_odc,
            o_ostate = self.phy_ostate
        )

        self.specials += Instance("sd_link",
            i_clk_50 = ClockSignal(),
            i_reset_n = ResetSignal(),
            o_link_card_state = self.card_state,
            i_phy_cmd_in = self.cmd_in,
            i_phy_cmd_in_crc_good = self.cmd_in_crc_good,
            i_phy_cmd_in_act = self.cmd_in_act,
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
            o_err_host_is_spi = self.err_host_is_spi,
            o_err_unhandled_cmd = self.err_unhandled_cmd,
            o_err_cmd_crc = self.err_cmd_crc,
            o_cmd_in_cmd = self.cmd_in_cmd
        ) 

        # Verilog sources from ProjectVault ORP
        platform.add_source_dir(os.path.join(os.path.abspath(os.path.dirname(__file__)), "verilog"))

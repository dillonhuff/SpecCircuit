//
//--------------------------------------------------------------------------------
//          THIS FILE WAS AUTOMATICALLY GENERATED BY THE GENESIS2 ENGINE        
//  FOR MORE INFORMATION: OFER SHACHAM (CHIP GENESIS INC / STANFORD VLSI GROUP)
//    !! THIS VERSION OF GENESIS2 IS NOT FOR ANY COMMERCIAL USE !!
//     FOR COMMERCIAL LICENSE CONTACT SHACHAM@ALUMNI.STANFORD.EDU
//--------------------------------------------------------------------------------
//
//  
//	-----------------------------------------------
//	|            Genesis Release Info             |
//	|  $Change: 11904 $ --- $Date: 2013/08/03 $   |
//	-----------------------------------------------
//	
//
//  Source file: /horowitz/users/dhuff/CGRAGenerator/hardware/generator_z/memory_core/memory_core.vp
//  Source template: memory_core
//
// --------------- Begin Pre-Generation Parameters Status Report ---------------
//
//	From 'generate' statement (priority=5):
// Parameter ddepth 	= 1024
// Parameter dwidth 	= 16
//
//		---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
//	From Command Line input (priority=4):
//
//		---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
//	From XML input (priority=3):
//
//		---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//
//	From Config File input (priority=2):
//
// ---------------- End Pre-Generation Pramameters Status Report ----------------

///////////////////////////////////////////////////////////////////
// CGRA memory generator
//////////////////////////////////////////////////////////////////


`define xassert(condition, message) if(condition) begin $display(message); $finish(1); end


// dwidth (_GENESIS2_INHERITANCE_PRIORITY_) = 16
//
// ddepth (_GENESIS2_INHERITANCE_PRIORITY_) = 0x400
//


module memory_core_unq1(
clk_in,
clk_en,
reset,
config_addr,
config_data,
config_read,
config_write,
config_en,
config_en_sram,
config_en_linebuf,
data_in,
data_out,
wen_in,
ren_in,
valid_out,
chain_in,
chain_out,
chain_wen_in,
chain_valid_out,
almost_full,
almost_empty,
addr_in,
read_data,
read_data_sram,
read_data_linebuf,
flush
);

input clk_in;
input clk_en;
input reset;
input config_en;
input [3:0] config_en_sram;
input config_en_linebuf;
input wen_in;
input ren_in;
input chain_wen_in;
input [31:0] config_addr;
input [31:0] config_data;
input config_read;
input config_write;
input [15:0] data_in;
input [15:0] chain_in;
input [15:0] addr_in;
input flush;
output reg [31:0] read_data;
output reg [31:0] read_data_sram;
output [31:0] read_data_linebuf;
output chain_valid_out;
output [15:0] chain_out;
output reg valid_out;
output reg almost_full;
output reg almost_empty;
output reg [15:0] data_out;

wire  [1:0] mode;
wire  tile_en;
wire [12:0] depth;
wire  [3:0] almost_count;
wire        enable_chain;

wire gclk, gclk_in, gclk_sram;
wire wen_in_int;
wire mem_cen0;
wire mem_cen1;
reg mem_cen0_int;
reg mem_cen1_int;

wire lb_valid_out;
wire fifo_valid_out;
wire lb_almost_full;
wire fifo_almost_full;
wire lb_almost_empty;
wire fifo_almost_empty;
wire lb_wen;
wire lb_ren;
wire fifo_wen;
wire fifo_ren;

wire clk;
wire [15:0] data_in_int;
wire [15:0] mem_data_out0;
wire [15:0] mem_data_out1;

wire [15:0] lb_addr;
wire [15:0] fifo_addr;
wire [31:0] lb_mem_data_out;
wire [31:0] fifo_mem_data_out;
wire [15:0] lb_out;
wire [15:0] fifo_out;

reg [15:0] mem_data_in0;
reg [15:0] mem_data_in1;
reg [31:0] config_mem;
reg [15:0] mem_addr;
reg mem_ren0;
reg mem_wen0;
reg mem_ren1;
reg mem_wen1;
reg sram_sel;
//reg [15:0] stall_snapshot_0;
//reg [15:0] stall_snapshot_1;
reg prev_clk_en;
//wire [15:0] mem_data_out0_with_snap;
//wire [15:0] mem_data_out1_with_snap;

assign mode = config_mem[1:0];
assign tile_en = config_mem[2];
assign depth = config_mem[15:3];
assign almost_count = config_mem[19:16];
assign enable_chain = config_mem[19];
assign gclk_in = (tile_en==1'b1) ? clk_in : 0;
assign gclk = clk_en ? gclk_in : 0;
assign gclk_sram = (clk_en || config_en_sram) ? gclk_in : 0;


assign data_in_int = (enable_chain==1'b1)?chain_in:data_in;
assign wen_in_int = (enable_chain==1'b1)?chain_wen_in:wen_in;
assign chain_out = data_out;
assign chain_valid_out = valid_out;


//assign mem_data_out0_with_snap = (clk_en > prev_clk_en) ? stall_snapshot_0 : mem_data_out0;
//assign mem_data_out1_with_snap = (clk_en > prev_clk_en) ? stall_snapshot_1 : mem_data_out1;

// WENHACK code, please do not remove w/o checking with steveri@stanford.edu!
// At the last minute, run.csh can modify wen_int assignment above
// to read "assign  wen_in_int = WENHACK" instead
reg WENHACK;
always @(posedge gclk_in or posedge reset) begin
   if (reset==1'b1) begin
     WENHACK = 1'b0;
   end else if (data_in != 16'b0) begin
     WENHACK = 1'b1;
   end
end

/*always @(posedge gclk_in or posedge reset) begin
   if (reset==1'b1) begin
      stall_snapshot_0 <= 0;
      stall_snapshot_1 <= 0;
      prev_clk_en <= 1;
   end
   else begin
      prev_clk_en <= clk_en;
      if(prev_clk_en > clk_en) begin
         stall_snapshot_0 <= mem_data_out0;
         stall_snapshot_1 <= mem_data_out1;
      end
   end
end*/

always @(posedge clk_in or posedge reset) begin
  if (reset==1'b1) begin
    config_mem <= 32'd0;
  end else begin
    if (config_en==1'b1) begin
       case (config_addr[31:24])
         8'd0: config_mem[31:0] <= config_data;
       endcase
    end
  end
end


always @(*) begin
  read_data_sram = {16'b0, (config_en_sram[3:2]==2'b0) ? mem_data_out0:mem_data_out1};
  if(config_en_sram != 4'b0) begin
      mem_cen0_int = 1'b1;
      mem_cen1_int = 1'b1;
      mem_wen0 = (config_write & (config_en_sram[0] | config_en_sram[1]));
      mem_ren0 = (config_read & (config_en_sram[0] | config_en_sram[1]));
      mem_wen1 = (config_write & (config_en_sram[2] | config_en_sram[3]));
      mem_ren1 = (config_read & (config_en_sram[2] | config_en_sram[3]));
      mem_addr = {(config_en_sram[3] | config_en_sram[1]), config_addr[31:24]};
      mem_data_in0 = config_data[15:0];
      mem_data_in1 = config_data[15:0];
      data_out = (sram_sel==1'b1)?mem_data_out1:mem_data_out0;
      valid_out = 1'b0;  
  end
  else begin
  case (mode)
    2'd0: begin
      mem_cen0_int = wen_in_int;
      mem_cen1_int = wen_in_int;
      mem_wen0 = lb_wen;
      mem_ren0 = lb_ren;
      mem_wen1 = lb_wen;
      mem_ren1 = lb_ren;
      mem_addr = lb_addr;
      {mem_data_in1,mem_data_in0} = lb_mem_data_out;
      data_out = lb_out; 
      valid_out = lb_valid_out & wen_in_int;
      almost_full = lb_almost_full;
      almost_empty = lb_almost_empty;
    end
    2'd1: begin
      mem_cen0_int = 1'b1;
      mem_cen1_int = 1'b1;
      mem_wen0 = fifo_wen;
      mem_ren0 = fifo_ren;
      mem_wen1 = fifo_wen;
      mem_ren1 = fifo_ren;
      mem_addr = fifo_addr;
      {mem_data_in1,mem_data_in0} = fifo_mem_data_out;
      data_out = fifo_out;
      valid_out = fifo_valid_out;
      almost_full = fifo_almost_full;
      almost_empty = fifo_almost_empty;
    end
    2'd2: begin
      mem_cen0_int = 1'b1;
      mem_cen1_int = 1'b1;
      mem_wen0 = (~addr_in[9] & wen_in_int);
      mem_ren0 = ren_in;
      mem_wen1 = (addr_in[9] & wen_in_int);
      mem_ren1 = ren_in;
      mem_addr = addr_in;      
      mem_data_in0 = data_in;
      mem_data_in1 = data_in;
      data_out = (sram_sel==1'b1)?mem_data_out1:mem_data_out0;
      valid_out = 1'b1;  
      almost_full = 1'b0; 
      almost_empty = 1'b0; 
    end
    default: begin
      mem_cen0_int = 1'b1;
      mem_cen1_int = 1'b1;
      mem_wen0 = 1'b0; 
      mem_ren0 = 1'b0;
      mem_wen1 = 1'b0; 
      mem_ren1 = 1'b1;
      mem_addr = 16'd0;      
      mem_data_in0 = 16'd0;
      mem_data_in1 = 16'd0;
      data_out = 16'd0;
      valid_out = 1'b0;
      almost_full = 1'b0;
      almost_empty = 1'b0;
    end
  endcase
  end //else
end

always @(posedge gclk) begin
  sram_sel <= addr_in[9];
end

assign mem_cen0 = mem_cen0_int & (mem_wen0 | mem_ren0);
assign mem_cen1 = mem_cen1_int & (mem_wen1 | mem_ren1);


always @(*) begin
        case (config_addr[31:24])
            8'd0 : read_data = config_mem[31:0];
            default : read_data = 'h0;
        endcase
    end

linebuffer_control_unq1  linebuffer_control
(
.clk(gclk),
.reset(reset),
.flush(flush),
.fsm_en(wen_in_int),
.config_en(config_en_linebuf),
.config_addr(config_addr),
.config_data(config_data),
.data_in(data_in_int),
.wen(wen_in_int),
.data_out(lb_out),
.stall_read(1'b0),
.valid(lb_valid_out),
.almost_full(lb_almost_full),
.almost_empty(lb_almost_empty),
.almost_count(almost_count),
.depth(depth),
.addr_to_mem(lb_addr),
.data_to_mem(lb_mem_data_out),
.data_from_mem({mem_data_out1,mem_data_out0}),
.wen_to_mem(lb_wen),
.ren_to_mem(lb_ren),
.read_data(read_data_linebuf)
);

   // always @(posedge gclk) begin
   //    $display("lb_valid  = %b", lb_valid_out);
   //    $display("data_out  = %b", data_out);      
   // end

fifo_control_unq1  fifo_control
(
.clk(gclk),
.reset(reset),
.flush(flush),
.data_in(data_in_int),
.wen(wen_in_int),
.data_out(fifo_out),
.stall_read(ren_in),
.valid(fifo_valid_out),
.almost_full(fifo_almost_full),
.almost_empty(fifo_almost_empty),
.almost_count(almost_count),
.depth(depth),
.addr_to_mem(fifo_addr),
.data_to_mem(fifo_mem_data_out),
.data_from_mem({mem_data_out1,mem_data_out0}),
.wen_to_mem(fifo_wen),
.ren_to_mem(fifo_ren)
);

mem_unq1  mem_inst0
(
.data_out(mem_data_out0),
.data_in(mem_data_in0),
.clk(gclk_sram),
.cen(mem_cen0),
.wen(mem_wen0),
.addr(mem_addr[8:0])
);

mem_unq1  mem_inst1
(
.data_out(mem_data_out1),
.data_in(mem_data_in1),
.clk(gclk_sram),
.cen(mem_cen1),
.wen(mem_wen1),
.addr(mem_addr[8:0])
);
endmodule



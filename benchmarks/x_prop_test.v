module test();

   reg clk;
   wire [15:0] a;
   wire [15:0] b;

   wire [15:0] a_lt_b;

   reg [31:0]  cycle_count;
   reg [31:0] cycle_max;
   
   

   assign a = 16'b100001000000000;
   assign b = 16'b0000x1000000000;

   assign a_lt_b = a + b;
   

   initial begin
      clk = 0;
      cycle_count = 0;
      cycle_max = 10;
      
   end   
   
   always #2 clk = ~clk;

   always @(posedge clk) begin
      $display("a < b ? %b", a_lt_b);
      cycle_count <= cycle_count + 1;

      if (cycle_count == cycle_max) begin
         $finish();
      end
   end

endmodule

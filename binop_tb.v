module test();

   reg [15:0] in0;
   reg [15:0] in1;
   wire [15:0] out;

   integer     outFile;

   initial begin

      outFile = $fopen("tb_output.txt", "w");
      
      in0 = 16'b0010101000111000;
      in1 = 16'b1101101000010101;

      #1 $fwrite(outFile, "%b\n", out);

      $finish();
      
   end

   top dut(.in0(in0), .in1(in1), .out(out));
   
endmodule

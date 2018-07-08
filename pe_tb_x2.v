module test();

   reg [31:0] config_addr_in;
   reg [31:0] config_data_in;

   reg       clk;
   reg       rst;

   integer    config_file, output_file;

   reg [15:0] in_BUS16_S0_T0;
   reg [15:0] in_BUS16_S0_T1;
   reg [15:0] in_BUS16_S0_T2;
   reg [15:0] in_BUS16_S0_T3;
   reg [15:0] in_BUS16_S0_T4;

   reg [15:0] in_BUS16_S1_T0;
   reg [15:0] in_BUS16_S1_T1;
   reg [15:0] in_BUS16_S1_T2;
   reg [15:0] in_BUS16_S1_T3;
   reg [15:0] in_BUS16_S1_T4;

   reg [15:0] in_BUS16_S2_T0;
   reg [15:0] in_BUS16_S2_T1;
   reg [15:0] in_BUS16_S2_T2;
   reg [15:0] in_BUS16_S2_T3;
   reg [15:0] in_BUS16_S2_T4;

   reg [15:0] in_BUS16_S3_T0;
   reg [15:0] in_BUS16_S3_T1;
   reg [15:0] in_BUS16_S3_T2;
   reg [15:0] in_BUS16_S3_T3;
   reg [15:0] in_BUS16_S3_T4;
   
   wire [15:0] out_BUS16_S3_T1;
   
   initial begin
      output_file = $fopen("tb_output.txt");
      config_file = $fopen("./test/pw2_16x16_only_config_lines.bsa");

      #1 clk = 0;
      #1 rst = 0;
      #1 rst = 1;
      #1 rst = 0;
      
      
      #20 $fclose(config_file);
      $fclose(output_file);
      
      
      $finish();
   end // initial begin

   always #2 clk = !clk;

   always @(posedge clk) begin
      $fwrite(output_file, "%b\n", out_BUS16_S3_T1);
   end

   top pe_tile(.clk_in(clk),
               .reset(rst),
               .tile_id(16'h15),

               .in_BUS16_S0_T0(in_BUS16_S0_T0),
               .in_BUS16_S0_T1(in_BUS16_S0_T1),
               .in_BUS16_S0_T2(in_BUS16_S0_T2),
               .in_BUS16_S0_T3(in_BUS16_S0_T3),
               .in_BUS16_S0_T4(in_BUS16_S0_T4),

               .in_BUS16_S1_T0(in_BUS16_S1_T0),
               .in_BUS16_S1_T1(in_BUS16_S1_T1),
               .in_BUS16_S1_T2(in_BUS16_S1_T2),
               .in_BUS16_S1_T3(in_BUS16_S1_T3),
               .in_BUS16_S1_T4(in_BUS16_S1_T4),

               .in_BUS16_S2_T0(in_BUS16_S2_T0),
               .in_BUS16_S2_T1(in_BUS16_S2_T1),
               .in_BUS16_S2_T2(in_BUS16_S2_T2),
               .in_BUS16_S2_T3(in_BUS16_S2_T3),
               .in_BUS16_S2_T4(in_BUS16_S2_T4),

               .in_BUS16_S3_T0(in_BUS16_S3_T0),
               .in_BUS16_S3_T1(in_BUS16_S3_T1),
               .in_BUS16_S3_T2(in_BUS16_S3_T2),
               .in_BUS16_S3_T3(in_BUS16_S3_T3),
               .in_BUS16_S3_T4(in_BUS16_S3_T4),
               
               .out_BUS16_S3_T1(out_BUS16_S3_T1));
   
endmodule

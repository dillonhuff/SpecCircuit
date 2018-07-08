module test();

   reg config_loaded;
   

   reg [31:0] config_addr_in;
   reg [31:0] config_data_in;

   reg       clk;
   reg       rst;

   integer    config_file, output_file, scan_file;

   reg        in_BUS1_S1_T0;
   reg        in_BUS1_S1_T1;
   reg        in_BUS1_S1_T2;
   reg        in_BUS1_S1_T3;
   reg        in_BUS1_S1_T4;
   

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
      output_file = $fopen("tb_output.txt", "w");
      config_file = $fopen("./test/hwmaster_pw2_sixteen.bsa", "r");

      if (config_file == 0) begin
         $display("config file is null");
         $finish;
      end

      in_BUS1_S1_T0 = 1;
      
      in_BUS1_S1_T1 = 1;
      
      in_BUS1_S1_T2 = 1;
      in_BUS1_S1_T3 = 1;
      in_BUS1_S1_T4 = 1;
      
      #1 config_loaded = 0;
      
      #1 clk = 0;
      #1 rst = 0;
      #1 rst = 1;
      #1 rst = 0;

      in_BUS16_S0_T0 = 16'd490;
      in_BUS16_S0_T1 = 16'd490;
      in_BUS16_S0_T2 = 16'd490;
      in_BUS16_S0_T3 = 16'd490;
      in_BUS16_S0_T4 = 16'd490;
      
      in_BUS16_S1_T0 = 16'd490;
      in_BUS16_S1_T1 = 16'd490;
      in_BUS16_S1_T2 = 16'd490;
      in_BUS16_S1_T3 = 16'd490;
      in_BUS16_S1_T4 = 16'd490;

      in_BUS16_S2_T0 = 16'd490;
      in_BUS16_S2_T1 = 16'd490;
      in_BUS16_S2_T2 = 16'd490;
      in_BUS16_S2_T3 = 16'd490;
      in_BUS16_S2_T4 = 16'd490;

      in_BUS16_S3_T0 = 16'd490;
      in_BUS16_S3_T1 = 16'd490;
      in_BUS16_S3_T2 = 16'd490;
      in_BUS16_S3_T3 = 16'd490;
      in_BUS16_S3_T4 = 16'd490;
      
      #2000 $display("DONE");

      $fclose(config_file);
      $fclose(output_file);
      
      $finish();
   end // initial begin

   always #2 clk = !clk;

   always @(negedge clk) begin
      if (!config_loaded) begin
         
         scan_file = $fscanf(config_file, "%h %h\n", config_addr_in, config_data_in);
         
         if (!$feof(config_file)) begin
            $display("config_addr_in = %h", config_addr_in);
            $display("config_data_in = %h", config_data_in);
         end else begin
            $display("DONE loading config");
            config_loaded <= 1;
            config_addr_in <= 32'h0;
            config_data_in <= 32'h0;
         end
      end
      
      $fwrite(output_file, "%b, %d\n", out_BUS16_S3_T1, out_BUS16_S3_T1);
   end

   top pe_tile(.clk_in(clk),
               .reset(rst),
               .tile_id(16'h15),

               .config_addr(config_addr_in),
               .config_data(config_data_in),

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

               .in_BUS1_S1_T0(in_BUS1_S1_T0),
               .in_BUS1_S1_T1(in_BUS1_S1_T1),
               .in_BUS1_S1_T2(in_BUS1_S1_T2),
               .in_BUS1_S1_T3(in_BUS1_S1_T3),
               .in_BUS1_S1_T4(in_BUS1_S1_T4),
      
               .out_BUS16_S3_T1(out_BUS16_S3_T1));
   
endmodule

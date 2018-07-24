// Full system test of CGRA
module test();

   reg [15:0]       data_driver_16_S0;
   reg [15:0]       data_driver_16_S1;
   reg [15:0]       data_driver_16_S2;
   reg [15:0]       data_driver_16_S3;

   wire [15:0] data_out_16_S0;
   wire [15:0] data_out_16_S1;
   wire [15:0] data_out_16_S2;
   wire [15:0] data_out_16_S3;
   
   reg clk;
   reg rst;

   wire tdo;
   wire trst_n;
   wire tdi;
   wire tms;

   wire tck;

   reg reset_done;

   reg [31:0] config_addr;
   reg [31:0] config_data;

   integer    config_file;
   integer    scan_file;
   integer    test_output_file;

   reg 	      config_done;
   reg 	      clear_with_zeros_done;
   reg [31:0] clear_zero_count;

   reg [64:0] cycle_count;
   wire [64:0] max_cycles;

   assign max_cycles = 30;
   
   initial begin

      cycle_count = 0;
      config_file = $fopen("./test/conv_bw_only_config_lines.bsa", "r");
      test_output_file = $fopen("conv_bw_gold_cgra_out_30.txt", "w");

      reset_done = 0;
      clear_with_zeros_done = 0;
      clear_zero_count = 0;

      if (config_file == 0) begin
	 $display("config_file was null");
	 $finish;
      end else begin
	 $display("Loaded config file, descriptor = %d", config_file);
      end

      if (test_output_file == 0) begin
	 $display("test_output_file was null");
	 $finish;
      end else begin
	 $display("Loaded config file, descriptor = %d", config_file);
      end
      
      #1 clk = 0;

      data_driver_16_S2 = 0;

      config_addr = 0;
      config_data = 0;

      #2 rst = 0;
      #2 rst = 1;
      #3 rst = 0;

      reset_done = 1;
      config_done = 0;
      
      $display("DONE WITH RESET");

   end // initial begin

   always #2 clk = ~clk;

   reg [0:0] data_in_S0_T0_reg;
   

   wire [15:0] data_in_16_S0;
   wire [15:0] data_in_16_S1;
   wire [15:0] data_in_16_S2;
   wire [15:0] data_in_16_S3;

   always @(posedge clk) begin
      $display("cycle_count = %d, out = %b", cycle_count, data_driver_16_S0);
      
   end

   // After reseting load data / configuration between rising clock edges
   always @(negedge clk) begin

      if (reset_done && config_done && clear_with_zeros_done) begin
	 cycle_count <= cycle_count + 1;
	 $fwrite(test_output_file, "%b\n", data_out_16_S0);      
      end


      if (reset_done) begin
	 scan_file = $fscanf(config_file, "%h %h\n", config_addr, config_data);

	 if (!$feof(config_file)) begin
	 
	    $display("config addr = %h", config_addr);
	    $display("config data = %h", config_data);
	 
	 end else begin
	    config_done <= 1;
	    config_addr <= 0;
	 end

	 if (cycle_count >= max_cycles) begin
	    $display("Finished at cycle count %d", cycle_count);

            $display("\tdata in side 0 = %b, %d", data_driver_16_S0, data_driver_16_S0);
            $display("\tdata in side 1 = %b, %d", data_driver_16_S1, data_driver_16_S1);
            $display("\tdata in side 2 = %b, %d", data_driver_16_S2, data_driver_16_S2);
            $display("\tdata in side 3 = %b, %d", data_driver_16_S3, data_driver_16_S3);

            $display("\n");

            $display("\tdata out side 0 = %b, %d", data_out_16_S0, data_out_16_S0);
            $display("\tdata out side 1 = %b, %d", data_out_16_S1, data_out_16_S1);
            $display("\tdata out side 2 = %b, %d", data_out_16_S2, data_out_16_S2);
            $display("\tdata out side 3 = %b, %d", data_out_16_S3, data_out_16_S3);

            $fclose(config_file);
	    $finish();
	 end
      end

      if (reset_done && config_done && clear_with_zeros_done) begin
	 //data_driver_16_S2 <= data_driver_16_S2 + 1;
	 data_driver_16_S2 <= 16'hxxxx;
      end else if (clear_zero_count >= 10000) begin
	 $display("Clear with zeros phase done");
	 clear_with_zeros_done <= 1;
      end else begin
	 data_driver_16_S2 <= 0;
	 clear_zero_count <= clear_zero_count + 1;
      end

   end

   
   wire [0:0] data_out_S0_T0;
   wire [0:0] data_out_S0_T1;
   wire [0:0] data_out_S0_T2;
   wire [0:0] data_out_S0_T3;
   wire [0:0] data_out_S0_T4;
   wire [0:0] data_out_S0_T5;
   wire [0:0] data_out_S0_T6;
   wire [0:0] data_out_S0_T7;
   wire [0:0] data_out_S0_T8;
   wire [0:0] data_out_S0_T9;
   wire [0:0] data_out_S0_T10;
   wire [0:0] data_out_S0_T11;
   wire [0:0] data_out_S0_T12;
   wire [0:0] data_out_S0_T13;
   wire [0:0] data_out_S0_T14;
   wire [0:0] data_out_S0_T15;

   assign data_out_16_S0 = {data_out_S0_T0,
			    data_out_S0_T1,
			    data_out_S0_T2,
			    data_out_S0_T3,
			    data_out_S0_T4,
			    data_out_S0_T5,
			    data_out_S0_T6,
			    data_out_S0_T7,
			    data_out_S0_T8,
			    data_out_S0_T9,
			    data_out_S0_T10,
			    data_out_S0_T11,
			    data_out_S0_T12,
			    data_out_S0_T13,
			    data_out_S0_T14,
			    data_out_S0_T15};

   wire [0:0] data_out_S1_T0;
   wire [0:0] data_out_S1_T1;
   wire [0:0] data_out_S1_T2;
   wire [0:0] data_out_S1_T3;
   wire [0:0] data_out_S1_T4;
   wire [0:0] data_out_S1_T5;
   wire [0:0] data_out_S1_T6;
   wire [0:0] data_out_S1_T7;
   wire [0:0] data_out_S1_T8;
   wire [0:0] data_out_S1_T9;
   wire [0:0] data_out_S1_T10;
   wire [0:0] data_out_S1_T11;
   wire [0:0] data_out_S1_T12;
   wire [0:0] data_out_S1_T13;
   wire [0:0] data_out_S1_T14;
   wire [0:0] data_out_S1_T15;

   assign data_out_16_S1 = {data_out_S1_T0,
			    data_out_S1_T1,
			    data_out_S1_T2,
			    data_out_S1_T3,
			    data_out_S1_T4,
			    data_out_S1_T5,
			    data_out_S1_T6,
			    data_out_S1_T7,
			    data_out_S1_T8,
			    data_out_S1_T9,
			    data_out_S1_T10,
			    data_out_S1_T11,
			    data_out_S1_T12,
			    data_out_S1_T13,
			    data_out_S1_T14,
			    data_out_S1_T15};

   wire [0:0] data_out_S2_T0;
   wire [0:0] data_out_S2_T1;
   wire [0:0] data_out_S2_T2;
   wire [0:0] data_out_S2_T3;
   wire [0:0] data_out_S2_T4;
   wire [0:0] data_out_S2_T5;
   wire [0:0] data_out_S2_T6;
   wire [0:0] data_out_S2_T7;
   wire [0:0] data_out_S2_T8;
   wire [0:0] data_out_S2_T9;
   wire [0:0] data_out_S2_T10;
   wire [0:0] data_out_S2_T11;
   wire [0:0] data_out_S2_T12;
   wire [0:0] data_out_S2_T13;
   wire [0:0] data_out_S2_T14;
   wire [0:0] data_out_S2_T15;

   assign data_out_16_S2 = {data_out_S2_T0,
			    data_out_S2_T1,
			    data_out_S2_T2,
			    data_out_S2_T3,
			    data_out_S2_T4,
			    data_out_S2_T5,
			    data_out_S2_T6,
			    data_out_S2_T7,
			    data_out_S2_T8,
			    data_out_S2_T9,
			    data_out_S2_T10,
			    data_out_S2_T11,
			    data_out_S2_T12,
			    data_out_S2_T13,
			    data_out_S2_T14,
			    data_out_S2_T15};

   wire [0:0] data_out_S3_T0;
   wire [0:0] data_out_S3_T1;
   wire [0:0] data_out_S3_T2;
   wire [0:0] data_out_S3_T3;
   wire [0:0] data_out_S3_T4;
   wire [0:0] data_out_S3_T5;
   wire [0:0] data_out_S3_T6;
   wire [0:0] data_out_S3_T7;
   wire [0:0] data_out_S3_T8;
   wire [0:0] data_out_S3_T9;
   wire [0:0] data_out_S3_T10;
   wire [0:0] data_out_S3_T11;
   wire [0:0] data_out_S3_T12;
   wire [0:0] data_out_S3_T13;
   wire [0:0] data_out_S3_T14;
   wire [0:0] data_out_S3_T15;

   assign data_out_16_S3 = {data_out_S3_T0,
			    data_out_S3_T1,
			    data_out_S3_T2,
			    data_out_S3_T3,
			    data_out_S3_T4,
			    data_out_S3_T5,
			    data_out_S3_T6,
			    data_out_S3_T7,
			    data_out_S3_T8,
			    data_out_S3_T9,
			    data_out_S3_T10,
			    data_out_S3_T11,
			    data_out_S3_T12,
			    data_out_S3_T13,
			    data_out_S3_T14,
			    data_out_S3_T15};
   
   wire [0:0] data_in_S0_T0;
   wire [0:0] data_in_S0_T1;
   wire [0:0] data_in_S0_T2;
   wire [0:0] data_in_S0_T3;
   wire [0:0] data_in_S0_T4;
   wire [0:0] data_in_S0_T5;
   wire [0:0] data_in_S0_T6;
   wire [0:0] data_in_S0_T7;
   wire [0:0] data_in_S0_T8;
   wire [0:0] data_in_S0_T9;
   wire [0:0] data_in_S0_T10;
   wire [0:0] data_in_S0_T11;
   wire [0:0] data_in_S0_T12;
   wire [0:0] data_in_S0_T13;
   wire [0:0] data_in_S0_T14;
   wire [0:0] data_in_S0_T15;   

   wire [0:0] data_in_S1_T0;
   wire [0:0] data_in_S1_T1;
   wire [0:0] data_in_S1_T2;
   wire [0:0] data_in_S1_T3;
   wire [0:0] data_in_S1_T4;
   wire [0:0] data_in_S1_T5;
   wire [0:0] data_in_S1_T6;
   wire [0:0] data_in_S1_T7;
   wire [0:0] data_in_S1_T8;
   wire [0:0] data_in_S1_T9;
   wire [0:0] data_in_S1_T10;
   wire [0:0] data_in_S1_T11;
   wire [0:0] data_in_S1_T12;
   wire [0:0] data_in_S1_T13;
   wire [0:0] data_in_S1_T14;
   wire [0:0] data_in_S1_T15;   

   wire [0:0] data_in_S2_T0;
   wire [0:0] data_in_S2_T1;
   wire [0:0] data_in_S2_T2;
   wire [0:0] data_in_S2_T3;
   wire [0:0] data_in_S2_T4;
   wire [0:0] data_in_S2_T5;
   wire [0:0] data_in_S2_T6;
   wire [0:0] data_in_S2_T7;
   wire [0:0] data_in_S2_T8;
   wire [0:0] data_in_S2_T9;
   wire [0:0] data_in_S2_T10;
   wire [0:0] data_in_S2_T11;
   wire [0:0] data_in_S2_T12;
   wire [0:0] data_in_S2_T13;
   wire [0:0] data_in_S2_T14;
   wire [0:0] data_in_S2_T15;   
   
   wire [0:0] data_in_S3_T0;
   wire [0:0] data_in_S3_T1;
   wire [0:0] data_in_S3_T2;
   wire [0:0] data_in_S3_T3;
   wire [0:0] data_in_S3_T4;
   wire [0:0] data_in_S3_T5;
   wire [0:0] data_in_S3_T6;
   wire [0:0] data_in_S3_T7;
   wire [0:0] data_in_S3_T8;
   wire [0:0] data_in_S3_T9;
   wire [0:0] data_in_S3_T10;
   wire [0:0] data_in_S3_T11;
   wire [0:0] data_in_S3_T12;
   wire [0:0] data_in_S3_T13;
   wire [0:0] data_in_S3_T14;
   wire [0:0] data_in_S3_T15;   
   
   assign data_in_16_S2 = {
			   data_in_S2_T0,
			   data_in_S2_T1,
			   data_in_S2_T2,
			   data_in_S2_T3,
			   data_in_S2_T4,
			   data_in_S2_T5,
			   data_in_S2_T6,
			   data_in_S2_T7,
			   data_in_S2_T8,
			   data_in_S2_T9,
			   data_in_S2_T10,
			   data_in_S2_T11,
			   data_in_S2_T12,
			   data_in_S2_T13,
			   data_in_S2_T14,
			   data_in_S2_T15};

   // always @(posedge clk) begin
   //    if (reset_done && config_done && clear_with_zeros_done) begin
   // 	 data_driver_16_S2 <= data_driver_16_S2 + 1;	 
   //    end else if (clear_zero_count >= 10000) begin
   // 	 $display("Clear with zeros phase done");
	 
   // 	 clear_with_zeros_done <= 1;
   //    end else begin
   // 	 data_driver_16_S2 <= 0;
   // 	 clear_zero_count <= clear_zero_count + 1;
   //    end
   // end

   assign    {data_in_S0_T0,
	      data_in_S0_T1,
	      data_in_S0_T2,
	      data_in_S0_T3,
	      data_in_S0_T4,
	      data_in_S0_T5,
	      data_in_S0_T6,
	      data_in_S0_T7,
	      data_in_S0_T8,
	      data_in_S0_T9,
	      data_in_S0_T10,
	      data_in_S0_T11,
	      data_in_S0_T12,
	      data_in_S0_T13,
	      data_in_S0_T14,
	      data_in_S0_T15} = data_driver_16_S0;

   assign    {data_in_S1_T0,
	      data_in_S1_T1,
	      data_in_S1_T2,
	      data_in_S1_T3,
	      data_in_S1_T4,
	      data_in_S1_T5,
	      data_in_S1_T6,
	      data_in_S1_T7,
	      data_in_S1_T8,
	      data_in_S1_T9,
	      data_in_S1_T10,
	      data_in_S1_T11,
	      data_in_S1_T12,
	      data_in_S1_T13,
	      data_in_S1_T14,
	      data_in_S1_T15} = data_driver_16_S1;

   assign    {data_in_S2_T0,
	      data_in_S2_T1,
	      data_in_S2_T2,
	      data_in_S2_T3,
	      data_in_S2_T4,
	      data_in_S2_T5,
	      data_in_S2_T6,
	      data_in_S2_T7,
	      data_in_S2_T8,
	      data_in_S2_T9,
	      data_in_S2_T10,
	      data_in_S2_T11,
	      data_in_S2_T12,
	      data_in_S2_T13,
	      data_in_S2_T14,
	      data_in_S2_T15} = data_driver_16_S2;

   assign    {data_in_S3_T0,
	      data_in_S3_T1,
	      data_in_S3_T2,
	      data_in_S3_T3,
	      data_in_S3_T4,
	      data_in_S3_T5,
	      data_in_S3_T6,
	      data_in_S3_T7,
	      data_in_S3_T8,
	      data_in_S3_T9,
	      data_in_S3_T10,
	      data_in_S3_T11,
	      data_in_S3_T12,
	      data_in_S3_T13,
	      data_in_S3_T14,
	      data_in_S3_T15} = data_driver_16_S3;
   
   top cgra(.clk_in(clk),
	    .reset_in(rst),
	    .config_addr_in(config_addr),
	    .config_data_in(config_data),

	    .pad_S0_T0_in(data_in_S0_T0),
	    .pad_S0_T1_in(data_in_S0_T1),
	    .pad_S0_T2_in(data_in_S0_T2),
	    .pad_S0_T3_in(data_in_S0_T3),
	    .pad_S0_T4_in(data_in_S0_T4),
	    .pad_S0_T5_in(data_in_S0_T4),
	    .pad_S0_T6_in(data_in_S0_T6),
	    .pad_S0_T7_in(data_in_S0_T7),
	    .pad_S0_T8_in(data_in_S0_T8),
	    .pad_S0_T9_in(data_in_S0_T9),
	    .pad_S0_T10_in(data_in_S0_T10),
	    .pad_S0_T11_in(data_in_S0_T11),
	    .pad_S0_T12_in(data_in_S0_T12),
	    .pad_S0_T13_in(data_in_S0_T13),
	    .pad_S0_T14_in(data_in_S0_T14),
	    .pad_S0_T15_in(data_in_S0_T15),

	    .pad_S1_T0_in(data_in_S1_T0),
	    .pad_S1_T1_in(data_in_S1_T1),
	    .pad_S1_T2_in(data_in_S1_T2),
	    .pad_S1_T3_in(data_in_S1_T3),
	    .pad_S1_T4_in(data_in_S1_T4),
	    .pad_S1_T5_in(data_in_S1_T4),
	    .pad_S1_T6_in(data_in_S1_T6),
	    .pad_S1_T7_in(data_in_S1_T7),
	    .pad_S1_T8_in(data_in_S1_T8),
	    .pad_S1_T9_in(data_in_S1_T9),
	    .pad_S1_T10_in(data_in_S1_T10),
	    .pad_S1_T11_in(data_in_S1_T11),
	    .pad_S1_T12_in(data_in_S1_T12),
	    .pad_S1_T13_in(data_in_S1_T13),
	    .pad_S1_T14_in(data_in_S1_T14),
	    .pad_S1_T15_in(data_in_S1_T15),
            
	    .pad_S2_T0_in(data_in_S2_T0),
	    .pad_S2_T1_in(data_in_S2_T1),
	    .pad_S2_T2_in(data_in_S2_T2),
	    .pad_S2_T3_in(data_in_S2_T3),
	    .pad_S2_T4_in(data_in_S2_T4),
	    .pad_S2_T5_in(data_in_S2_T5),
	    .pad_S2_T6_in(data_in_S2_T6),
	    .pad_S2_T7_in(data_in_S2_T7),
	    .pad_S2_T8_in(data_in_S2_T8),
	    .pad_S2_T9_in(data_in_S2_T9),
	    .pad_S2_T10_in(data_in_S2_T10),
	    .pad_S2_T11_in(data_in_S2_T11),
	    .pad_S2_T12_in(data_in_S2_T12),
	    .pad_S2_T13_in(data_in_S2_T13),
	    .pad_S2_T14_in(data_in_S2_T14),
	    .pad_S2_T15_in(data_in_S2_T15),

	    .pad_S3_T0_in(data_in_S3_T0),
	    .pad_S3_T1_in(data_in_S3_T1),
	    .pad_S3_T2_in(data_in_S3_T2),
	    .pad_S3_T3_in(data_in_S3_T3),
	    .pad_S3_T4_in(data_in_S3_T4),
	    .pad_S3_T5_in(data_in_S3_T5),
	    .pad_S3_T6_in(data_in_S3_T6),
	    .pad_S3_T7_in(data_in_S3_T7),
	    .pad_S3_T8_in(data_in_S3_T8),
	    .pad_S3_T9_in(data_in_S3_T9),
	    .pad_S3_T10_in(data_in_S3_T10),
	    .pad_S3_T11_in(data_in_S3_T11),
	    .pad_S3_T12_in(data_in_S3_T12),
	    .pad_S3_T13_in(data_in_S3_T13),
	    .pad_S3_T14_in(data_in_S3_T14),
	    .pad_S3_T15_in(data_in_S3_T15),
            
	    .pad_S0_T0_out(data_out_S0_T0),
	    .pad_S0_T1_out(data_out_S0_T1),
	    .pad_S0_T2_out(data_out_S0_T2),
	    .pad_S0_T3_out(data_out_S0_T3),	    	    
	    .pad_S0_T4_out(data_out_S0_T4),
	    .pad_S0_T5_out(data_out_S0_T5),
	    .pad_S0_T6_out(data_out_S0_T6),
	    .pad_S0_T7_out(data_out_S0_T7),
	    .pad_S0_T8_out(data_out_S0_T8),
	    .pad_S0_T9_out(data_out_S0_T9),
	    .pad_S0_T10_out(data_out_S0_T10),
	    .pad_S0_T11_out(data_out_S0_T11),
	    .pad_S0_T12_out(data_out_S0_T12),
	    .pad_S0_T13_out(data_out_S0_T13),
	    .pad_S0_T14_out(data_out_S0_T14),
	    .pad_S0_T15_out(data_out_S0_T15),	    

	    .pad_S1_T0_out(data_out_S1_T0),
	    .pad_S1_T1_out(data_out_S1_T1),
	    .pad_S1_T2_out(data_out_S1_T2),
	    .pad_S1_T3_out(data_out_S1_T3),	    	    
	    .pad_S1_T4_out(data_out_S1_T4),
	    .pad_S1_T5_out(data_out_S1_T5),
	    .pad_S1_T6_out(data_out_S1_T6),
	    .pad_S1_T7_out(data_out_S1_T7),
	    .pad_S1_T8_out(data_out_S1_T8),
	    .pad_S1_T9_out(data_out_S1_T9),
	    .pad_S1_T10_out(data_out_S1_T10),
	    .pad_S1_T11_out(data_out_S1_T11),
	    .pad_S1_T12_out(data_out_S1_T12),
	    .pad_S1_T13_out(data_out_S1_T13),
	    .pad_S1_T14_out(data_out_S1_T14),
	    .pad_S1_T15_out(data_out_S1_T15),	    

	    .pad_S2_T0_out(data_out_S2_T0),
	    .pad_S2_T1_out(data_out_S2_T1),
	    .pad_S2_T2_out(data_out_S2_T2),
	    .pad_S2_T3_out(data_out_S2_T3),	    	    
	    .pad_S2_T4_out(data_out_S2_T4),
	    .pad_S2_T5_out(data_out_S2_T5),
	    .pad_S2_T6_out(data_out_S2_T6),
	    .pad_S2_T7_out(data_out_S2_T7),
	    .pad_S2_T8_out(data_out_S2_T8),
	    .pad_S2_T9_out(data_out_S2_T9),
	    .pad_S2_T10_out(data_out_S2_T10),
	    .pad_S2_T11_out(data_out_S2_T11),
	    .pad_S2_T12_out(data_out_S2_T12),
	    .pad_S2_T13_out(data_out_S2_T13),
	    .pad_S2_T14_out(data_out_S2_T14),
	    .pad_S2_T15_out(data_out_S2_T15),	    

	    .pad_S3_T0_out(data_out_S3_T0),
	    .pad_S3_T1_out(data_out_S3_T1),
	    .pad_S3_T2_out(data_out_S3_T2),
	    .pad_S3_T3_out(data_out_S3_T3),	    	    
	    .pad_S3_T4_out(data_out_S3_T4),
	    .pad_S3_T5_out(data_out_S3_T5),
	    .pad_S3_T6_out(data_out_S3_T6),
	    .pad_S3_T7_out(data_out_S3_T7),
	    .pad_S3_T8_out(data_out_S3_T8),
	    .pad_S3_T9_out(data_out_S3_T9),
	    .pad_S3_T10_out(data_out_S3_T10),
	    .pad_S3_T11_out(data_out_S3_T11),
	    .pad_S3_T12_out(data_out_S3_T12),
	    .pad_S3_T13_out(data_out_S3_T13),
	    .pad_S3_T14_out(data_out_S3_T14),
	    .pad_S3_T15_out(data_out_S3_T15),	    
	    
	    .tdi(tdi),
	    .tms(tms),
	    .tck(tck),
	    .tdo(tdo),
	    .trst_n(trst_n));
   
endmodule

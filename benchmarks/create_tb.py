import os

# config_file_name = './test/conv_3_1_only_config_lines.bsa'
# output_file_name = 'conv_3_1_tb_output.txt'
# tb_name = 'conv_3_1_tb.v'
# tb_file = './benchmarks/test.v'

def generate_tb_from_template(tb_template_file, config_file_name, output_file_name, tb_file):
    with open(tb_template_file, 'r') as myfile:
        data = myfile.read()

    lines = data.splitlines()

    new_tb = ''

    for line in lines:
        if line.find('config_file = $fopen(') != -1:
            new_tb += '\tconfig_file = $fopen(\"' + config_file_name + '\", \"r\");\n'
        elif line.find('test_output_file = $fopen(') != -1:
            new_tb += '\ttest_output_file = $fopen(\"' + output_file_name + '\", \"w\");\n'
        else:
            new_tb += line + '\n'

    tb_output_file = open(tb_file, 'w')
    tb_output_file.write(new_tb)
    tb_output_file.close()

def generate_tb_for_application_from_template(app_name, bitstream_name, tb_template_file):
    # Generate the real test benches
    config_file_name = './test/' + bitstream_name + '_only_config_lines.bsa'
    output_file_name = app_name + '_tb_output.txt'
    tb_name = app_name + '_tb.v'
    tb_template_file = './benchmarks/test.v'

    generate_tb_from_template(tb_template_file, config_file_name, output_file_name, tb_name)

def run_iverilog(app_name, tb_name, top_mod_file_name):
    compile_cmd = 'iverilog -o ' + app_name + ' ' + tb_name + ' ' + top_mod_file_name
    print 'iverilog compile command = ', compile_cmd
    compile_res = os.system(compile_cmd)

    assert(compile_res == 0)

    run_cmd = './' + app_name
    print 'iverilog run command = ', run_cmd
    run_res = os.system(run_cmd)

    assert(run_res == 0)

generate_tb_for_application_from_template('conv_3_1_specialized', 'conv_3_1', './benchmarks/test.v')
run_iverilog('conv_3_1_specialized', 'conv_3_1_specialized_tb.v', 'conv_3_1_cgra.v')

generate_tb_for_application_from_template('conv_2_1_specialized', 'conv_2_1', './benchmarks/test.v')
run_iverilog('conv_2_1_specialized', 'conv_2_1_specialized_tb.v', 'conv_2_1_cgra.v')

generate_tb_for_application_from_template('conv_bw_specialized', 'conv_bw', './benchmarks/test.v')
run_iverilog('conv_bw_specialized', 'conv_bw_specialized_tb.v', 'conv_bw_cgra.v')

# Q: What should the testbench structure be?
# A: I think that I want it to be: A specialized version and an unspecialized version
#    of each testbench, so the file name needs to be different

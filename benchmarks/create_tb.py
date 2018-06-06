import os


# Must replace x constants to avoid messing up linebuffer initialization
def remove_x_constants(verilog_file_name):
    myfile = open(verilog_file_name, 'r')
    file_str = myfile.read()
    myfile.close()

    no_x_str = file_str.replace('\'hx', '\'h0')

    outfile = open(verilog_file_name, 'w')
    outfile.write(no_x_str)
    outfile.close()

remove_x_constants('conv_2_1_cgra_no_x.v')

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
    remove_x_constants(top_mod_file_name)

    compile_cmd = 'iverilog -o ' + app_name + ' ' + tb_name + ' ' + top_mod_file_name
    print 'iverilog compile command = ', compile_cmd
    compile_res = os.system(compile_cmd)

    assert(compile_res == 0)

    run_cmd = './' + app_name
    print 'iverilog run command = ', run_cmd
    run_res = os.system(run_cmd)

    assert(run_res == 0)

# This function compares output files from the end backward. It checks whether or
# not the results are correct
def compare_output_files(file0, file1):
    print 'Comparing outputs of ', file0, ' and ', file1

    with open(file0, 'r') as myfile:
        file0_str = myfile.read()

    with open(file1, 'r') as myfile:
        file1_str = myfile.read()

    file0_lines = file0_str.splitlines()
    file1_lines = file1_str.splitlines()

    file0_num_lines = len(file0_lines)
    file1_num_lines = len(file1_lines)

    assert(file0_num_lines == file1_num_lines)

    # 0 1
    # 0 1
    # 0 0
    # 0 0

    # Shared len == 2

    num_lines = file0_num_lines
    for i in range(0, num_lines):
        l0 = file0_lines[i]
        l1 = file1_lines[i]

        if l0 != l1:
            print 'Disagreement on shared suffix line ', i, ': ', l0, ' != ', l1

    print 'Done with comparison'

generate_tb_for_application_from_template('conv_3_1_specialized', 'conv_3_1', './benchmarks/test.v')
run_iverilog('conv_3_1_specialized', 'conv_3_1_specialized_tb.v', 'conv_3_1_cgra.v')
#compare_output_files('conv_2_1_specialized_tb_output.txt', 'conv_3_1_unspecialized_tb_output.txt')

# generate_tb_for_application_from_template('conv_2_1_specialized', 'conv_2_1', './benchmarks/test.v')
# run_iverilog('conv_2_1_specialized', 'conv_2_1_specialized_tb.v', 'conv_2_1_cgra.v')
# compare_output_files('conv_2_1_specialized_tb_output.txt', 'conv_3_1_specialized_tb_output.txt')

# generate_tb_for_application_from_template('conv_bw_specialized', 'conv_bw', './benchmarks/test.v')
# run_iverilog('conv_bw_specialized', 'conv_bw_specialized_tb.v', 'conv_bw_cgra.v')

# compare_output_files('conv_2_1_specialized_tb_output.txt', 'conv_bw_specialized_tb_output.txt')

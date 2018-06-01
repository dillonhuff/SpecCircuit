
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

def generate_tb_for_application_from_template(app_name, tb_template_file):
    # Generate the real test benches
    config_file_name = './test/' + app_name + '_only_config_lines.bsa'
    output_file_name = app_name + '_tb_output.txt'
    tb_name = app_name + '_tb.v'
    tb_template_file = './benchmarks/test.v'

    generate_tb_from_template(tb_template_file, config_file_name, output_file_name, tb_name)

generate_tb_for_application_from_template('conv_3_1', './benchmarks/test.v')
generate_tb_for_application_from_template('conv_2_1', './benchmarks/test.v')
generate_tb_for_application_from_template('conv_bw', './benchmarks/test.v')

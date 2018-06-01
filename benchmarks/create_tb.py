
tb_file = './benchmarks/test.v'

with open(tb_file, 'r') as myfile:
  data = myfile.read()

lines = data.splitlines()

config_file_name = './test/conv_3_1_only_config_lines.bsa'
output_file_name = 'conv_3_1_tb_output.txt'

new_tb = ''

for line in lines:
    if line.find('config_file = $fopen(') != -1:
        new_tb += '\tconfig_file = $fopen(\"' + config_file_name + '\", \"w\");\n'
    elif line.find('test_output_file = $fopen(') != -1:
        new_tb += '\ttest_output_file = $fopen(\"' + output_file_name + '\", \"w\");\n'
    else:
        new_tb += line + '\n'

print 'NEW TB'
print new_tb

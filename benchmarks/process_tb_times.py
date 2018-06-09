myfile = open('conv_3_1_unspecialized_tb_metadata.txt', 'r')
file_str = myfile.read()
myfile.close()

ls = file_str.splitlines()

for line in ls:
    print line

assert(len(ls) == 2)

start_time_str = ls[0]
end_time_str = ls[1]

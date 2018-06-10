from sets import Set

def num_tiles_used_in_bitstream(bs_file_name):
    bs_file = open(bs_file_name, 'r')
    bs = bs_file.read()
    bs_file.close()

    bs_lines = bs.splitlines()
    tiles = Set([])
    for ln in bs_lines:
        addr = ln.partition(" ")[0]
        addr = addr[len(addr) - 3:]
        #print 'line = ', ln, 'addr = ', addr
        if not (addr in tiles):
            tiles.add(addr)
            

    return len(tiles)

print num_tiles_used_in_bitstream('./test/pw2_16x16_only_config_lines.bsa')
print num_tiles_used_in_bitstream('./test/conv_2_1_only_config_lines.bsa')
print num_tiles_used_in_bitstream('./test/conv_3_1_only_config_lines.bsa')
print num_tiles_used_in_bitstream('./test/conv_bw_travis_only_config_lines.bsa')

def seconds_from_date(date_str):
    hr_min_sec = date_str.partition(":")

    # print 'hr_min_sec comps'
    # for cp in hr_min_sec:
    #     print cp
        
    hour = int(hr_min_sec[0])

    min_sec = hr_min_sec[2].partition(":")
    minute = int(min_sec[0])
    sec = int(min_sec[2])

    return hour*3600 + minute*60 + sec

def process_specialized_line(file_str):
    ls = file_str.splitlines()

    start_time_str = ls[0]
    end_time_str = ls[1]

    s_str = 'Start='
    e_str = 'Finish='

    assert(start_time_str.startswith(s_str))
    assert(end_time_str.startswith(e_str))

    start_time_str = start_time_str[len(s_str):]
    end_time_str = end_time_str[len(e_str):]

    start_date_str = start_time_str.partition("--")[2]
    end_date_str = end_time_str.partition("--")[2]

    assert(start_date_str == end_date_str)

    start_time_str = start_time_str.partition("--")[0]
    end_time_str = end_time_str.partition("--")[0]

    start_seconds = seconds_from_date(start_time_str)
    end_seconds = seconds_from_date(end_time_str)
    
    assert((end_seconds - start_seconds) >= 0)

    print 'Start seconds           = ', start_seconds
    print 'End seconds             = ', end_seconds
    print 'Runtime Difference      = ', (end_seconds - start_seconds)
    
def process_metadata_file(file_name):
    myfile = open(file_name, 'r')
    file_str = myfile.read()
    myfile.close()

    process_specialized_line(file_str)

def process_metadata_files(app_name):
    unspecialized_file_name = app_name + '_unspecialized_tb_metadata.txt'
    print '----- Timing for ', unspecialized_file_name
    process_metadata_file(unspecialized_file_name)

    specialized_file_name = app_name + '_specialized_tb_metadata.txt'
    print '----- Timing for ', specialized_file_name
    process_metadata_file(specialized_file_name)
    
process_metadata_files('conv_3_1')
process_metadata_files('conv_2_1')
process_metadata_files('conv_bw_travis')

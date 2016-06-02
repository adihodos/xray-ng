import os
import sys
import datetime

def to_float_rgb(rgb) :
    return rgb[0] / 255.0, rgb[1] / 255.0, rgb[2] / 255.0

def output_rgb_color(stringval) :
    clr_val = int(stringval, 16)

    return to_float_rgb(((clr_val >> 16) & 0xFF, (clr_val >> 8) & 0xFF, (clr_val >> 0) & 0xFF))

def parse_color_file(file_name) :
    coll = dict()

    with open(file_name, 'r') as color_file :
        for line in color_file :
            line = line.lstrip().rstrip()

            if not line.startswith('.') and not line.endswith('}') :
                continue

            line = line.lstrip('.')
            parts = line.split(' ')

            coll[parts[0].replace('-', '')] = parts[3].lstrip('#').rstrip(';')

    return coll

def output_color_def_hpp_file(template_file, output_file, content) :
    file_content = ''

    with open(template_file, 'r') as tfile :
        for line in tfile :
            file_content += line

    file_content = file_content.replace('{gen_time}', str(datetime.datetime.now()))
    file_content = file_content.replace("{pallete_definitions}", content)

    with open(output_file, 'w') as outfile :
        outfile.write(file_content)

def output_color_def_cc_file(template_file, output_file, color_coll) :
    file_content = ''

    with open(template_file, 'r') as tfile :
        for line in tfile :
            file_content += line

    file_content = file_content.replace('{gen_time}', str(datetime.datetime.now()))
    tmpl_string = "constexpr xray::rendering::rgb_color xray::rendering::color_palette::{palette_name}::{color_name};"
    all_content = ""
    for k, v in color_coll.iteritems() :
        for color in v :
            s = tmpl_string.replace("{palette_name}", k).replace("{color_name}", color)
            all_content += s + "\n"

    file_content = file_content.replace("{file_content}", all_content)
    with open(output_file, 'w') as outfile :
        outfile.write(file_content)


    # file_content = file_content.replace("{pallete_definitions}", content)


def gen_pallete_for_defs(color_def_coll, pallette_name) :
    tmpl_entry = 'static constexpr rgb_color {} = {{{:.4f}f, {:.4f}f, {:.4}f, 1.0f}};'

    lines = []
    for (clr, clr_val_str) in color_def_coll.items() :
        rgb = output_rgb_color(clr_val_str)
        lines.append(tmpl_entry.format(clr, rgb[0], rgb[1], rgb[2]))

    lines = sorted(lines)

    content = "\tstruct " + pallette_name + " {\n"
    for l in lines :
        content += '\t\t'
        content += l
        content += '\n'

    content = content + "\n\t};\n"
    return content

    # templ_str = "struct {pallette_name} {"
    # p_str = templ_str.replace("{pallette_name}", pallette_name)

if __name__ == "__main__" :
    file_list = [('colordefs/flat_design_colors_full.css', 'flat'), ('colordefs/material_design_colors_full.css', 'material')]

    full_content = ''
    coll_wp = dict()
    for (fname, cname) in file_list :
        coll = parse_color_file(fname)

        if len(coll) is 0 :
            print 'No colors could be extracted from file !!'
            sys.exit(-1)

        full_content = full_content + gen_pallete_for_defs(coll, cname)
        coll_wp[cname] = coll

    output_color_def_hpp_file('colordefs/output_template_file', 'color_palettes.hpp', full_content)
    output_color_def_cc_file('colordefs/output_templ_cc_file', 'color_palettes.cc', coll_wp)

        #output_color_def_cpp_file(cname, coll)

    sys.exit(0)

import sys
import string
import os

def gen_keysyms_x11() :
  keySymData = [
    ('symtables/x11.keymap.1', 'X11_MISC_FUNCTION_KEYS_MAPPING_TABLE', 0xFF, None),
    ('symtables/x11.keymap.0', 'X11_LATIN1_KEYS_MAPPING_TABLE', 0x000000FF, 8),
    ('symtables/win32.keymap', 'WIN32_KEYS_MAPPING_TABLE', 0xFF, None)
  ]

  enum_class_name = 'KeySymbol'

  enum_names = set()
  i = 0
  j = 0
  os.makedirs('out', exist_ok=True)
  for kd in keySymData :
    fname = kd[0]
    i += 1
    with open(fname, 'r') as fp :
      kslist = [f'{enum_class_name}::unknown' for i in range(0, 256)]

      for ln in fp.readlines() :
        # print(ln)
        ldefs = ln.split(',')
        ename = ldefs[0].strip()
        eidx = ldefs[1].strip()

        if not ename or not eidx :
          continue

        idx = int(eidx, 16) & kd[2]

        print("idx {}", idx)
        
        kslist[idx] = f'{enum_class_name}::{ename}'
        enum_names.add(f'{enum_class_name}::{ename}')
      
      # print(kslist)
      s = f'static constexpr const {enum_class_name} ' + kd[1] + '[] = {\n'
      with open('out/outf{}'.format(i), 'w') as outf :
        outf.write(s)
        for l in kslist :
          outf.writelines(l)
          outf.write(',\n')

        outf.write('};\n')
        outf.flush()

  enum_names_ordered = [x for x in enum_names]
  enum_names_ordered.append(f'{enum_class_name}::unknown')
  enum_names_ordered.sort()
  with open('out/imgui_mappings.hpp', 'w') as imgui:
    #valid_keys = [x for x in filter(lambda x: x != 'key_sym::e::unknown', enum_names_ordered)]
    str_enum = f'''
        enum class {enum_class_name} : uint8_t {{
    '''

    imgui.write(str_enum);
    for e in enum_names_ordered:
      idx = e.rfind('::')
      imgui.write(f'{e[(idx + 2):]},\n')

    imgui.write('};\n')

    struct_name  = 'XrayKeyToImGuiKeyPair'
    str = f'''
      struct {struct_name} {{
          {enum_class_name} xr_keycode;
          int32_t im_keycode;
      }};

      static constexpr {struct_name} XR_KEY_TO_IMGUI_KEY_TABLE[{len(enum_names_ordered)}] = {{
    '''

    imgui.write(str);

    for e in enum_names_ordered:
      imgui.write(f'\t{{ {e}, ImGuiKey_  }},\n')

    imgui.write('};')
    imgui.flush()

if __name__ == '__main__' :
  gen_keysyms_x11()
  sys.exit(0)

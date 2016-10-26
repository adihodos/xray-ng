import sys
import string

def gen_keysyms_x11() :
  keySymData = [
    ('symtables/x11.keymap.1', 'X11_MISC_FUNCTION_KEYS_MAPPING_TABLE', 0xFF, None),
    ('symtables/x11.keymap.0', 'X11_LATIN1_KEYS_MAPPING_TABLE', 0x000000FF, 8)
  ]

  i = 0
  for kd in keySymData :
    fname = kd[0]
    i += 1
    with open(fname, 'r') as fp :
      kslist = ['key_sym::e::unknown' for i in range(0, 256)]

      for ln in fp.readlines() :
        # print(ln)
        ldefs = ln.split(',')
        ename = ldefs[0].strip()
        eidx = ldefs[1].strip()

        if not ename or not eidx :
          continue

        idx = int(eidx, 16) & kd[2]

        print("idx {}", idx)
        
        kslist[idx] = 'key_sym::e::{}'.format(ename)
      
      # print(kslist)
      s = 'static constexpr const xray::ui::key_sym::e ' + kd[1] + '[] = {\n'
      with open('outf{}'.format(i), 'w') as outf :
        outf.write(s)
        for l in kslist :
          outf.writelines(l)
          outf.write(',\n')

        outf.write('};\n')
        outf.flush()

if __name__ == '__main__' :
  gen_keysyms_x11()
  sys.exit(0)
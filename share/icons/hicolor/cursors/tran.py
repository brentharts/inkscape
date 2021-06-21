import sys

f = open(sys.argv[1], 'r')
o = open(sys.argv[2], 'w')

style = False

for line in f:
  if line.find("<defs>") >= 0 or line.find(" <def") >= 0:
    o.write("  <style>\n    @import '../cursors.css';\n  </style>\n")
  o.write(line)

#    txt = line.strip()
#    if txt == '':
#        sys.stdout.write('\n\n')
#        sys.stdout.flush()
#    sys.stdout.write( txt + ';')
#    sys.stdout.flush()

f.close()
o.close()


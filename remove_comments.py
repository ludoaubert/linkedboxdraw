
f=open("bombix-beta.cpp", 'r')
buffer = f.read()
f.close()
buffer2 = return re.sub('//.*?\n|/\*.*?\*/', '', buffer, flags=re.S)

f=open("bombix-beta2.cpp", 'w')
f.write(buffer2)
f.close()

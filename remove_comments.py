import re

f=open("bombix-beta.cpp", 'r')
buffer = f.read()
f.close()
buffer2 = re.sub('//.*?\n|/\*.*?\*/', '', buffer, flags=re.S)

strings_to_replace = {'HORIZONTAL': 'H', 'VERTICAL': 'V', 'INCREASE': 'I', 'DECREASE': 'D'} 

for key, value in strings_to_replace.items():
    buffer2 = buffer2.replace(key, value)

f=open("bombix-beta2.cpp", 'w')
f.write(buffer2)
f.close()

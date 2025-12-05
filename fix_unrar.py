import sys

path = r'f:\repos\sumatrapdf-gurupia\ext\unrar\cmddata.cpp'
try:
    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()
except UnicodeDecodeError:
    with open(path, 'r', encoding='latin-1') as f:
        content = f.read()

# Replace 'unsigned int Flags;' with '\nunsigned int Flags;'
# Also handle 'uint Flags;' just in case
new_content = content.replace('unsigned int Flags;', '\nunsigned int Flags;')
new_content = new_content.replace('uint Flags;', '\nunsigned int Flags;')

with open(path, 'w', encoding='utf-8') as f:
    f.write(new_content)

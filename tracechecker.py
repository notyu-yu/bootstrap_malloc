import sys

file = sys.argv[1].strip()

print(f'checking file: {file}')

with open(file, 'r') as f:
    lines_lst = [i for i in f.read().split("\n") if i]
    # Variable allocation information, 0 is free 1 is allocated
    alloc_lst = [0]*int((lines_lst)[1])
    op_lst = [i.split() for i in lines_lst[4:]]
    # Actions, f, a, or r
    action_lst = [i[0] for i in op_lst]
    # Variable used in each line
    variable_lst = [int(i[1]) for i in op_lst]
    for i in range(len(op_lst)):
        v = variable_lst[i]
        if action_lst[i] == 'f':
            if alloc_lst[v]==0:
                print(f'unallocated line freed on line {i+4}')
                alloc_lst[v]=0
            else:
                alloc_lst[v]=0
        elif action_lst[i] == 'a':
            if alloc_lst[v]==1:
                print(f'allocating pointer twice on line {i+4}')
                alloc_lst[v]=1
            else:
                alloc_lst[v]=1
        elif action_lst[i] == 'r':
            if alloc_lst[v]==0:
                print(f'reallocating unallocated pointer on line {i+4}')
        else:
            print(f'invalid action "{action_lst[i]}" on line {i+4}')

print("check finished")

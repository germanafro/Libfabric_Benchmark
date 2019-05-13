"""
    helper script
    to help gnuplot iterate correctly
"""

import os

print("hoi")

fo = open("NxN_helper.dat", "w")

en = int(input(('Anzahl Entry_nodes: ')))
pn = int(input(('Anzahl Processing_Nodes:')))

for i in range(en):
    for j in range(pn):
        outstring = str(i)+ " " + str(j)+ "\n" 
        print(outstring)
        fo.write(outstring)

fo.close()

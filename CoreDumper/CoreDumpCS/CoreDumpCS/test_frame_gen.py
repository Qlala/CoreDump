# -*- coding: utf-8 -*-
"""
Created on Mon Jan  7 15:26:08 2019

@author: Qlala
"""
import numpy as np;
import random as rand;
import os;
#os.system("del test_frame2.txt")
#frame=open("test_frame2.txt","w");

#ba=bytearray(rand.getrandbits(8) for _ in range(400000))
#frame.write("0"*1000000)
#frame.close()
#ba.decode('ASCII');
#os.mkdir("test")
os.chdir("test");
for i in range(1000):
    t_frame=open("test_f"+str(i),"w")
    t_frame.write("0"*1000000)
    t_frame.close()
os.chdir("..")
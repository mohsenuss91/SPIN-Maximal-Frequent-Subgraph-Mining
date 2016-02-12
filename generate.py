import os
import sys

os.system('rm Input/*')

for x in range(1,5):
	for y in range(10,15):
		density = x/float(5)
		filename = "./input_" + str(y) + "_" + str(density)
		query = "java -jar RandomGraphGenerator.jar " + filename + " " + str(y) + " " + str(density) + " " +str(0)
		os.system(query)
		os.system("mv input* ./Input")
		
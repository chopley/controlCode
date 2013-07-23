def filesDiffer(file1, file2):
  input1 = open(file1)
  input2 = open(file2)
  data1 = input1.read()
  input1.close()
  data2 = input2.read()
  input2.close()
  return not(data1 == data2)

input = open('dirfilelist', 'r')
fileList = input.readlines()
prevDirectory = fileList[0][:-1]
for directory in fileList:
  directory = directory[:-1] # strip newline
  if(filesDiffer('dirfiles/' + directory + '/format', 'dirfiles/' + prevDirectory + '/format')):
    print 'echo "=== ' + prevDirectory + ' ' + directory + '==="'
    print 'diff dirfiles/' + prevDirectory + '/format dirfiles/' + directory + '/format'
    print 'cp dirfiles/' + directory + '/format /home/intweb/wikifiles/regmaps/' + directory
  prevDirectory = directory

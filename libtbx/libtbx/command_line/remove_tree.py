import distutils.dir_util
import sys, os
for arg in sys.argv[1:]:
  if (os.path.isfile(arg)):
    os.remove(arg)
  elif (os.path.isdir(arg)):
    distutils.dir_util.remove_tree(arg)

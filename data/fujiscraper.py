import urllib2

for i in xrange(1, 35):
  print i
  data = urllib2.urlopen("http://www.pro.or.jp/~fuji/java/puzzle/numline/%03d.data"% i).read().split("\n")
  f = open("slither.fuji.%d.txt" % i, "wt")
  w, h = map(int, data[0].split()[1:])
  f.write("%s %s\n" % (w, h))
  for j in xrange(2, 2 + h):  
    f.write(data[j].strip().replace(" ", "").replace("-", "."))
    f.write("\n")
  f.close()

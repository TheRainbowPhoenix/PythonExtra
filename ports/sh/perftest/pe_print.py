import time
t1 = time.time()

for i in range(100000):
  print(i)

t2 = time.time()
print(f"{t2-t1} seconds")

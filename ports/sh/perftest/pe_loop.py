import time
t1 = time.time()

for i in range(1000000):
  pass

t2 = time.time()
print(f"{t2-t1} seconds")
